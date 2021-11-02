#include <cstdio>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_recording.h"
#include "blow_detector.h"
#include "cmdline_options.h"
#include "constants.h"
#include "easy_fourier.h"
#include "tongue_detector.h"
#include "train_blow.h"
#include "train_tongue.h"

#include "config_io.h"

void crash(const char* s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

void useMain(ClickitongueCmdlineOpts opts)
{
  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  std::string detector = opts.detector.value();
  if (detector == "blow")
  {
    if (!opts.lowpass_percent.has_value())
      crash("--detector=blow requires a value for --lowpass_percent.");
    if (!opts.highpass_percent.has_value())
      crash("--detector=blow requires a value for --highpass_percent.");
    if (!opts.low_on_thresh.has_value())
      crash("--detector=blow requires a value for --low_on_thresh.");
    if (!opts.low_off_thresh.has_value())
      crash("--detector=blow requires a value for --low_off_thresh.");
    if (!opts.high_on_thresh.has_value())
      crash("--detector=blow requires a value for --high_on_thresh.");
    if (!opts.high_off_thresh.has_value())
      crash("--detector=blow requires a value for --high_off_thresh.");
    if (!opts.high_spike_frac.has_value())
      crash("--detector=blow requires a value for --high_spike_frac.");
    if (!opts.high_spike_level.has_value())
      crash("--detector=blow requires a value for --high_spike_level.");

    BlowDetector clicker(&action_queue, Action::LeftDown, Action::LeftUp,
        opts.lowpass_percent.value(), opts.highpass_percent.value(),
        opts.low_on_thresh.value(), opts.low_off_thresh.value(),
        opts.high_on_thresh.value(), opts.high_off_thresh.value(),
        opts.high_spike_frac.value(), opts.high_spike_level.value());

    AudioInput audio_input(blowDetectorCallback, &clicker, kFourierBlocksize);
    while (audio_input.active())
      Pa_Sleep(500);
  }
  else if (detector == "tongue")
  {
    if (!opts.tongue_low_hz.has_value())
      crash("--detector=tongue requires a value for --tongue_low_hz.");
    if (!opts.tongue_high_hz.has_value())
      crash("--detector=tongue requires a value for --tongue_high_hz.");
    if (!opts.tongue_hzenergy_high.has_value())
      crash("--detector=tongue requires a value for --tongue_hzenergy_high.");
    if (!opts.tongue_hzenergy_low.has_value())
      crash("--detector=tongue requires a value for --tongue_hzenergy_low.");
    if (!opts.refrac_blocks.has_value())
      crash("--detector=tongue requires a value for --refrac_blocks.");

    TongueDetector clicker(&action_queue, Action::ClickLeft,
        opts.tongue_low_hz.value(), opts.tongue_high_hz.value(),
        opts.tongue_hzenergy_high.value(), opts.tongue_hzenergy_low.value(),
        opts.refrac_blocks.value());
    AudioInput audio_input(tongueDetectorCallback, &clicker, kFourierBlocksize);
    while (audio_input.active())
      Pa_Sleep(500);
  }
  action_dispatcher.shutdown();
  action_dispatch.join();
}

int equalizerCallback(const void* inputBuffer, void* outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags, void* userData)
{
  static int print_once_per_10_6ms_chunks = 0;
  if (++print_once_per_10_6ms_chunks == 10)
  {
    print_once_per_10_6ms_chunks = 0;
    g_fourier->printEqualizer(static_cast<const float*>(inputBuffer));
  }
  return paContinue;
}

const char kBadDetectorOpt[] = "Must specify --detector=blow or tongue.";

void validateCmdlineOpts(ClickitongueCmdlineOpts opts)
{
  if (!opts.mode.has_value())
  {
    printf("No --mode= specified; using default behavior (which is probably "
           "what you want).\n");
    return;
  }
  std::string mode = opts.mode.value();
  if (mode != "train" && mode != "use" && mode != "record" &&
      mode != "play" && mode != "equalizer")
  {
    crash("Invalid --mode= value. Must specify --mode=train, use, record,"
          "play, or equalizer. (Or not specify it).");
  }

  if (mode == "train" || mode == "use")
  {
    if (!opts.detector.has_value())
      crash(kBadDetectorOpt);
    if (opts.detector.value() != "blow" && opts.detector.value() != "tongue")
      crash(kBadDetectorOpt);
  }
  else if (mode == "record" || mode == "play")
    if (!opts.filename.has_value())
      crash("Must specify a --filename=");
}

void defaultMain()
{
  if (auto maybe_config = readConfig(); maybe_config.has_value())
  {
    Config config = maybe_config.value();
    // TODO
  }
  else
  {
    fprintf(stderr, "It looks like this is your first time running Clickitongue. First, we're going to train Clickitongue on the acoustics and typical background noise of your particular enivornment.\n"); // TODO

    // TODO writeConfig()
    fprintf(stderr, "Clickitongue should now be configured. Entering normal operation.\n");
    defaultMain();
  }
}

int main(int argc, char** argv)
{
  Pa_Initialize(); // get its annoying spam out of the way immediately
  printf("\n\n******IGNORE THE ABOVE!!! clickitongue is now running.******\n\n");

  ClickitongueCmdlineOpts opts =
      structopt::app("clickitongue").parse<ClickitongueCmdlineOpts>(argc, argv);
  validateCmdlineOpts(opts);
  g_fourier = new EasyFourier(kFourierBlocksize);

  if (opts.mode.has_value())
  {
    if (opts.mode.value() == "record")
    {
      RecordedAudio audio(opts.duration_seconds.value());
      audio.recordToFile(opts.filename.value());
    }
    else if (opts.mode.value() == "play")
    {
      RecordedAudio audio(opts.filename.value());
      audio.play();
    }
    else if (opts.mode.value() == "equalizer")
    {
      AudioInput audio_input(equalizerCallback, nullptr, kFourierBlocksize);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "use")
      useMain(opts);
    else if (opts.mode.value() == "train" && opts.detector.value() == "blow")
      trainBlow(/*verbose=*/true);
    else if (opts.mode.value() == "train" && opts.detector.value() == "tongue")
      trainTongue(/*verbose=*/true);
  }
  else
    defaultMain();

  Pa_Terminate(); // corresponds to the Pa_Initialize() at the start
  return 0;
}

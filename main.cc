#include <cstdio>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_recording.h"
#include "blow_detector.h"
#include "cmdline_options.h"
#include "constants.h"
#include "easy_fourier.h"
#include "interaction.h"
#include "tongue_detector.h"
#include "train_blow.h"
#include "train_tongue.h"

#include "config_io.h"

void crash(const char* s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

std::thread spawnBlowDetector(
    BlockingQueue<Action>* action_queue, Action action_on, Action action_off,
    double lowpass_percent, double highpass_percent, double low_on_thresh,
    double low_off_thresh, double high_on_thresh, double high_off_thresh,
    double high_spike_frac, double high_spike_level)
{
  return std::thread([=]()
  {
    BlowDetector clicker(action_queue, action_on, action_off, lowpass_percent,
                         highpass_percent, low_on_thresh, low_off_thresh,
                         high_on_thresh, high_off_thresh, high_spike_frac,
                         high_spike_level);
    AudioInput audio_input(blowDetectorCallback, &clicker, kFourierBlocksize);
    while (audio_input.active())
      Pa_Sleep(500);
  });
}

std::thread spawnTongueDetector(
    BlockingQueue<Action>* action_queue, Action action, double tongue_low_hz,
    double tongue_high_hz, double tongue_hzenergy_high,
    double tongue_hzenergy_low, int refrac_blocks)
{
  return std::thread([=]()
  {
    TongueDetector clicker(action_queue, action, tongue_low_hz, tongue_high_hz,
                           tongue_hzenergy_high, tongue_hzenergy_low,
                           refrac_blocks);
    AudioInput audio_input(tongueDetectorCallback, &clicker, kFourierBlocksize);
    while (audio_input.active())
      Pa_Sleep(500);
  });
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

    std::thread blow_thread = spawnBlowDetector(
        &action_queue, Action::LeftDown, Action::LeftUp,
        opts.lowpass_percent.value(), opts.highpass_percent.value(),
        opts.low_on_thresh.value(), opts.low_off_thresh.value(),
        opts.high_on_thresh.value(), opts.high_off_thresh.value(),
        opts.high_spike_frac.value(), opts.high_spike_level.value());
    blow_thread.join();
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

    std::thread tongue_thread = spawnTongueDetector(
        &action_queue, Action::ClickLeft, opts.tongue_low_hz.value(),
        opts.tongue_high_hz.value(), opts.tongue_hzenergy_high.value(),
        opts.tongue_hzenergy_low.value(), opts.refrac_blocks.value());
    tongue_thread.join();
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
    return;
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

void normalOperation(Config config)
{
  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  std::thread blow_thread;
  if (config.blow.enabled)
  {
    printf("spawning blow detector for left clicks\n");
    blow_thread = spawnBlowDetector(
        &action_queue, config.blow.action_on, config.blow.action_off,
        config.blow.lowpass_percent, config.blow.highpass_percent,
        config.blow.low_on_thresh, config.blow.low_off_thresh,
        config.blow.high_on_thresh, config.blow.high_off_thresh,
        config.blow.high_spike_frac, config.blow.high_spike_level);
  }
  std::thread tongue_thread;
  if (config.tongue.enabled)
  {
    printf("spawning tongue detector for %s clicks\n",
           config.tongue.action == Action::ClickRight ? "right" : "left");
    tongue_thread = spawnTongueDetector(
        &action_queue, config.tongue.action, config.tongue.tongue_low_hz,
        config.tongue.tongue_high_hz, config.tongue.tongue_hzenergy_high,
        config.tongue.tongue_hzenergy_low, config.tongue.refrac_blocks);
  }

  if (config.blow.enabled)
    blow_thread.join();
  if (config.tongue.enabled)
    tongue_thread.join();
  action_dispatcher.shutdown();
  action_dispatch.join();
}

void firstTimeTrain()
{
  promptInfo(
"It looks like this is your first time running Clickitongue.\n"
"First, we're going to train Clickitongue on the acoustics\n"
"and typical background noise of your particular enivornment.");

  bool try_blows = promptYesNo(
"Will you be able to keep your mic positioned no more than a few centimeters\n"
"from your mouth for long-term usage? If so, Clickitongue will be able to \n"
"recognize blowing in addition to tongue clicking, allowing both left and \n"
"right clicking. Otherwise, Clickitongue will only be able to left click.\n\n"
"So: will your mic be close enough to directly blow on?");

  Config config;
  if (try_blows)
    config.blow = trainBlow();
  Action tongue_action = config.blow.enabled ? Action::ClickRight
                                             : Action::ClickLeft;
  config.tongue = trainTongue(tongue_action);

  if (config.blow.enabled && config.tongue.enabled)
  {
    promptInfo(
"Clickitongue should now be configured. Blow on the mic to left click, keep "
"blowing to hold the left 'button' down. Tongue click to right click.\n\n"
"Now entering normal operation.");
  }
  else if (config.tongue.enabled)
  {
    promptInfo(
"Clickitongue should now be configured. Tongue click to left click.\n\n"
"Now entering normal operation.");
  }
  else
  {
    promptInfo(
"Clickitongue was not able to find parameters that distinguish your tongue "
"clicks from background noise with acceptable accuracy. Remove sources of "
"background noise, move the mic closer to your mouth, and try again.");
    return;
  }
#ifdef CLICKITONGUE_WINDOWS
  promptInfo(
"A note on admin privileges:\n\nWindows does not allow lesser privileged "
"processes to send clicks to processes running as admin. If you ever find that "
"just one particular window refuses to listen to Clickitongue's clicks, try "
"running Clickitongue as admin. (But you might never encounter this problem.)");
#endif

  if (!writeConfig(config))
  {
    promptInfo("Failed to write config file. You'll have to redo this "
               "training the next time you run Clickitongue.");
  }
  normalOperation(config);
}

void defaultMain()
{
  if (auto maybe_config = readConfig(); maybe_config.has_value())
    normalOperation(maybe_config.value());
  else
    firstTimeTrain();
}

int main(int argc, char** argv)
{
  Pa_Initialize(); // get its annoying spam out of the way immediately
#ifndef CLICKITONGUE_WINDOWS
  promptInfo("****clickitongue is now running.****");
#endif

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
      trainTongue(Action::ClickLeft, /*verbose=*/true);
  }
  else
    defaultMain();

  Pa_Terminate(); // corresponds to the Pa_Initialize() at the start
  return 0;
}

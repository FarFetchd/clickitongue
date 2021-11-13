#include <cstdio>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_recording.h"
#include "cmdline_options.h"
#include "constants.h"
#include "easy_fourier.h"
#include "fft_result_distributor.h"
#include "hum_detector.h"
#include "interaction.h"
#include "blow_detector.h"
#include "train_hum.h"
#include "train_blow.h"

#include "config_io.h"

void crash(const char* s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

int equalizerCallback(const void* inputBuffer, void* outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags, void* userData)
{
  g_fourier->printEqualizer(static_cast<const float*>(inputBuffer));
  return paContinue;
}

int spikesCallback(const void* inputBuffer, void* outputBuffer,
                   unsigned long framesPerBuffer,
                   const PaStreamCallbackTimeInfo* timeInfo,
                   PaStreamCallbackFlags statusFlags, void* userData)
{
  g_fourier->printTopTwoSpikes(static_cast<const float*>(inputBuffer));
  return paContinue;
}

int octavesCallback(const void* inputBuffer, void* outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags, void* userData)
{
  g_fourier->printOctavePowers(static_cast<const float*>(inputBuffer));
  return paContinue;
}

int overtonesCallback(const void* inputBuffer, void* outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags, void* userData)
{
  g_fourier->printOvertones(static_cast<const float*>(inputBuffer));
  return paContinue;
}

void validateCmdlineOpts(ClickitongueCmdlineOpts opts)
{
  if (!opts.mode.has_value())
    return;
  std::string mode = opts.mode.value();
  if (mode != "record" && mode != "play" && mode != "equalizer" &&
      mode != "spikes" && mode != "octaves" && mode != "overtones")
  {
    crash("Invalid --mode= value. Must specify --mode=train, use, record,"
          "play, equalizer, spikes, octaves, or overtones. (Or not specify it).");
  }

  if ((mode == "record" || mode == "play") && !opts.filename.has_value())
    crash("Must specify a --filename=");
}

void normalOperation(Config config)
{
  if (!config.blow.enabled && !config.hum.enabled)
  {
    promptInfo("Neither hum nor blow is enabled. Exiting.");
    return;
  }

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  if (config.blow.enabled)
    printf("spawning blow detector for left clicks\n");
  if (config.hum.enabled)
  {
    printf("spawning hum detector for %s clicks\n",
           config.hum.action_on == Action::RightDown ? "right" : "left");
  }
  printf("\ndetection parameters:\n%s\n", config.toString().c_str());

  std::vector<std::unique_ptr<Detector>> detectors;
  if (config.blow.enabled)
  {
    detectors.emplace_back(std::make_unique<BlowDetector>(
        &action_queue, config.blow.action_on, config.blow.action_off,
        config.blow.o5_on_thresh, config.blow.o5_off_thresh, config.blow.o6_on_thresh,
        config.blow.o6_off_thresh, config.blow.o7_on_thresh, config.blow.o7_off_thresh,
        config.blow.ewma_alpha));
  }
  if (config.hum.enabled)
  {
    detectors.emplace_back(std::make_unique<HumDetector>(
        &action_queue, config.hum.action_on, config.hum.action_off,
        config.hum.o1_on_thresh, config.hum.o1_off_thresh, config.hum.o6_limit,
        config.hum.ewma_alpha));
  }
  FFTResultDistributor fft_distributor(std::move(detectors));

  AudioInput audio_input(fftDistributorCallback, &fft_distributor, kFourierBlocksize);
  while (audio_input.active())
    Pa_Sleep(500);

  action_dispatcher.shutdown();
  action_dispatch.join();
}

constexpr bool DOING_DEVELOPMENT_TESTING = true;
constexpr char kDefaultConfig[] = "default";
void firstTimeTrain()
{
  promptInfo(
"It looks like this is your first time running Clickitongue.\n"
"First, we're going to train Clickitongue on the acoustics\n"
"and typical background noise of your particular enivornment.");

  bool try_blows = promptYesNo(
"Will you be able to keep your mic positioned no more than a few centimeters\n"
"from your mouth for long-term usage? If so, Clickitongue will be able to \n"
"recognize blowing in addition to humming, allowing both left and \n"
"right clicking. Otherwise, Clickitongue will only be able to left click.\n\n"
"So: will your mic be close enough to directly blow on?");

  std::vector<std::pair<AudioRecording, int>> blow_examples;
  if (try_blows)
  {
    if (DOING_DEVELOPMENT_TESTING) // for easy development of the code
    {
      blow_examples.emplace_back(AudioRecording("data/blows_0.pcm"), 0);
      blow_examples.emplace_back(AudioRecording("data/blows_1.pcm"), 1);
      blow_examples.emplace_back(AudioRecording("data/blows_2.pcm"), 2);
      blow_examples.emplace_back(AudioRecording("data/blows_3.pcm"), 3);
      blow_examples.emplace_back(AudioRecording("data/blows_4.pcm"), 4);
      blow_examples.emplace_back(AudioRecording("data/blows_5.pcm"), 5);
    }
    else // for actual use
      for (int i = 0; i < 6; i++)
        blow_examples.emplace_back(recordExampleBlow(i), i);
  }
  std::vector<std::pair<AudioRecording, int>> hum_examples;
  if (DOING_DEVELOPMENT_TESTING) // for easy development of the code
  {
    hum_examples.emplace_back(AudioRecording("data/hums_0.pcm"), 0);
    hum_examples.emplace_back(AudioRecording("data/hums_1.pcm"), 1);
    hum_examples.emplace_back(AudioRecording("data/hums_2.pcm"), 2);
    hum_examples.emplace_back(AudioRecording("data/hums_3.pcm"), 3);
    hum_examples.emplace_back(AudioRecording("data/hums_4.pcm"), 4);
    hum_examples.emplace_back(AudioRecording("data/hums_5.pcm"), 5);
  }
  else // for actual use
    for (int i = 0; i < 6; i++)
      hum_examples.emplace_back(recordExampleHum(i), i);

  // all examples of one are negative examples for the other
  std::vector<std::pair<AudioRecording, int>> hum_examples_plus_neg;
  std::vector<std::pair<AudioRecording, int>> blow_examples_plus_neg;
  for (auto const& ex_pair : blow_examples)
    blow_examples_plus_neg.emplace_back(ex_pair.first, ex_pair.second);
  for (auto const& ex_pair : hum_examples)
    hum_examples_plus_neg.emplace_back(ex_pair.first, ex_pair.second);
  for (auto const& ex_pair : blow_examples)
    hum_examples_plus_neg.emplace_back(ex_pair.first, 0);
  for (auto const& ex_pair : hum_examples)
    blow_examples_plus_neg.emplace_back(ex_pair.first, 0);

  Config config;
  if (try_blows)
    config.blow = trainBlow(blow_examples_plus_neg, true);

  Action hum_on_action = config.blow.enabled ? Action::RightDown
                                             : Action::LeftDown;
  Action hum_off_action = config.blow.enabled ? Action::RightUp
                                              : Action::LeftUp;
  config.hum = trainHum(hum_examples_plus_neg, hum_on_action, hum_off_action, true);

  if (config.blow.enabled && config.hum.enabled)
  {
    promptInfo(
"Clickitongue should now be configured. Blow on the mic to left click, keep "
"blowing to hold the left 'button' down. Hum for right.\n\n"
"Now entering normal operation.");
  }
  else if (config.hum.enabled)
  {
    promptInfo(
"Clickitongue should now be configured. Hum to left click.\n\n"
"Now entering normal operation.");
  }
  else
  {
    promptInfo(
"Clickitongue was not able to find parameters that distinguish your humming "
"from background noise with acceptable accuracy. Remove sources of "
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

  if (!writeConfig(config, kDefaultConfig))
  {
    promptInfo("Failed to write config file. You'll have to redo this "
               "training the next time you run Clickitongue.");
  }
  normalOperation(config);
}

void defaultMain()
{
  if (auto maybe_config = readConfig(kDefaultConfig); maybe_config.has_value())
    normalOperation(maybe_config.value());
  else
    firstTimeTrain();
}

#ifdef CLICKITONGUE_LINUX
#include <unistd.h> // for geteuid
#endif

int main(int argc, char** argv)
{
  Pa_Initialize(); // get its annoying spam out of the way immediately
#ifndef CLICKITONGUE_WINDOWS
  promptInfo("****clickitongue is now running.****");
#endif // CLICKITONGUE_WINDOWS

  ClickitongueCmdlineOpts opts =
      structopt::app("clickitongue").parse<ClickitongueCmdlineOpts>(argc, argv);
  validateCmdlineOpts(opts);
  g_fourier = new EasyFourier();

  if (opts.mode.has_value())
  {
    if (opts.mode.value() == "record")
    {
      AudioRecording audio(opts.duration_seconds.value());
      audio.recordToFile(opts.filename.value());
    }
    else if (opts.mode.value() == "play")
    {
      AudioRecording audio(opts.filename.value());
      audio.play();
    }
    else if (opts.mode.value() == "equalizer")
    {
      AudioInput audio_input(equalizerCallback, nullptr, kFourierBlocksize);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "spikes")
    {
      AudioInput audio_input(spikesCallback, nullptr, kFourierBlocksize);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "octaves")
    {
      AudioInput audio_input(octavesCallback, nullptr, kFourierBlocksize);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "overtones")
    {
      AudioInput audio_input(overtonesCallback, nullptr, kFourierBlocksize);
      while (audio_input.active())
        Pa_Sleep(500);
    }
  }
  else
  {
#ifdef CLICKITONGUE_LINUX
    if (geteuid() != 0)
      crash("clickitongue must be run as root.");
    initLinuxUinput();
#endif // CLICKITONGUE_LINUX
    defaultMain();
  }

  Pa_Terminate(); // corresponds to the Pa_Initialize() at the start
  return 0;
}

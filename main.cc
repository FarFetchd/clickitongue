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

void describeLoadedParams(Config config)
{
  std::string msg = "Clickitongue started!\n";
  if (config.blow.enabled)
  {
    msg += std::string("Blow to ") +
           (config.blow.action_on == Action::RightDown ? "right" : "left") +
           " click.\n";
  }
  if (config.hum.enabled)
  {
    msg += std::string("Hum to ") +
           (config.hum.action_on == Action::RightDown ? "right" : "left") +
           " click.\n";
  }
  // TODO whistle
  promptInfo(msg.c_str());
  //printf("\ndetection parameters:\n%s\n", config.toString().c_str());
}

std::vector<std::unique_ptr<Detector>> makeDetectorsFromConfig(
    Config config, BlockingQueue<Action>* action_queue)
{
  std::unique_ptr<Detector> blow_detector;
  if (config.blow.enabled)
  {
    blow_detector = std::make_unique<BlowDetector>(
        action_queue, config.blow.action_on, config.blow.action_off,
        config.blow.o5_on_thresh, config.blow.o5_off_thresh, config.blow.o6_on_thresh,
        config.blow.o6_off_thresh, config.blow.o7_on_thresh, config.blow.o7_off_thresh,
        config.blow.ewma_alpha);
  }
  std::unique_ptr<Detector> hum_detector;
  if (config.hum.enabled)
  {
    hum_detector = std::make_unique<HumDetector>(
        action_queue, config.hum.action_on, config.hum.action_off,
        config.hum.o1_on_thresh, config.hum.o1_off_thresh, config.hum.o6_limit,
        config.hum.ewma_alpha, /*require_warmup=*/config.blow.enabled);
    if (blow_detector)
      blow_detector->addInhibitionTarget(hum_detector.get());
  }

  std::vector<std::unique_ptr<Detector>> detectors;
  if (config.blow.enabled)
    detectors.push_back(std::move(blow_detector));
  if (config.hum.enabled)
    detectors.push_back(std::move(hum_detector));
  return detectors;
}

double loadScaleFromConfig(Config config)
{
  double scale = 1;
  if (config.blow.enabled)
    scale = config.blow.scale;
  else if (config.hum.enabled)
    scale = config.hum.scale;
//   else if (config.whistle.enabled) TODO
//     scale = config.whistle.scale;

  if (config.blow.enabled && config.hum.enabled &&
      config.hum.scale != config.blow.scale)
  {
    promptInfo("Error! Blow and hum both enabled with different scaling factors.");
    exit(1);
  }
//   if (config.whistle.enabled && config.hum.enabled && TODO
//       config.hum.scale != config.whistle.scale)
//   {
//     promptInfo("Error! Whistle and hum both enabled with different scaling factors.");
//     exit(1);
//   }
//   if (config.blow.enabled && config.whistle.enabled &&
//       config.whistle.scale != config.blow.scale)
//   {
//     promptInfo("Error! Blow and whistle both enabled with different scaling factors.");
//     exit(1);
//   }
  return scale;
}

void normalOperation(Config config)
{
  if (!config.blow.enabled && !config.hum.enabled) // TODO whistle
  {
    promptInfo("Neither hum nor blow is enabled. Exiting.");
    return;
  }

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  FFTResultDistributor fft_distributor(makeDetectorsFromConfig(config, &action_queue),
                                       loadScaleFromConfig(config));
  AudioInput audio_input(fftDistributorCallback, &fft_distributor, kFourierBlocksize);
  describeLoadedParams(config);

  while (audio_input.active())
    Pa_Sleep(500);

  action_dispatcher.shutdown();
  action_dispatch.join();
}

void displayAndReset(std::string* msg)
{
  promptInfo(msg->c_str());
  *msg = "";

// TODO OSX
#ifdef CLICKITONGUE_LINUX
  printf("Press any key to continue to the training prompt.\n\n");
  make_getchar_like_getch(); getchar(); resetTermios();
#endif
}

constexpr char kDefaultConfig[] = "default";
void firstTimeTrain()
{
  promptInfo(
"It looks like this is your first time running Clickitongue.\n"
"First, we're going to train Clickitongue on the acoustics\n"
"and typical background noise of your particular enivornment.");

  bool try_blows = promptYesNo(
"Can you keep your mic positioned ~1cm from your mouth for long-term usage? ");

  // TODO needed for OSX too?
#ifdef CLICKITONGUE_LINUX
  printf("press any key if this message isn't automatically advanced beyond...");
  fflush(stdout);
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("(ok, now advanced beyond that message)\n");
  promptInfo("About to start training...");
#endif

  std::string intro_message;
  if (try_blows)
  {
    intro_message +=
"Great! You'll be able to do both left- and right-clicks, using blowing and\n"
"humming. Ensure your mic is in position ~1cm from your mouth. If your mic has\n"
"a foamy/fuzzy windscreen, remove it for best results.\n\n";
  }
  else
  {
    intro_message +=
"Ok, we will fall back to left-clicks only, controlled by humming.\n\n";
  }

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
    {
      intro_message +=
"We will first train Clickitongue on your blowing. These should be extremely\n"
"gentle puffs of air, blown directly onto the mic. Don't try too hard to make\n"
"these training examples strong and clear, or else Clickitongue will expect\n"
"that much strength every time. These puffs should be more focused than simply\n"
"exhaling through an open mouth, but weaker than just about any other blow.\n"
"Think of trying to propel an eyelash over a hand's width.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of blows.\n\n";
      displayAndReset(&intro_message);
      for (int i = 0; i < 6; i++)
        blow_examples.emplace_back(recordExampleBlow(i), i);
      blow_examples.emplace_back(recordExampleBlow(1, /*prolonged=*/true), 1);
    }
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
  {
    intro_message +=
"We will now train Clickitongue on your humming. These hums should be simple,\n"
"relatively quiet closed-mouth hums, like saying 'hm' in reaction to something\n"
"just a tiny bit interesting.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of hums.\n\n";
    displayAndReset(&intro_message);
    for (int i = 0; i < 6; i++)
      hum_examples.emplace_back(recordExampleHum(i), i);
    hum_examples.emplace_back(recordExampleHum(1, /*prolonged=*/true), 1);
  }

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

  // TODO scale whistle
  double scale = 1.0;
  if (try_blows)
    scale = pickBlowScalingFactor(blow_examples_plus_neg);
  else
    scale = pickHumScalingFactor(hum_examples_plus_neg);
  printf("using scale %g\n", scale);

  Config config;
  if (try_blows)
    config.blow = trainBlow(blow_examples_plus_neg, scale, true);

  Action hum_on_action = config.blow.enabled ? Action::RightDown
                                             : Action::LeftDown;
  Action hum_off_action = config.blow.enabled ? Action::RightUp
                                              : Action::LeftUp;
  config.hum = trainHum(hum_examples_plus_neg, hum_on_action, hum_off_action,
                        scale, true);

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

  std::string attempted_filepath;
  if (!writeConfig(config, kDefaultConfig, &attempted_filepath))
  {
    std::string msg = "Failed to write config file " + attempted_filepath +
    ". You'll have to redo this training the next time you run Clickitongue.";
    promptInfo(msg.c_str());
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
  if (kNumChannels != 2)
  {
    promptInfo("kNumChannels must be 2! The code that feeds into the FFT "
               "averages [sample ind] and [sample ind + 1] together, under "
               "the assumption that each frame is 2 samples. If you need "
               "to change kNumChannels, you must change that code. (Don't) "
               "just look only at sample 0 - perhaps only sample 1 has data. ");
    return 1;
  }

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

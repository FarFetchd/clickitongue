#include <cstdio>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_recording.h"
#include "blow_detector.h"
#include "cmdline_options.h"
#include "constants.h"
#include "easy_fourier.h"
#include "fft_result_distributor.h"
#include "hum_detector.h"
#include "interaction.h"
#include "main_train.h"
#include "train_blow.h"
#include "train_hum.h"

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
        config.hum.o1_on_thresh, config.hum.o1_off_thresh, config.hum.o2_on_thresh,
        config.hum.o3_limit, config.hum.o6_limit, config.hum.ewma_alpha,
        /*require_warmup=*/config.blow.enabled);
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

void defaultMain(bool ignore_existing_config)
{
  if (ignore_existing_config)
    firstTimeTrain();
  else if (auto maybe_cfg = readConfig(kDefaultConfig); maybe_cfg.has_value())
    normalOperation(maybe_cfg.value());
  else
    firstTimeTrain();
}

#ifdef CLICKITONGUE_LINUX
#include <unistd.h> // for geteuid
#endif

extern bool g_forget_input_dev;
int main(int argc, char** argv)
{
  if (kNumChannels != 2)
  {
    promptInfo("kNumChannels must be 2! The code that feeds into the FFT "
               "averages [sample ind] and [sample ind + 1] together, under "
               "the assumption that each frame is 2 samples. If you need "
               "to change kNumChannels, you must change that code. (Don't "
               "just look only at sample 0 - perhaps only sample 1 has data.)");
    return 1;
  }

  Pa_Initialize(); // get its annoying spam out of the way immediately
#ifndef CLICKITONGUE_WINDOWS
  promptInfo("****clickitongue is now running.****");
#endif // CLICKITONGUE_WINDOWS
  printf("clickitongue v0.0.1\n");

  ClickitongueCmdlineOpts opts =
      structopt::app("clickitongue").parse<ClickitongueCmdlineOpts>(argc, argv);
  validateCmdlineOpts(opts);
  g_forget_input_dev = opts.forget_input_dev.value();
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
    defaultMain(opts.retrain.value());
  }

  Pa_Terminate(); // corresponds to the Pa_Initialize() at the start
  return 0;
}

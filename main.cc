#include <cstdio>
#include <mutex>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_recording.h"
#include "blow_detector.h"
#include "cat_detector.h"
#include "cmdline_options.h"
#include "constants.h"
#include "easy_fourier.h"
#include "fft_result_distributor.h"
#include "hum_detector.h"
#include "interaction.h"
#include "main_train.h"

#include "config_io.h"

int g_num_channels = 2;

// The Pa_Terminate() documentation is... menacing... about what happens if
// every Pa_Initialize() call isn't matched before exiting. So let's be sure.
// (Never call naked exit(), Pa_Initialize(), or Pa_Terminate()!)
int g_unresolved_pulseaudio_inits = 0;
bool g_shutting_down = false;
std::mutex g_pa_init_mutex;
PaError initPulseAudio()
{
  const std::lock_guard<std::mutex> lock(g_pa_init_mutex);
  if (g_shutting_down)
  {
    while (g_unresolved_pulseaudio_inits > 0)
    {
      g_unresolved_pulseaudio_inits--;
      Pa_Terminate();
    }
    exit(0);
  }
  g_unresolved_pulseaudio_inits++;
  PaError res = Pa_Initialize();
  static bool have_initd_pulseaudio = false;
  if (!have_initd_pulseaudio)
  {
#ifndef CLICKITONGUE_WINDOWS
    if (res == paNoError)
      promptInfo("Clickitongue successfully initialized PulseAudio.");
    else
      PRINTERR(stderr, "\n\nFailed to initialize PulseAudio: PaError %d\n", res);
#endif
    have_initd_pulseaudio = true;
  }
  return res;
}
PaError deinitPulseAudio()
{
  const std::lock_guard<std::mutex> lock(g_pa_init_mutex);
  if (g_shutting_down)
    return paNoError;
  if (g_unresolved_pulseaudio_inits > 0)
  {
    g_unresolved_pulseaudio_inits--;
    return Pa_Terminate();
  }
  return paNoError;
}
void makeSafeToExit()
{
  const std::lock_guard<std::mutex> lock(g_pa_init_mutex);
  g_shutting_down = true;
  while (g_unresolved_pulseaudio_inits > 0)
  {
    g_unresolved_pulseaudio_inits--;
    Pa_Terminate();
  }
}
void safelyExit(int exit_code)
{
  makeSafeToExit();
  exit(exit_code);
}
void crash(const char* s)
{
  PRINTERR(stderr, "%s\n", s);
  safelyExit(1);
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

void printDeviceDetails()
{
  int num_devices = Pa_GetDeviceCount();
  if (num_devices < 0)
  {
    PRINTERR(stderr, "Error: Pa_GetDeviceCount() returned %d\n", num_devices);
    safelyExit(1);
  }

  for (PaDeviceIndex i=0; i<num_devices; i++)
  {
    const PaDeviceInfo* dev_info = Pa_GetDeviceInfo(i);
    if (!dev_info)
    {
      PRINTERR(stderr, "Error: Pa_GetDeviceInfo(%d) returned null\n", i);
      safelyExit(1);
    }
    PRINTF("dev %d: %s, max channels %d%s\n",
           i, dev_info->name, dev_info->maxInputChannels,
           dev_info->maxInputChannels <= 0 ? " (output only!)" : "");
  }
}

void validateCmdlineOpts(ClickitongueCmdlineOpts opts)
{
  if (!opts.mode.has_value())
    return;
  std::string mode = opts.mode.value();
  if (mode != "record" && mode != "play" && mode != "equalizer" &&
      mode != "spikes" && mode != "octaves" && mode != "overtones" &&
      mode != "devdetails")
  {
    crash("Invalid --mode= value. Must specify --mode=train, use, record,\n"
          "play, equalizer, spikes, octaves, overtones, or devdetails.\n"
          "(Or not specify it).");
  }

  if ((mode == "record" || mode == "play") && !opts.filename.has_value())
    crash("Must specify a --filename=");
}

extern bool g_show_debug_info;
void describeLoadedParams(Config config, bool first_time)
{
  std::string msg = "Clickitongue " CLICKITONGUE_VERSION " started!\n";
  if (config.blow.enabled && config.blow.action_on != Action::NoAction)
  {
    msg += std::string("Blow to ") +
           (config.blow.action_on == Action::RightDown ? "right" : "left") +
           " click.\n";
  }
  if (config.cat.enabled && config.cat.action_on != Action::NoAction)
  {
    msg += std::string("Make a sound like getting a cat's attention to ") +
           (config.cat.action_on == Action::RightDown ? "right" : "left") +
           " click.\n";
  }
  if (config.hum.enabled && config.hum.action_on != Action::NoAction)
  {
    msg += std::string("Hum to ") +
           (config.hum.action_on == Action::RightDown ? "right" : "left") +
           " click.\n";
  }
  if (first_time && !g_show_debug_info)
    promptInfo(msg.c_str());
  else
    PRINTF("%s", msg.c_str());

#ifdef CLICKITONGUE_WINDOWS
  PRINTF(
"A note on admin privileges:\n"
"Windows does not allow lesser privileged processes to send clicks to processes\n"
"running as admin. If you ever find that just one particular window refuses to\n"
"listen to Clickitongue's clicks, try running Clickitongue as admin.\n");
#endif
}

std::vector<std::unique_ptr<Detector>> makeDetectorsFromConfig(
    Config config, BlockingQueue<Action>* action_queue)
{
  std::unique_ptr<Detector> cat_detector;
  if (config.cat.enabled)
  {
    cat_detector = std::make_unique<CatDetector>(
        action_queue, config.cat.action_on, config.cat.action_off,
        config.cat.o7_on_thresh, config.cat.o1_limit, config.cat.use_limit);
    g_HACK_all_detectors.push_back(cat_detector.get());
  }

  std::unique_ptr<Detector> blow_detector;
  if (config.blow.enabled)
  {
    blow_detector = std::make_unique<BlowDetector>(
        action_queue, config.blow.action_on, config.blow.action_off,
        config.blow.o1_on_thresh, config.blow.o7_on_thresh,
        config.blow.o7_off_thresh, config.blow.lookback_blocks,
        /*require_warmup=*/config.cat.enabled);
    if (cat_detector)
    {
      blow_detector->addInhibitionTarget(cat_detector.get());
      cat_detector->addInhibitionTarget(blow_detector.get());
    }
    g_HACK_all_detectors.push_back(blow_detector.get());
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
    if (cat_detector)
      cat_detector->addInhibitionTarget(hum_detector.get());
    g_HACK_all_detectors.push_back(hum_detector.get());
  }

  std::vector<std::unique_ptr<Detector>> detectors;
  if (blow_detector)
    detectors.push_back(std::move(blow_detector));
  if (cat_detector)
    detectors.push_back(std::move(cat_detector));
  if (hum_detector)
    detectors.push_back(std::move(hum_detector));
  return detectors;
}

double loadScaleFromConfig(Config config)
{
  double scale = 1;
  if (config.blow.enabled)
    scale = config.blow.scale;
  if (config.cat.enabled)
    scale = config.cat.scale;
  if (config.hum.enabled)
    scale = config.hum.scale;

  if ((config.blow.enabled && scale != config.blow.scale) ||
      (config.cat.enabled && scale != config.cat.scale) ||
      (config.hum.enabled && scale != config.hum.scale))
  {
    crash("All detectors enabled in a config must have the same scaling factor.");
  }
  return scale;
}

void normalOperation(Config config, bool first_time)
{
  if (!config.blow.enabled && !config.cat.enabled &&
      !config.hum.enabled)
  {
    PRINTF("no mouse control configured - only voice-to-LLM is active.\n");
  }

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  FFTResultDistributor fft_distributor(
      makeDetectorsFromConfig(config, &action_queue), loadScaleFromConfig(config),
      /*training=*/false);
  AudioInput audio_input(fftDistributorCallback, &fft_distributor, kFourierBlocksize, kFramesPerSec, paFloat32, g_num_channels);
  describeLoadedParams(config, first_time);

  if (config.whisper_url.size() > 6)
  {
    PRINTF("voice-to-LLM coding mode enabled!\n");
    PRINTF("you'll need to install xsel if you haven't already. this mode won't work on Wayland.\n");
    PRINTF("whisper URL: %s\n", config.whisper_url.c_str());
    PRINTF("athene URL: %s\n", config.athene_url.c_str());
    std::thread read_fifo_thread(readFIFO);
    read_fifo_thread.detach();
  }

  while (audio_input.active())
    Pa_Sleep(500);

  action_dispatcher.shutdown();
  action_dispatch.join();
}

void defaultMain(bool ignore_existing_config)
{
  std::string config_name = kDefaultConfig;
  if (ignore_existing_config)
    firstTimeTrain();
  else if (auto maybe_cfg = readConfig(config_name); maybe_cfg.has_value())
  {
    PRINTF("Read existing config profile '%s'.\n", config_name.c_str());
    normalOperation(maybe_cfg.value(), /*first_time=*/false);
  }
  else
    firstTimeTrain();
}

extern bool g_forget_input_dev;

#ifdef CLICKITONGUE_LINUX
#include <unistd.h> // for geteuid
char* g_program_path = nullptr;
#endif

#ifndef CLICKITONGUE_WINDOWS
#include <csignal>
volatile sig_atomic_t g_shutdown_flag = 0;
void sigintHandler(int signal)
{
  g_shutdown_flag = 1;
}
void signalWatcher()
{
  while (g_shutdown_flag == 0)
    Pa_Sleep(200);
  safelyExit(0);
}
#endif


#ifdef CLICKITONGUE_WINDOWS
#include "windows_gui.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else // not windows
int main(int argc, char** argv)
#endif
{
  ClickitongueCmdlineOpts opts;
#ifndef CLICKITONGUE_WINDOWS
  std::thread sigint_watcher_thread(signalWatcher);
  sigint_watcher_thread.detach();
  signal(SIGINT, sigintHandler);
  opts = structopt::app("clickitongue", CLICKITONGUE_VERSION)
             .parse<ClickitongueCmdlineOpts>(argc, argv);
  validateCmdlineOpts(opts);
#else // windows
  std::thread win_gui_thread(windowsGUI, hInstance, nCmdShow);
  win_gui_thread.detach();
#endif
  if (initPulseAudio() != paNoError) // get its annoying spam out of the way
    crash("Pa_Initialize() failed. Giving up.");

  g_show_debug_info = opts.debug.value();
  g_forget_input_dev = opts.forget_input_dev.value();
  g_fourier = new EasyFourier();
#ifdef CLICKITONGUE_LINUX
  g_program_path = realpath(argv[0], nullptr);
  PRINTF("hi %s\n", g_program_path);
#endif

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
      AudioInput audio_input(equalizerCallback, nullptr, kFourierBlocksize, kFramesPerSec, paFloat32, g_num_channels);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "spikes")
    {
      AudioInput audio_input(spikesCallback, nullptr, kFourierBlocksize, kFramesPerSec, paFloat32, g_num_channels);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "octaves")
    {
      AudioInput audio_input(octavesCallback, nullptr, kFourierBlocksize, kFramesPerSec, paFloat32, g_num_channels);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "overtones")
    {
      AudioInput audio_input(overtonesCallback, nullptr, kFourierBlocksize, kFramesPerSec, paFloat32, g_num_channels);
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (opts.mode.value() == "devdetails")
      printDeviceDetails();
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

  deinitPulseAudio(); // corresponds to the initPulseAudio() at the start
  makeSafeToExit();
  return 0;
}

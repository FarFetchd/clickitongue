#include "audio_input.h"

#include "config_io.h"
#include "interaction.h"

#include <cstdio>
#include <cstdlib>

#ifdef CLICKITONGUE_LINUX
#include "pa_linux_alsa.h"
#endif // CLICKITONGUE_LINUX

void crash(PaError err)
{
  Pa_Terminate();
  fprintf(stderr, "An error occurred while using the portaudio stream\n");
  fprintf(stderr, "Error number: %d\n", err);
  fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
  exit(1);
}

int recordCallback(const void* input_buf, void* output_buf,
                   unsigned long frames_provided,
                   const PaStreamCallbackTimeInfo* time_info,
                   PaStreamCallbackFlags status_flags, void* user_data)
{
  RecordingState* data_ptr = static_cast<RecordingState*>(user_data);
  const Sample* cur_in = static_cast<const Sample*>(input_buf);
  std::vector<Sample>* recorded_samples = &data_ptr->samples;
  int done_or_continue = paContinue;

  long frames_left = data_ptr->size_frames - data_ptr->frame_index;
  if (frames_left < frames_provided)
  {
    frames_provided = frames_left;
    done_or_continue = paComplete;
  }

  if (input_buf != NULL)
  {
    for (int i=0; i<frames_provided; i++)
    {
      recorded_samples->push_back(*cur_in++); // left or mono
      if (kNumChannels == 2)
        recorded_samples->push_back(*cur_in++); // right
    }
  }
  else
  {
    for (int i=0; i<frames_provided; i++)
    {
      recorded_samples->push_back(kSilentSample);  // left or mono
      if (kNumChannels == 2)
        recorded_samples->push_back(kSilentSample);  // right
    }
  }
  data_ptr->frame_index += frames_provided;
  return done_or_continue;
}

const char kNotInputDev[] = "CLICKITONGUENOTANINPUTDEVICE";
std::vector<std::string> getDeviceNames()
{
  int num_devices = Pa_GetDeviceCount();
  if (num_devices < 0)
  {
    fprintf(stderr, "Error: Pa_GetDeviceCount() returned %d\n", num_devices);
    exit(1);
  }

  std::vector<std::string> dev_names;
  for (PaDeviceIndex i=0; i<num_devices; i++)
  {
    const PaDeviceInfo* dev_info = Pa_GetDeviceInfo(i);
    if (!dev_info)
    {
      fprintf(stderr, "Error: Pa_GetDeviceInfo(%d) returned null\n", i);
      exit(1);
    }
    if (dev_info->maxInputChannels > 0)
      dev_names.push_back(std::to_string(i) + ") " + dev_info->name);
    else
      dev_names.push_back(kNotInputDev);
  }
  return dev_names;
}

std::vector<std::string> markChosenDevName(std::vector<std::string> names,
                                           int choice)
{
  std::vector<std::string> ret;
  for (int i=0; i<names.size(); i++)
  {
    std::string cur = "";
    if (choice == i)
    {
      cur = ">>> ";
      if (names[i] == kNotInputDev)
        cur += "NOT AN INPUT DEV?!?!?!? ";
    }
    ret.push_back(cur + names[i]);
  }
  return ret;
}




#ifdef CLICKITONGUE_WINDOWS
int askUserChoice(std::vector<std::string> dev_names, int default_dev_ind)
{
  int user_choice = default_dev_ind;
  while (true)
  {
    std::vector<std::string> marked = markChosenDevName(dev_names, user_choice);
    std::string prompt = "Is this the audio input device you want to use?\n\n\n\n";
    for (std::string x : marked)
      if (x != kNotInputDev)
        prompt += x + "\n\n";
    prompt += "\n\n(Clickitongue will remember your selection for the future. "
              "If you ever want to switch to another input device, delete the "
              "file audio_input_device.config).";
    if (promptYesNo(prompt.c_str()))
      break;
    do
    {
      user_choice++;
      if (user_choice >= dev_names.size())
        user_choice = 0;
    } while (dev_names[user_choice] == kNotInputDev);
  }
  return user_choice;
}
#else // linux or OSX
int askUserChoice(std::vector<std::string> dev_names, int default_dev_ind)
{
  int user_choice = default_dev_ind;
  printf("\nDevices:\n");
  for (std::string x : dev_names)
    if (x != kNotInputDev)
      printf("%s\n", x.c_str());
  printf(
"\nEnter the number of the input device you want to use.\n\n"
"(Clickitongue will remember your selection for the future. If you ever want\n"
"to switch to another input device, run clickitongue --forget_input_dev).\n\n");
  int scanf_num_read = 0;
  while (scanf_num_read != 1)
  {
    int highest_acceptable = dev_names.size() - 1;
    printf("If in doubt, enter %d. Enter [%d-%d]: ",
           default_dev_ind, 0, highest_acceptable);
    scanf_num_read = scanf("%d", &user_choice);
  }
  return user_choice;
}
#endif

bool g_forget_input_dev = false;
PaDeviceIndex chooseInputDevice()
{
  static PaDeviceIndex chosen = paNoDevice;
  if (chosen != paNoDevice)
    return chosen;

  PaDeviceIndex default_dev_ind = Pa_GetDefaultInputDevice();
  if (default_dev_ind == paNoDevice)
  {
    fprintf(stderr, "Error: No default audio input device.\n");
    exit(1);
  }
  chosen = default_dev_ind;
  std::vector<std::string> dev_names = getDeviceNames();

  if (!g_forget_input_dev)
  {
    std::optional<int> maybe_remembered = loadDeviceConfig(dev_names);
    if (maybe_remembered.has_value())
    {
      chosen = maybe_remembered.value();
      return chosen;
    }
  }

  int num_output_devs = 0;
  int any_output_dev_ind = -1;
  for (int i=0; i<dev_names.size(); i++)
    if (dev_names[i] != kNotInputDev)
    {
      num_output_devs++;
      any_output_dev_ind = i;
    }
  if (num_output_devs == 1)
  {
    chosen = any_output_dev_ind;
    return writeDeviceConfig(dev_names, chosen);
  }

  chosen = askUserChoice(dev_names, default_dev_ind);
  return writeDeviceConfig(dev_names, chosen);
}

AudioInput::AudioInput(int seconds_to_record, int frames_per_cb)
  : stream_(nullptr), data_(seconds_to_record * kFramesPerSec)
{
  ctorCommon(recordCallback, &data_, frames_per_cb);
}

AudioInput::AudioInput(int(*custom_record_cb)(const void*, void*, unsigned long,
                       const PaStreamCallbackTimeInfo*,
                       PaStreamCallbackFlags, void*), void* user_opaque,
                       int frames_per_cb)
  : stream_(nullptr), data_(0)
{
  ctorCommon(custom_record_cb, user_opaque, frames_per_cb);
}

void AudioInput::ctorCommon(int(*record_cb)(const void*, void*, unsigned long,
                            const PaStreamCallbackTimeInfo*,
                            PaStreamCallbackFlags, void*), void* opaque,
                            int frames_per_cb)
{
  PaError err = Pa_Initialize();
  if (err != paNoError) crash(err);

  PaStreamParameters input_param;
  input_param.device = chooseInputDevice();
  if (input_param.device == paNoDevice)
  {
    fprintf(stderr,"Error: No default input device.\n");
    Pa_Terminate();
    exit(1);
  }
  input_param.channelCount = kNumChannels;
  input_param.sampleFormat = paFloat32;
  input_param.suggestedLatency =
    Pa_GetDeviceInfo(input_param.device)->defaultLowInputLatency;
  input_param.hostApiSpecificStreamInfo = NULL;

  err = Pa_OpenStream(&stream_, &input_param, NULL, kFramesPerSec,
                      frames_per_cb,//.value_or(paFramesPerBufferUnspecified),
                      paClipOff, record_cb, opaque);
#ifdef CLICKITONGUE_LINUX
  PaAlsa_EnableRealtimeScheduling(stream_, 1);
#endif // CLICKITONGUE_LINUX

  if (err != paNoError) crash(err);
  err = Pa_StartStream(stream_);
  if (err != paNoError) crash(err);
}

AudioInput::~AudioInput()
{
  closeStream();
  Pa_Terminate();
}

void AudioInput::closeStream()
{
  if (stream_)
  {
    Pa_CloseStream(stream_);
    stream_ = nullptr;
  }
}

bool AudioInput::active() const
{
  if (stream_)
    return Pa_IsStreamActive(stream_) == 1;
  else
    return false;
}

std::vector<Sample> const& AudioInput::recordedSamples()
{
  return data_.samples;
}

long* AudioInput::frame_index_ptr()
{
  return data_.frame_index_ptr();
}

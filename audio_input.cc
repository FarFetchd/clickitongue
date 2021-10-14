#include "audio_input.h"

#include <cstdio>
#include <cstdlib>

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
  RecordingState* data_ptr = (RecordingState*)user_data;
  const Sample* cur_in = (const Sample*)input_buf;
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

AudioInput::AudioInput(int seconds_to_record, std::optional<int> frames_per_cb)
  : stream_(nullptr), data_(seconds_to_record * kFramesPerSec)
{
  ctorCommon(recordCallback, &data_, frames_per_cb);
}

AudioInput::AudioInput(int(*custom_record_cb)(const void*, void*, unsigned long,
                       const PaStreamCallbackTimeInfo*,
                       PaStreamCallbackFlags, void*), void* user_opaque,
                       std::optional<int> frames_per_cb)
  : stream_(nullptr), data_(0)
{
  ctorCommon(custom_record_cb, user_opaque, frames_per_cb);
}

void AudioInput::ctorCommon(int(*record_cb)(const void*, void*, unsigned long,
                            const PaStreamCallbackTimeInfo*,
                            PaStreamCallbackFlags, void*), void* opaque,
                            std::optional<int> frames_per_cb)
{
  PaError err = Pa_Initialize();
  if (err != paNoError) crash(err);

  PaStreamParameters input_param;
  input_param.device = Pa_GetDefaultInputDevice();
  if (input_param.device == paNoDevice)
  {
    fprintf(stderr,"Error: No default input device.\n");
    Pa_Terminate();
    exit(1);
  }
  input_param.channelCount = 2; // stereo input
  input_param.sampleFormat = paFloat32;
  input_param.suggestedLatency =
    Pa_GetDeviceInfo(input_param.device)->defaultLowInputLatency;
  input_param.hostApiSpecificStreamInfo = NULL;

  // TODO explore what adding paDitherOff to the flags here would do.
  err = Pa_OpenStream(&stream_, &input_param, NULL, kFramesPerSec,
                      frames_per_cb.value_or(paFramesPerBufferUnspecified),
                      paClipOff, record_cb, opaque);
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

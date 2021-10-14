#include "audio_output.h"

#include <cstdio>
#include <cstdlib>

#include "portaudio.h"

struct PlaybackCbStruct
{
  const std::vector<Sample>* samples;
  int playback_sample_index = 0;
};

int playCallback(const void* input_buf, void* output_buf,
                 unsigned long frames_wanted,
                 const PaStreamCallbackTimeInfo* time_info,
                 PaStreamCallbackFlags status_flags, void* user_data)
{
  PlaybackCbStruct* arg = (PlaybackCbStruct*)user_data;
  std::vector<Sample> const& samples = *(arg->samples);
  Sample* cur_out = (Sample*)output_buf;

  long samples_left = samples.size() - arg->playback_sample_index;
  long frames_left = samples_left / kNumChannels;
  long frames_to_write = frames_left > frames_wanted ? frames_wanted : frames_left;
  for (int i = 0; i < frames_to_write; i++)
  {
    *cur_out++ = samples[arg->playback_sample_index++];
    if (kNumChannels == 2)
      *cur_out++ = samples[arg->playback_sample_index++];
  }

  long final_silent_frames = frames_wanted - frames_to_write;
  for (int i = 0; i < final_silent_frames; i++)
  {
    *cur_out++ = 0;
    if (kNumChannels == 2)
      *cur_out++ = 0;
  }
  return frames_left > frames_wanted ? paContinue : paComplete;
}

void playRecorded(const std::vector<Sample>* samples)
{
  Pa_Initialize();

  PlaybackCbStruct arg;
  arg.samples = samples;
  PaStreamParameters output_param;

  output_param.device = Pa_GetDefaultOutputDevice();
  if (output_param.device == paNoDevice)
  {
    printf("Couldn't get a default output device!\n");
    exit(1);
  }
  output_param.channelCount = kNumChannels;
  output_param.sampleFormat = paFloat32;
  output_param.suggestedLatency =
    Pa_GetDeviceInfo(output_param.device)->defaultLowOutputLatency;
  output_param.hostApiSpecificStreamInfo = NULL;

  fflush(stdout);
  PaStream* stream;
  Pa_OpenStream(&stream, NULL, &output_param, kFramesPerSec,
                /*frames_per_buffer=*/512, paClipOff, playCallback, &arg);
  if (stream)
  {
    printf("now playing %g seconds of audio...\n",
           (samples->size() / kNumChannels) / (float)kFramesPerSec);
    Pa_StartStream(stream);
    while (Pa_IsStreamActive(stream) == 1)
      Pa_Sleep(100);
    Pa_CloseStream(stream);
    printf("...playback done.\n");
    fflush(stdout);
  }
  Pa_Terminate();
}

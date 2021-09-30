#ifndef CLICKITONGUE_AUDIO_INPUT_H_
#define CLICKITONGUE_AUDIO_INPUT_H_

#include <vector>
#include "portaudio.h"

#include "constants.h"

struct RecordingState
{
  RecordingState(long size_in_frames) : size_frames(size_in_frames)
  {
    samples.reserve(size_frames * kNumChannels);
  }
  long* frame_index_ptr()
  {
    return &frame_index;
  }

  // *frame-based* index into samples. So, if kNumChannels==2, frame_index 2 is
  // actually pointing to samples[4] (and 5).
  long frame_index = 0;
  // intended final size of samples, again specified in frames rather than samples.
  const long size_frames;
  std::vector<Sample> samples;
};

int recordCallback(const void* input_buf, void* output_buf,
                   unsigned long frame_count,
                   const PaStreamCallbackTimeInfo* time_info,
                   PaStreamCallbackFlags status_flags, void* user_data);

class AudioInput
{
public:
  // Call me for a nice straightforward recording of audio.
  AudioInput(int seconds_to_record);
  // Call me if you want an audio input stream, and want to do your own stuff with it.
  AudioInput(int(*custom_record_cb)(const void*, void*, unsigned long,
                                    const PaStreamCallbackTimeInfo*,
                                    PaStreamCallbackFlags, void*), void* user_opaque);

  void ctorCommon(int(*record_cb)(const void*, void*, unsigned long,
                                  const PaStreamCallbackTimeInfo*,
                                  PaStreamCallbackFlags, void*), void* opaque);

  ~AudioInput();

  bool active() const;

  // Access the recorded audio. Will be empty if you used the non-default ctor.
  // Perhaps ok to access while recording is ongoing, if you're ok with concurrent
  // reading+appending of an unprotected vector.
  std::vector<Sample> const& recordedSamples();

  // For querying current frame of ongoing recording.
  long* frame_index_ptr();

private:
  PaStream* stream_;
  RecordingState data_;
};

#endif // CLICKITONGUE_AUDIO_INPUT_H_

#ifndef CLICKITONGUE_AUDIO_RECORDING_H_
#define CLICKITONGUE_AUDIO_RECORDING_H_

#include <string>
#include <vector>

#include "blocking_queue.h"

// For saving/loading raw PCM files. Reads/writes files with big-endian uint16
// samples, but stores in memory as 32-bit float.
class AudioRecording
{
public:
  // Loads fname as raw big-endian uint16 PCM
  explicit AudioRecording(std::string fname);
  // Records 'seconds' of audio into samples_. (This ctor blocks until those
  // seconds of recording have finished).
  explicit AudioRecording(int seconds);
  // Record into samples_ until stop_recording fires, then writes a WAV file to fname.
  AudioRecording(PokeQueue* stop_recording, std::string fname, PokeQueue* wav_ready);

  // actual playback of samples_
  void play() const;
  // write samples_ to fname
  void recordToFile(std::string fname) const;
  void writeToWAVFile(std::string fname) const;
  // accessor
  std::vector<float> const& samples() const;

  // Overlays rhs onto our own data. We ignore its end if it's longer than we are.
  AudioRecording& operator+=(AudioRecording const& rhs);
  // Scale up or down by this factor.
  void scale(double factor);

  // atomic here is awkward for compilation elsewhere... i think this is fine;
  // worst case it records one frame longer than intended
  bool keep_recording_ = true;
  std::vector<float> samples_;
  std::vector<int16_t> hack_s16_samples_;
};

#endif // CLICKITONGUE_AUDIO_RECORDING_H_

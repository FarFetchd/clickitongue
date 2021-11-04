#ifndef CLICKITONGUE_AUDIO_RECORDING_H_
#define CLICKITONGUE_AUDIO_RECORDING_H_

#include <string>
#include <vector>

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

  // actual playback of samples_
  void play() const;
  // write samples_ to fname
  void recordToFile(std::string fname) const;
  // accessor
  std::vector<float> const& samples() const;

  // Overlays rhs onto our own data. We ignore its end if it's longer than we are.
  AudioRecording& operator+=(AudioRecording const& rhs);
  // Scale up or down by this factor. TODO There is probably some decibel thing
  // that is the more correct way to do this.
  void scale(double factor);

private:
  std::vector<float> samples_;
};

#endif // CLICKITONGUE_AUDIO_RECORDING_H_

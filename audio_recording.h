#ifndef CLICKITONGUE_AUDIO_RECORDING_H_
#define CLICKITONGUE_AUDIO_RECORDING_H_

#include <string>
#include <vector>

// For saving/loading raw PCM files.
class RecordedAudio
{
public:
  // Loads fname as raw float PCM
  explicit RecordedAudio(std::string fname);
  // Records 'seconds' of audio into samples_. (This ctor blocks until those
  // seconds of recording have finished).
  explicit RecordedAudio(int seconds);

  // actual playback of samples_
  void play() const;
  // write samples_ to fname
  void recordToFile(std::string fname) const;
  // accessor
  std::vector<float> const& samples() const;

  // Overlays rhs onto our own data. We ignore its end if it's longer than we are.
  RecordedAudio& operator+=(RecordedAudio const& rhs);

private:
  std::vector<float> samples_;
};

#endif // CLICKITONGUE_AUDIO_RECORDING_H_

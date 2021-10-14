#ifndef CLICKITONGUE_AUDIO_RECORDING_H_
#define CLICKITONGUE_AUDIO_RECORDING_H_

#include <string>
#include <vector>

void recordToFile(std::string fname, int seconds);

// For saving/loading raw PCM files.
class RecordedAudio
{
public:
  explicit RecordedAudio(std::string fname);
  void play() const;
  std::vector<float> const& samples() const;

  // Overlays rhs onto our own data. We ignore its end if it's longer than we are.
  RecordedAudio& operator+=(RecordedAudio const& rhs);

private:
  std::vector<float> samples_;
};

#endif // CLICKITONGUE_AUDIO_RECORDING_H_

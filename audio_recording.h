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

private:
  std::vector<float> samples_;
};

#endif // CLICKITONGUE_AUDIO_RECORDING_H_

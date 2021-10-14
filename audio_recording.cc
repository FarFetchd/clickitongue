#include "audio_recording.h"

#include <cassert>

#include "audio_input.h"
#include "audio_output.h"

void recordToFile(std::string fname, int seconds)
{
  AudioInput recorder(seconds);
  while (recorder.active())
    Pa_Sleep(100);
  FILE* writer = fopen(fname.c_str(), "wb");
  fwrite(recorder.recordedSamples().data(), sizeof(Sample),
         recorder.recordedSamples().size(), writer);
  fclose(writer);
  printf("Wrote %d seconds of float format %d channel %d Hz audio to %s.\n",
          seconds, kNumChannels, kFramesPerSec, fname.c_str());
}

RecordedAudio::RecordedAudio(std::string fname)
{
  FILE* reader = fopen(fname.c_str(), "rb");
  if (!reader)
  {
    fprintf(stderr, "could not open %s. crashing.\n", fname.c_str());
    exit(1);
  }
  fseek(reader, 0, SEEK_END);
  size_t flen = ftell(reader);
  rewind(reader);
  for (size_t bytes_read = 0; bytes_read < flen; bytes_read += sizeof(Sample))
  {
    Sample cur;
    assert(1 == fread(&cur, sizeof(Sample), 1, reader));
    samples_.push_back(cur);
  }
  fclose(reader);
}

void RecordedAudio::play() const
{
  playRecorded(&samples_);
}

std::vector<float> const& RecordedAudio::samples() const { return samples_; }

RecordedAudio& RecordedAudio::operator+=(RecordedAudio const& rhs)
{
  for (int i = 0; i < samples_.size() && i < rhs.samples().size(); i++)
    samples_[i] = rhs.samples()[i];
  return *this;
}

#include "audio_recording.h"

#include <cassert>

#include "audio_input.h"
#include "audio_output.h"

// (don't want to incude weird networking headers just to get htons)
bool isLittleEndian()
{
  int as_int = 1;
  char* as_bytes = reinterpret_cast<char*>(&as_int);
  return as_bytes[0] == 1;
}

RecordedAudio::RecordedAudio(std::string fname)
{
  bool little_endian = isLittleEndian();
  FILE* reader = fopen(fname.c_str(), "rb");
  if (!reader)
  {
    fprintf(stderr, "could not open %s. crashing.\n", fname.c_str());
    exit(1);
  }
  fseek(reader, 0, SEEK_END);
  size_t flen = ftell(reader);
  rewind(reader);
  for (size_t bytes_read = 0; bytes_read < flen; bytes_read += sizeof(uint16_t))
  {
    uint16_t cur16;
    assert(1 == fread(&cur16, sizeof(uint16_t), 1, reader));
    if (little_endian)
    {
      char* as_bytes = reinterpret_cast<char*>(&cur16);
      as_bytes[0] ^= as_bytes[1];
      as_bytes[1] ^= as_bytes[0];
      as_bytes[0] ^= as_bytes[1];
    }
    double temp = cur16;
    temp -= 32768.0;
    temp /= 32767.0;
    samples_.push_back((float)temp);
  }
  fclose(reader);
}

RecordedAudio::RecordedAudio(int seconds)
{
  AudioInput recorder(seconds, paFramesPerBufferUnspecified);
  while (recorder.active())
    Pa_Sleep(100);
  samples_ = recorder.recordedSamples();
}

void RecordedAudio::play() const
{
  playRecorded(&samples_);
}

std::vector<float> const& RecordedAudio::samples() const { return samples_; }

RecordedAudio& RecordedAudio::operator+=(RecordedAudio const& rhs)
{
  for (int i = 0; i < samples_.size() && i < rhs.samples().size(); i++)
    samples_[i] += rhs.samples()[i];
  return *this;
}

void RecordedAudio::scale(double factor)
{
  for (float& s : samples_)
    s *= factor;
}

void RecordedAudio::recordToFile(std::string fname) const
{
  bool little_endian = isLittleEndian();
  FILE* writer = fopen(fname.c_str(), "wb");
  for (float sample : samples_)
  {
    uint16_t sample16;
    double temp = ((double)sample) * 32767.0 + 32768.0;
    if (temp > 65534.5)
      sample16 = 65535;
    else if (temp < 0.5)
      sample16 = 0;
    else
      sample16 = (uint16_t)(((uint32_t)(temp*2.0))/2);
    if (little_endian)
    {
      char* as_bytes = reinterpret_cast<char*>(&sample16);
      as_bytes[0] ^= as_bytes[1];
      as_bytes[1] ^= as_bytes[0];
      as_bytes[0] ^= as_bytes[1];
    }
    fwrite(&sample16, sizeof(uint16_t), 1, writer);
  }
  fclose(writer);
  float seconds = (samples_.size() / kNumChannels) / (float)kFramesPerSec;
  printf("Wrote %g seconds of big-endian uint16 %d channel %d Hz audio to %s.\n",
         seconds, kNumChannels, kFramesPerSec, fname.c_str());
}

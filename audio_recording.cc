#include "audio_recording.h"

#include <cassert>
#include <cstring>
#include <thread>

#include "audio_input.h"
#include "audio_output.h"
#include "interaction.h"

void crash(const char* s);

// (don't want to incude weird networking headers just to get htons)
bool isLittleEndian()
{
  int as_int = 1;
  char* as_bytes = reinterpret_cast<char*>(&as_int);
  return as_bytes[0] == 1;
}

uint16_t swap16(uint16_t value)
{
  return (value << 8) | (value >> 8);
}

AudioRecording::AudioRecording(std::string fname)
{
  bool little_endian = isLittleEndian();
  FILE* reader = fopen(fname.c_str(), "rb");
  if (!reader)
  {
    std::string msg = "could not open "+fname+". crashing.";
    crash(msg.c_str());
  }
  fseek(reader, 0, SEEK_END);
  size_t flen = ftell(reader);
  rewind(reader);
  for (size_t bytes_read = 0; bytes_read < flen; bytes_read += sizeof(uint16_t))
  {
    uint16_t cur16;
    assert(1 == fread(&cur16, sizeof(uint16_t), 1, reader));
    if (little_endian)
      cur16 = swap16(cur16);
    double temp = cur16;
    temp -= 32768.0;
    temp /= 32767.0;
    samples_.push_back((float)temp);
  }
  fclose(reader);
}

AudioRecording::AudioRecording(int seconds)
{
  AudioInput recorder(seconds, paFramesPerBufferUnspecified);
  while (recorder.active())
    Pa_Sleep(100);
  samples_ = recorder.recordedSamples();
}

int recordIndefinitelyCallback(const void* input_buf, void* output_buf,
                               unsigned long frames_provided,
                               const PaStreamCallbackTimeInfo* time_info,
                               PaStreamCallbackFlags status_flags, void* user_data)
{
  AudioRecording* me = static_cast<AudioRecording*>(user_data);
  const int16_t* cur_in = static_cast<const int16_t*>(input_buf);
  std::vector<int16_t>* recorded_samples = &me->hack_s16_samples_;

  if (input_buf != NULL)
  {
    for (int i=0; i<frames_provided; i++)
    {
      recorded_samples->push_back(*cur_in++); // left or mono
      // if (g_num_channels == 2)          NOPE
      //   recorded_samples->push_back(*cur_in++); // right
    }
  }
  else
  {
//    PRINTF("got SILENCE\n");
    for (int i=0; i<frames_provided; i++)
    {
      recorded_samples->push_back(kSilentSample);  // left or mono
      // if (g_num_channels == 2)
      //   recorded_samples->push_back(kSilentSample);  // right
    }
  }
  return me->keep_recording_ ? paContinue : paComplete;
}

AudioRecording::AudioRecording(PokeQueue* stop_recording, std::string fname, PokeQueue* wav_ready)
{
  AudioRecording* me = this;
  std::thread whisper_record_thread([stop_recording, me, fname, wav_ready]()
  {
    AudioInput recorder(recordIndefinitelyCallback, me, paFramesPerBufferUnspecified, kWhisperFrameRate, paInt16, 1);
    stop_recording->consumePoke();
    me->keep_recording_ = false;
    while (recorder.active())
      Pa_Sleep(1);
    me->writeToWAVFile(fname);
    wav_ready->poke();
  });
  whisper_record_thread.detach();
}

void AudioRecording::play() const
{
  playRecorded(&samples_);
}

std::vector<float> const& AudioRecording::samples() const { return samples_; }

AudioRecording& AudioRecording::operator+=(AudioRecording const& rhs)
{
  for (int i = 0; i < samples_.size() && i < rhs.samples().size(); i++)
    samples_[i] += rhs.samples()[i];
  return *this;
}

void AudioRecording::scale(double factor)
{
  for (float& s : samples_)
    s *= factor;
}

void AudioRecording::recordToFile(std::string fname) const
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
      sample16 = swap16(sample16);
    fwrite(&sample16, sizeof(uint16_t), 1, writer);
  }
  fclose(writer);
  float seconds = (samples_.size() / g_num_channels) / (float)kFramesPerSec;
  PRINTF("Wrote %g seconds of big-endian uint16 %d channel %d Hz audio to %s.\n",
         seconds, g_num_channels, kFramesPerSec, fname.c_str());
}

bool writeS16ToFile(std::string filename, const std::vector<int16_t>& s16_samples)
{
    if (!isLittleEndian())
      crash("only little endian supported here for now");
    FILE* file = fopen(filename.c_str(), "wb");
    if (!file)
      crash((std::string("error opening file ") + filename).c_str());

    uint32_t num_samples = s16_samples.size();

    // WAV header
    const char riff[] = "RIFF";
    fwrite(riff, 1, 4, file);

    uint32_t chunk_size = 36 + num_samples * 2;
    fwrite(&chunk_size, 4, 1, file);

    const char wave[] = "WAVE";
    fwrite(wave, 1, 4, file);

    const char fmt[] = "fmt ";
    fwrite(fmt, 1, 4, file);

    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, file);

    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, file);

    uint16_t num_channels = 1;
    fwrite(&num_channels, 2, 1, file);

    uint32_t weird = kWhisperFrameRate;
    fwrite(&weird, 4, 1, file);

    uint32_t byte_rate = kWhisperFrameRate * 2;
    fwrite(&byte_rate, 4, 1, file);

    uint16_t block_align = 2;
    fwrite(&block_align, 2, 1, file);

    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, file);

    const char data[] = "data";
    fwrite(data, 1, 4, file);

    uint32_t data_size = num_samples * 2;
    fwrite(&data_size, 4, 1, file);

    fwrite(s16_samples.data(), 2, s16_samples.size(), file);

    fclose(file);
    return true;
}

void AudioRecording::writeToWAVFile(std::string fname) const
{
  writeS16ToFile(fname, hack_s16_samples_);
}

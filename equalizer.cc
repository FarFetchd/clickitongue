#include "equalizer.h"

#include <fftw3.h>

#include "constants.h"

void printEqualizer(const float* samples, int num_frames)
{
  ComplexDouble* transformed = new ComplexDouble[num_frames / 2 + 1];
  double orig_real[num_frames];
  for (int i=0; i<num_frames; i++)
    orig_real[i] = samples[i * kNumChannels];

  fftw_plan fftw_forward =
    fftw_plan_dft_r2c_1d(num_frames, orig_real,
                         reinterpret_cast<fftw_complex*>(transformed),
                         FFTW_ESTIMATE); // TODO wisdom
  fftw_execute(fftw_forward);
  fftw_destroy_plan(fftw_forward);

  printEqualizerAlreadyFreq(transformed, num_frames);
  delete[] transformed;
}

void printEqualizerAlreadyFreq(ComplexDouble* freq_buckets, int orig_num_frames)
{
  constexpr int kMaxHeight = 50;
  const int columns = orig_num_frames / 2 + 1; // including \n char at end
  char* bars = new char[columns * kMaxHeight + 1];
  for (int height = kMaxHeight; height > 0; height--)
  {
    int y = kMaxHeight - height;
    for (int x = 0; x < orig_num_frames / 2; x++)
      if (abs(freq_buckets[x].real()) > height)
        bars[columns * y + x] = '0';
      else
        bars[columns * y + x] = ' ';
    bars[columns * y + orig_num_frames / 2] = '\n';
  }
  printf("\e[1;1H\e[2J");
  bars[columns * kMaxHeight] = 0;
  printf("%s", bars);
  delete[] bars;
}

void printMaxBucket(const float* samples, int num_frames)
{
  ComplexDouble* transformed = new ComplexDouble[num_frames / 2 + 1];
  double orig_real[num_frames];
  for (int i=0; i<num_frames; i++)
    orig_real[i] = samples[i * kNumChannels];

  fftw_plan fftw_forward =
    fftw_plan_dft_r2c_1d(num_frames, orig_real,
                         reinterpret_cast<fftw_complex*>(transformed),
                         FFTW_ESTIMATE);
  fftw_execute(fftw_forward);
  fftw_destroy_plan(fftw_forward);

  double max = 0;
  int max_ind = 0;
  for (int i = 0; i < num_frames / 2 + 1; i++)
  {
    if (abs(transformed[i].real()) > max)
    {
      max = abs(transformed[i].real());
      max_ind = i;
    }
  }
  float fraction = (float)max_ind / (float)(num_frames / 2 + 1);
  printf("%g: %g\n", fraction * 44100, max);

  delete[] transformed;
}

#include "equalizer.h"

#include <complex>
#include <fftw3.h>
using ComplexDouble = std::complex<double>;

void printEqualizer(const float* samples, int num_frames, int num_channels)
{
  ComplexDouble* transformed = new ComplexDouble[num_frames / 2 + 1];
  double orig_real[num_frames];
  for (int i=0; i<num_frames; i++)
    orig_real[i] = samples[i * num_channels];

  fftw_plan fftw_forward = fftw_plan_dft_r2c_1d(num_frames, orig_real, reinterpret_cast<fftw_complex*>(transformed), FFTW_ESTIMATE);
  fftw_execute(fftw_forward);
  fftw_destroy_plan(fftw_forward);

  constexpr int kMaxHeight = 50;
  const int columns = num_frames / 2 + 1; // including \n char at end
  char* bars = new char[columns * kMaxHeight + 1];
  for (int height = kMaxHeight; height > 0; height--)
  {
    int y = kMaxHeight - height;
    for (int x = 0; x < num_frames / 2; x++)
      if (abs(transformed[x].real()) > height)
        bars[columns * y + x] = '0';
      else
        bars[columns * y + x] = ' ';
    bars[columns * y + num_frames / 2] = '\n';
  }
  printf("\e[1;1H\e[2J");
  bars[columns * kMaxHeight] = 0;
  printf("%s", bars);
  delete[] transformed;
  delete[] bars;
}

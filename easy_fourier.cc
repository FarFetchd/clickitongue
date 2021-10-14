#include "easy_fourier.h"

#include <cmath>

#include "constants.h"

double* makeBinFreqs(int blocksize, double bin_width)
{
  double* bin_freq = new double[blocksize];
  bin_freq[0] = 0;
  for (int i = 0; i < blocksize / 2 + 1; i++)
    bin_freq[i] = bin_freq[i-1] + bin_width;
  return bin_freq;
}

EasyFourier::EasyFourier(int blocksize)
  : blocksize_(blocksize), bin_width_(kNyquist / ((double)(blocksize_ / 2 + 1))),
    half_width_(bin_width_ / 2.0), bin_freq_(makeBinFreqs(blocksize_, bin_width_)) {}

EasyFourier::~EasyFourier()
{
  delete[] bin_freq_;
}

void EasyFourier::doFFT(double* input, fftw_complex* output) const
{
  fftw_plan fftw_forward =
    fftw_plan_dft_r2c_1d(blocksize_, input, output, FFTW_ESTIMATE); // TODO wisdom
  fftw_execute(fftw_forward);
  fftw_destroy_plan(fftw_forward);
}

double EasyFourier::freqOfBin(int index) const { return bin_freq_[index]; }

double EasyFourier::binWidth() const { return bin_width_; }
double EasyFourier::halfWidth() const { return half_width_; }

void EasyFourier::printEqualizer(const float* samples) const
{
  fftw_complex transformed[blocksize_ / 2 + 1];
  double orig_real[blocksize_];
  for (int i=0; i<blocksize_; i++)
    orig_real[i] = samples[i * kNumChannels];

  fftw_plan fftw_forward =
    fftw_plan_dft_r2c_1d(blocksize_, orig_real, transformed, FFTW_ESTIMATE); // TODO wisdom
  fftw_execute(fftw_forward);
  fftw_destroy_plan(fftw_forward);

  printEqualizerAlreadyFreq(transformed);
}

void EasyFourier::printEqualizerAlreadyFreq(fftw_complex* freq_buckets) const
{
  constexpr int kMaxHeight = 50;
  const int columns = blocksize_ / 2 + 1; // including \n char at end
  char bars[columns * kMaxHeight + 1];
  for (int height = kMaxHeight; height > 0; height--)
  {
    int y = kMaxHeight - height;
    for (int x = 0; x < blocksize_ / 2; x++)
      if (fabs(freq_buckets[x][0]) > height)
        bars[columns * y + x] = '0';
      else
        bars[columns * y + x] = ' ';
    bars[columns * y + blocksize_ / 2] = '\n';
  }
  printf("\e[1;1H\e[2J");
  bars[columns * kMaxHeight] = 0;
  printf("%s", bars);
}

void EasyFourier::printMaxBucket(const float* samples) const
{
  fftw_complex transformed[blocksize_ / 2 + 1];
  double orig_real[blocksize_];
  for (int i=0; i<blocksize_; i++)
    orig_real[i] = samples[i * kNumChannels];

  fftw_plan fftw_forward =
    fftw_plan_dft_r2c_1d(blocksize_, orig_real, transformed, FFTW_ESTIMATE);
  fftw_execute(fftw_forward);
  fftw_destroy_plan(fftw_forward);

  double max = 0;
  int max_ind = 0;
  for (int i = 0; i < blocksize_ / 2 + 1; i++)
  {
    if (fabs(transformed[i][0]) > max)
    {
      max = fabs(transformed[i][0]);
      max_ind = i;
    }
  }
  float fraction = (float)max_ind / (float)(blocksize_ / 2 + 1);
  printf("%g: %g\n", fraction * 22050, max);
}


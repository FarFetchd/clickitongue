#ifndef CLICKITONGUE_EASY_FOURIER_H_
#define CLICKITONGUE_EASY_FOURIER_H_

#include <fftw3.h>

// Everything in here assumes 44.1KHz audio.
class EasyFourier
{
public:
  explicit EasyFourier(int blocksize);
  ~EasyFourier();

  void doFFT(double* input, fftw_complex* output) const;

  // Returns the frequency center of the provided bin index.
  // orig_input_len should be the number of frames given to the FFT.
  // Must have 0 < bin_index <= orig_input_len/2 + 1.
  double freqOfBin(int index) const;

  // In Hz, the difference between two adjacent bin centers.
  double binWidth() const;
  // 0.5 * binWidth
  double halfWidth() const;

  void printEqualizer(const float* samples) const;

  // length of freq_buckets should be orig_num_frames / 2 + 1
  // (i.e. the output of a real-to-complex FFT with length orig_num_frames input)
  void printEqualizerAlreadyFreq(fftw_complex* freq_buckets) const;

  // prints the frequency bucket with the most energy, and its energy.
  void printMaxBucket(const float* samples) const;

private:
  const int blocksize_;
  const double bin_width_;
  const double half_width_;
  const double* const bin_freq_;
};

#endif // CLICKITONGUE_EASY_FOURIER_H_

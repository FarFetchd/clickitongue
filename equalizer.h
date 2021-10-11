#ifndef CLICKITONGUE_EQUALIZER_H_
#define CLICKITONGUE_EQUALIZER_H_

#include <complex>
using ComplexDouble = std::complex<double>;

void printEqualizer(const float* samples, int num_frames);

// length of freq_buckets should be orig_num_frames / 2 + 1
// (i.e. the output of a real-to-complex FFT with length orig_num_frames input)
void printEqualizerAlreadyFreq(ComplexDouble* freq_buckets, int orig_num_frames);

#endif // CLICKITONGUE_EQUALIZER_H_

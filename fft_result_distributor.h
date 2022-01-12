#ifndef CLICKITONGUE_FFT_RESULT_DISTRIBUTOR_H_
#define CLICKITONGUE_FFT_RESULT_DISTRIBUTOR_H_

#include <optional>

#include "portaudio.h"

#include "detector.h"
#include "easy_fourier.h"
#include "constants.h"

// PortAudio has, through a callback, given us a block of samples. Do a single
// FFT and pass the same result to all detectors present.
class FFTResultDistributor
{
public:
  FFTResultDistributor(std::vector<std::unique_ptr<Detector>>&& detectors,
                       bool training);

  void processAudio(const Sample* cur_sample, int num_frames);

private:
  std::vector<std::unique_ptr<Detector>> detectors_;
  FourierLease fft_lease_;
  // Whether these FFTs are being done on pre-recorded data, for training.
  const bool training_;

  std::vector<double> detector_scales_;
  bool all_same_scale_;
  double single_scale_ = -123.456;
};

int fftDistributorCallback(const void* input, void* output,
                           unsigned long num_frames,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags, void* user_data);

#endif // CLICKITONGUE_FFT_RESULT_DISTRIBUTOR_H_

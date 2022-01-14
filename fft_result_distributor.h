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
                       double scale, bool training);

  void processAudio(const Sample* cur_sample, int num_frames);

  void replaceDetectors(std::vector<std::unique_ptr<Detector>>&& detectors);

private:
  std::vector<std::unique_ptr<Detector>> detectors_;
  FourierLease fft_lease_;
  const double scale_;
  // Whether these FFTs are being done on pre-recorded data, for training.
  const bool training_;
};

int fftDistributorCallback(const void* input, void* output,
                           unsigned long num_frames,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags, void* user_data);

#endif // CLICKITONGUE_FFT_RESULT_DISTRIBUTOR_H_

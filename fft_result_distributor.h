#ifndef CLICKITONGUE_FFT_RESULT_DISTRIBUTOR_H_
#define CLICKITONGUE_FFT_RESULT_DISTRIBUTOR_H_

#include <optional>

#include "portaudio.h"

#include "blow_detector.h"
#include "easy_fourier.h"
#include "tongue_detector.h"
#include "constants.h"

// PortAudio has, through a callback, given us a block of samples. Do a single
// FFT and pass the same result to both the tongue detector and blow detector.
class FFTResultDistributor
{
public:
  FFTResultDistributor(std::optional<BlowDetector> blow_detector,
                       std::optional<TongueDetector> tongue_detector);

  void processAudio(const Sample* cur_sample, int num_frames);

private:
  std::optional<BlowDetector> blow_detector_;
  std::optional<TongueDetector> tongue_detector_;
  FourierLease fft_lease_;
};

int fftDistributorCallback(const void* input, void* output,
                           unsigned long num_frames,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags, void* user_data);

#endif // CLICKITONGUE_FFT_RESULT_DISTRIBUTOR_H_

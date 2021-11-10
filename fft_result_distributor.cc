#include "fft_result_distributor.h"

#include <cstdio>

FFTResultDistributor::FFTResultDistributor(std::optional<BlowDetector> blow_detector,
                                           std::optional<TongueDetector> tongue_detector)
  : blow_detector_(blow_detector), tongue_detector_(tongue_detector),
    fft_lease_(g_fourier->borrowWorker())
{
  if (blow_detector_.has_value() && tongue_detector_.has_value())
  {
    blow_detector_->set_tongue_link(&tongue_detector_.value());
    tongue_detector_->set_wait_for_blow(true);
  }
}

void FFTResultDistributor::processAudio(const Sample* cur_sample, int num_frames)
{
  if (num_frames != kFourierBlocksize)
  {
    printf("illegal num_frames: expected %d, got %d\n", kFourierBlocksize, num_frames);
    exit(1);
  }

  for (int i=0; i<kFourierBlocksize; i++)
    fft_lease_.in[i] = cur_sample[i * kNumChannels];
  fft_lease_.runFFT();

  if (blow_detector_.has_value())
    blow_detector_->processFourier(fft_lease_.out);
  if (tongue_detector_.has_value())
    tongue_detector_->processFourier(fft_lease_.out);
}


int fftDistributorCallback(const void* input, void* output,
                           unsigned long num_frames,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags, void* user_data)
{
  FFTResultDistributor* distrib = static_cast<FFTResultDistributor*>(user_data);
  const Sample* cur_samples = static_cast<const Sample*>(input);
  if (cur_samples)
    distrib->processAudio(cur_samples, num_frames);
  return paContinue;
}

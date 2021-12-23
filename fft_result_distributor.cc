#include "fft_result_distributor.h"

#include <cstdio>

FFTResultDistributor::FFTResultDistributor(
    std::vector<std::unique_ptr<Detector>>&& detectors,
    double scale)
: detectors_(std::move(detectors)),
  fft_lease_(g_fourier->borrowWorker()),
  scale_(scale)
{}

void FFTResultDistributor::processAudio(const Sample* cur_sample, int num_frames)
{
  if (num_frames != kFourierBlocksize)
  {
    printf("illegal num_frames: expected %d, got %d\n", kFourierBlocksize, num_frames);
    exit(1);
  }

  for (int i=0; i<kFourierBlocksize; i++)
  {
    if (kNumChannels == 2)
      fft_lease_.in[i] = (cur_sample[i*kNumChannels] + cur_sample[i*kNumChannels+1]) / 2.0;
    else
      fft_lease_.in[i] = cur_sample[i];
  }
  fft_lease_.runFFT();

  for (int i=0; i<kNumFourierBins; i++)
  {
    fft_lease_.out[i][0] = scale_ * (fft_lease_.out[i][0]*fft_lease_.out[i][0] +
                                     fft_lease_.out[i][1]*fft_lease_.out[i][1]);
  }
  for (auto& detector : detectors_)
    detector->processFourierOutputBlock(fft_lease_.out);
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

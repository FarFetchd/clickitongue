#include "fft_result_distributor.h"

#include <cstdio>
#include <cstring>
#include <unordered_map>

void safelyExit(int exit_code);

FFTResultDistributor::FFTResultDistributor(
    std::vector<std::unique_ptr<Detector>>&& detectors, bool training)
: detectors_(std::move(detectors)), fft_lease_(g_fourier->borrowWorker()),
  training_(training)
{
  for (auto const& detector : detectors_)
    detector_scales_.push_back(detector->scale());

  all_same_scale_ = true;
  double any_scale = detector_scales_.front();
  for (double s : detector_scales_)
    if (s != any_scale)
      all_same_scale_ = false;
  if (all_same_scale_)
    single_scale_ = any_scale;
}

class ScaleApplicator
{
public:
  ScaleApplicator(std::vector<double> scales)
  {
    scales_ = scales;
    for (double s : scales_)
    {
      if (scaled_out_.find(s) == scaled_out_.end())
      {
        fftw_complex* thing = fftw_alloc_complex(kNumFourierBins);
        scaled_out_[s] = thing;
      }
    }
  }

  void compute(fftw_complex* out)
  {
    for (int i=0; i<kNumFourierBins; i++)
      out[i][0] = out[i][0]*out[i][0] + out[i][1]*out[i][1];
    for (double s : scales_)
    {
      fftw_complex* cur_out = scaled_out_.find(s)->second;
      memcpy(cur_out, out, sizeof(fftw_complex) * kNumFourierBins);
      for (int i=0; i<kNumFourierBins; i++)
        cur_out[i][0] *= s;
    }
  }

  fftw_complex* getScaleAppliedPower(double scale) { return scaled_out_[scale]; }

  ~ScaleApplicator()
  {
    for (auto [junk, ptr] : scaled_out_)
      fftw_free(ptr);
  }

private:
  std::vector<double> scales_;
  std::unordered_map<double, fftw_complex*> scaled_out_;
};

bool g_show_debug_info = false;
void FFTResultDistributor::processAudio(const Sample* cur_sample, int num_frames)
{
  if (num_frames != kFourierBlocksize)
  {
    printf("illegal num_frames: expected %d, got %d\n", kFourierBlocksize, num_frames);
    safelyExit(1);
  }

  for (int i=0; i<kFourierBlocksize; i++)
  {
    if (g_num_channels == 2)
      fft_lease_.in[i] = (cur_sample[i*g_num_channels] + cur_sample[i*g_num_channels+1]) / 2.0;
    else
      fft_lease_.in[i] = cur_sample[i];
  }
  fft_lease_.runFFT();

  if (all_same_scale_)
  {
    for (int i=0; i<kNumFourierBins; i++)
    {
      fft_lease_.out[i][0] = single_scale_ *
                                 (fft_lease_.out[i][0]*fft_lease_.out[i][0] +
                                  fft_lease_.out[i][1]*fft_lease_.out[i][1]);
    }
    for (auto& detector : detectors_)
      detector->processFourierOutputBlock(fft_lease_.out);
    if (g_show_debug_info && !training_)
      g_fourier->printOctavesAlreadyFreq(fft_lease_.out);
  }
  else
  {
    static ScaleApplicator scaler_(detector_scales_);
    scaler_.compute(fft_lease_.out);
    for (auto& detector : detectors_)
    {
      fftw_complex* out = scaler_.getScaleAppliedPower(detector->scale());
      detector->processFourierOutputBlock(out);
    }
    if (g_show_debug_info && !training_)
    {
      g_fourier->printOctavesAlreadyFreq(scaler_.getScaleAppliedPower(
          detectors_.front()->scale()));
    }
  }
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

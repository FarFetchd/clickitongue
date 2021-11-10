#include "tongue_detector.h"

#include <cassert>
#include <cmath>

#include "easy_fourier.h"

TongueDetector::TongueDetector(BlockingQueue<Action>* action_queue,
                               double tongue_low_hz, double tongue_high_hz,
                               double tongue_hzenergy_high,
                               double tongue_hzenergy_low,
                               double tongue_min_spikes_freq_frac,
                               double tongue_high_spike_frac,
                               double tongue_high_spike_level,
                               std::vector<int>* cur_frame_dest)
  : Detector(action_queue), action_(Action::RecordCurFrame),
    tongue_low_hz_(tongue_low_hz), tongue_high_hz_(tongue_high_hz),
    tongue_hzenergy_high_(tongue_hzenergy_high),
    tongue_hzenergy_low_(tongue_hzenergy_low),
    tongue_min_spikes_freq_frac_(tongue_min_spikes_freq_frac),
    tongue_high_spike_frac_(tongue_high_spike_frac),
    tongue_high_spike_level_(tongue_high_spike_level)
{
  setCurFrameDest(cur_frame_dest);
  setCurFrameSource(&cur_frame_);
  track_cur_frame_ = true;
}

TongueDetector::TongueDetector(BlockingQueue<Action>* action_queue, Action action,
                               double tongue_low_hz, double tongue_high_hz,
                               double tongue_hzenergy_high, double tongue_hzenergy_low,
                               double tongue_min_spikes_freq_frac,
                               double tongue_high_spike_frac,
                               double tongue_high_spike_level)
  : Detector(action_queue), action_(action),
    tongue_low_hz_(tongue_low_hz), tongue_high_hz_(tongue_high_hz),
    tongue_hzenergy_high_(tongue_hzenergy_high),
    tongue_hzenergy_low_(tongue_hzenergy_low),
    tongue_min_spikes_freq_frac_(tongue_min_spikes_freq_frac),
    tongue_high_spike_frac_(tongue_high_spike_frac),
    tongue_high_spike_level_(tongue_high_spike_level)
{
  assert(action_ != Action::RecordCurFrame);
}

void TongueDetector::processAudio(const Sample* cur_sample, int num_frames)
{
  if (track_cur_frame_)
    cur_frame_ += num_frames;

  FourierLease lease = g_fourier->borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
    lease.in[i] = cur_sample[i * kNumChannels];
  lease.runFFT();
  processFourier(lease.out);
}

void TongueDetector::processFourier(const fftw_complex* fft_bins)
{
  int bin_ind = 1;
  while (tongue_low_hz_ > g_fourier->freqOfBin(bin_ind) + g_fourier->halfWidth())
    bin_ind++;
  int low_bin_ind = bin_ind;
  while (tongue_high_hz_ < g_fourier->freqOfBin(bin_ind) - g_fourier->halfWidth())
    bin_ind++;
  int high_bin_ind = bin_ind;

  double energy = 0;
  for (int i = low_bin_ind + 1; i < high_bin_ind; i++)
    energy += g_fourier->binWidth() * fabs(fft_bins[i][0]);
  energy += (g_fourier->freqOfBin(low_bin_ind) + g_fourier->halfWidth() - tongue_low_hz_)
            * fabs(fft_bins[low_bin_ind][0]);
  energy += (tongue_high_hz_ - (g_fourier->freqOfBin(high_bin_ind) - g_fourier->halfWidth()))
            * fabs(fft_bins[high_bin_ind][0]);

  const int num_buckets = kFourierBlocksize / 2 + 1;
  int first_high_bucket = round(tongue_min_spikes_freq_frac_ * num_buckets);
  double avg_high = 0;
  int spike_count = 0;
  for (int i = first_high_bucket; i < num_buckets; i++)
  {
    double val = fabs(fft_bins[i][0]);
    avg_high += val;
    if (val > tongue_high_spike_level_)
      spike_count++;
  }
  avg_high /= (double)(num_buckets - first_high_bucket);

  bool too_many_high_spikes = (double)spike_count /
             (double)(num_buckets - first_high_bucket) > tongue_high_spike_frac_;
  bool too_high = avg_high > 1;

//   if (too_many_high_spikes) // TODO trim
//     printf("too many spikes\n");
//   if (too_high)
//     printf("too high\n");

  if (too_many_high_spikes || too_high)
    refrac_blocks_left_ = kRefracBlocks;
  else if (refrac_blocks_left_ > 0)
  {
    if (energy < tongue_hzenergy_low_)
      refrac_blocks_left_--;
  }
  else if (energy > tongue_hzenergy_high_)
  {
    kickoffAction(action_);
    refrac_blocks_left_ = kRefracBlocks;
  }
}

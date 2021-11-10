#include "blow_detector.h"

#include <cassert>
#include <cmath>

#include "easy_fourier.h"

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           double lowpass_percent, double highpass_percent,
                           double low_on_thresh, double low_off_thresh,
                           double high_on_thresh, double high_off_thresh,
                           double high_spike_frac, double high_spike_level,
                           std::vector<int>* cur_frame_dest)
  : Detector(action_queue),
    action_on_(Action::RecordCurFrame), action_off_(Action::NoAction),
    lowpass_percent_(lowpass_percent), highpass_percent_(highpass_percent),
    low_on_thresh_(low_on_thresh), low_off_thresh_(low_off_thresh),
    high_on_thresh_(high_on_thresh), high_off_thresh_(high_off_thresh),
    high_spike_frac_(high_spike_frac), high_spike_level_(high_spike_level)
{
  setCurFrameDest(cur_frame_dest);
  setCurFrameSource(&cur_frame_);
  track_cur_frame_ = true;
}

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           Action action_on, Action action_off,
                           double lowpass_percent, double highpass_percent,
                           double low_on_thresh, double low_off_thresh,
                           double high_on_thresh, double high_off_thresh,
                           double high_spike_frac, double high_spike_level)
  : Detector(action_queue), action_on_(action_on), action_off_(action_off),
    lowpass_percent_(lowpass_percent), highpass_percent_(highpass_percent),
    low_on_thresh_(low_on_thresh), low_off_thresh_(low_off_thresh),
    high_on_thresh_(high_on_thresh), high_off_thresh_(high_off_thresh),
    high_spike_frac_(high_spike_frac), high_spike_level_(high_spike_level)
{
  assert(action_on_ != Action::RecordCurFrame);
  assert(action_off_ != Action::RecordCurFrame);
}

void BlowDetector::processAudio(const Sample* cur_sample, int num_frames)
{
  if (track_cur_frame_)
    cur_frame_ += kFourierBlocksize;

  FourierLease lease = g_fourier->borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
    lease.in[i] = cur_sample[i * kNumChannels];
  lease.runFFT();
  processFourier(lease.out);
}

void BlowDetector::processFourier(const fftw_complex* fft_bins)
{
  const int num_buckets = kFourierBlocksize / 2 + 1;
  int last_low_bucket = round(lowpass_percent_ * num_buckets);
  double avg_low = 0;
  for (int i = 1; i <= last_low_bucket; i++)
    avg_low += fabs(fft_bins[i][0]);
  avg_low /= (double)(last_low_bucket + 1);

  int first_high_bucket = round(highpass_percent_ * num_buckets);
  double avg_high = 0;
  int spike_count = 0;
  for (int i = first_high_bucket; i < num_buckets; i++)
  {
    double val = fabs(fft_bins[i][0]);
    avg_high += val;
    if (val > high_spike_level_)
      spike_count++;
  }
  avg_high /= (double)(num_buckets - first_high_bucket);

  bool many_high_spikes = (double)spike_count /
             (double)(num_buckets - first_high_bucket) > high_spike_frac_;

  // require all clicks to last at least two blocks (at 256 frames per block,
  // 1 block is 5.8ms, and my physical mouse clicks appear to be 50-80ms long).
  if (mouse_down_ && !mouse_down_at_least_one_block_)
    mouse_down_at_least_one_block_ = true;
  else if (refrac_blocks_left_ == 0 && avg_low > low_on_thresh_ && avg_high > high_on_thresh_ && many_high_spikes && !mouse_down_)
  {
    kickoffAction(action_on_);
    refrac_blocks_left_ = kRefracBlocks;
    mouse_down_ = true;
    mouse_down_at_least_one_block_ = false;
  }
  else if (avg_low < low_off_thresh_ && avg_high < high_off_thresh_)
  {
    if (mouse_down_)
    {
      kickoffAction(action_off_);
      mouse_down_ = false;
    }
    if (refrac_blocks_left_ > 0)
      refrac_blocks_left_--;
  }
}

#include "blow_detector.h"

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           double o1_on_thresh, double o7_on_thresh,
                           double o7_off_thresh, int lookback_blocks,
                           bool require_delay,
                           std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
    o1_on_thresh_(o1_on_thresh),
    o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
    lookback_blocks_(lookback_blocks), require_delay_(require_delay)
{}

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           Action action_on, Action action_off,
                           double o1_on_thresh, double o7_on_thresh,
                           double o7_off_thresh, int lookback_blocks,
                           bool require_delay)
  : Detector(action_on, action_off, action_queue),
    o1_on_thresh_(o1_on_thresh),
    o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
    lookback_blocks_(lookback_blocks), require_delay_(require_delay)
{}

void updateMemory(double cur, double thresh, int* since)
{
  if (cur > thresh)
    *since = 0;
  else
  {
    ++*since;
    if (*since > kForeverBlocksAgo)
      *since = kForeverBlocksAgo;
  }
}

void BlowDetector::updateElevatedThreshs()
{
  blocks_since_event_ = 0;

  o1_elevated_thresh_ = 0;
  for (int i=0; i<10; i++)
    if (o1_recents_[i] > o1_elevated_thresh_)
      o1_elevated_thresh_ = o1_recents_[i];
  o1_elevated_thresh_ *= 0.9;
  o7_elevated_thresh_ = 0;
  for (int i=0; i<10; i++)
    if (o7_recents_[i] > o7_elevated_thresh_)
      o7_elevated_thresh_ = o7_recents_[i];
  o7_elevated_thresh_ *= 0.9;
}

void BlowDetector::updateState(const fftw_complex* freq_power)
{
  o1_cur_ = freq_power[1][0];
  o1_recents_[o1_recent_ind_] = o1_cur_;
  if (++o1_recent_ind_ >= 10)
    o1_recent_ind_ = 0;

  o7_cur_ = 0;
  for (int i=64; i<128; i++)
    o7_cur_ += freq_power[i][0];
  o7_recents_[o7_recent_ind_] = o7_cur_;
  if (++o7_recent_ind_ >= 10)
    o7_recent_ind_ = 0;

  double cur_o1_on_thresh = o1_on_thresh_;
  double cur_o7_on_thresh = o7_on_thresh_;
  if (blocks_since_event_ < 30)
  {
    double elevated_frac = (30.0 - (double)blocks_since_event_) / 30.0;
    double standard_frac = 1.0 - elevated_frac;
    // actually, going to allow these even if they are lower than the configured thresholds.
    //if (o1_elevated_thresh_ > o1_on_thresh_)
      cur_o1_on_thresh = standard_frac * o1_on_thresh_ + elevated_frac * o1_elevated_thresh_;
    //if (o7_elevated_thresh_ > o7_on_thresh_)
      cur_o7_on_thresh = standard_frac * o7_on_thresh_ + elevated_frac * o7_elevated_thresh_;
  }

  if (blocks_since_event_ < 30)
    blocks_since_event_++;

  updateMemory(o1_cur_, cur_o1_on_thresh, &blocks_since_1above_);
  updateMemory(o7_cur_, cur_o7_on_thresh, &blocks_since_7above_);
}

bool BlowDetector::shouldTransitionOn()
{
  bool activated = (blocks_since_1above_ <= lookback_blocks_ &&
                    blocks_since_7above_ <= lookback_blocks_);
  if (delay_blocks_left_ < 0 && activated)
  {
    delay_blocks_left_ = kBlowDelayBlocks;
  }
  if (delay_blocks_left_ > 0 && (!require_delay_ || --delay_blocks_left_ == 0))
  {
    delay_blocks_left_ = -1;
    updateElevatedThreshs();
    return true;
  }
  return false;
}

bool BlowDetector::shouldTransitionOff()
{
  bool under_threshs = o7_cur_ < o7_off_thresh_;

  if (!under_threshs)
  {
    deactivate_warmup_blocks_left_ = kBlowDeactivateWarmupBlocks;
    return false;
  }
  if (--deactivate_warmup_blocks_left_ <= 0)
  {
    deactivate_warmup_blocks_left_ = kBlowDeactivateWarmupBlocks;
    updateElevatedThreshs();
    return true;
  }
  return false;
}

int BlowDetector::refracPeriodLengthBlocks() const { return 10; }

void BlowDetector::resetEWMAs()
{
  blocks_since_1above_ = kForeverBlocksAgo;
  blocks_since_7above_ = kForeverBlocksAgo;
  delay_blocks_left_ = -1;
  deactivate_warmup_blocks_left_ = -1;
  updateElevatedThreshs();
}

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

void BlowDetector::updateState(const fftw_complex* freq_power)
{
  o1_cur_ = freq_power[1][0];

  o7_cur_ = 0;
  for (int i=64; i<128; i++)
    o7_cur_ += freq_power[i][0];

  updateMemory(o1_cur_, o1_on_thresh_, &blocks_since_1above_);
  updateMemory(o7_cur_, o7_on_thresh_, &blocks_since_7above_);
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
    return true;
  }
  return false;
}

int BlowDetector::refracPeriodLengthBlocks() const { return 20; }

void BlowDetector::resetEWMAs()
{
  blocks_since_1above_ = kForeverBlocksAgo;
  blocks_since_7above_ = kForeverBlocksAgo;
  delay_blocks_left_ = -1;
  deactivate_warmup_blocks_left_ = -1;
}

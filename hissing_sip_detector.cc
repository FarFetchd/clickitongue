#include "hissing_sip_detector.h"

#include <cassert>
#include <cmath>

HissingSipDetector::HissingSipDetector(
    BlockingQueue<Action>* action_queue,
    double o7_on_thresh, double o7_off_thresh, double o1_limit,
    double ewma_alpha, bool require_warmup, std::vector<int>* cur_frame_dest)
: Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
  o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
  o1_limit_(o1_limit), ewma_alpha_(ewma_alpha),
  one_minus_ewma_alpha_(1.0-ewma_alpha_), require_warmup_(require_warmup)
{}

HissingSipDetector::HissingSipDetector(
    BlockingQueue<Action>* action_queue, Action action_on, Action action_off,
    double o7_on_thresh, double o7_off_thresh, double o1_limit,
    double ewma_alpha, bool require_warmup)
: Detector(action_on, action_off, action_queue),
  o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
  o1_limit_(o1_limit), ewma_alpha_(ewma_alpha),
  one_minus_ewma_alpha_(1.0-ewma_alpha_), require_warmup_(require_warmup)
{}

void HissingSipDetector::updateState(const fftw_complex* freq_power)
{
  o1_ewma_ = o1_ewma_ * one_minus_ewma_alpha_ + freq_power[1][0] * ewma_alpha_;

  double cur_o7 = 0;
  for (int i=64; i<128; i++)
    cur_o7 += freq_power[i][0];
  o7_ewma_ = o7_ewma_ * one_minus_ewma_alpha_ + cur_o7 * ewma_alpha_;

  if (on_)
    warmup_blocks_left_ = kHissingSipWarmupBlocks;
}

bool HissingSipDetector::shouldTransitionOn()
{
  if (!(o7_ewma_ > o7_on_thresh_ && o1_ewma_ < o1_limit_))
  {
    warmup_blocks_left_ = kHissingSipWarmupBlocks;
    return false;
  }
  if (require_warmup_ && --warmup_blocks_left_ >= 0)
    return false;
  return true;
}

bool HissingSipDetector::shouldTransitionOff() const
{
  return o7_ewma_ < o7_off_thresh_;
}

int HissingSipDetector::refracPeriodLengthBlocks() const { return 16; }

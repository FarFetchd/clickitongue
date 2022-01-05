#include "hum_detector.h"

HumDetector::HumDetector(BlockingQueue<Action>* action_queue,
                         double o1_on_thresh, double o1_off_thresh,
                         double o2_on_thresh, double o3_limit, double o6_limit,
                         double ewma_alpha, bool require_warmup,
                         std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
    o1_on_thresh_(o1_on_thresh), o1_off_thresh_(o1_off_thresh),
    o2_on_thresh_(o2_on_thresh), o3_limit_(o3_limit), o6_limit_(o6_limit),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_),
    require_warmup_(require_warmup)
{}

HumDetector::HumDetector(BlockingQueue<Action>* action_queue,
                         Action action_on, Action action_off,
                         double o1_on_thresh, double o1_off_thresh,
                         double o2_on_thresh,
                         double o3_limit, double o6_limit, double ewma_alpha,
                         bool require_warmup)
  : Detector(action_on, action_off, action_queue),
    o1_on_thresh_(o1_on_thresh), o1_off_thresh_(o1_off_thresh),
    o2_on_thresh_(o2_on_thresh), o3_limit_(o3_limit), o6_limit_(o6_limit),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_),
    require_warmup_(require_warmup)
{}

void HumDetector::updateState(const fftw_complex* freq_power)
{
  o1_ewma_ = o1_ewma_ * one_minus_ewma_alpha_ + freq_power[1][0] * ewma_alpha_;
  o2_ewma_ = o2_ewma_ * one_minus_ewma_alpha_ + (freq_power[2][0] +
                                                 freq_power[3][0]) * ewma_alpha_;
  o3_ewma_ = o3_ewma_ * one_minus_ewma_alpha_ + ewma_alpha_ * (
      freq_power[4][0] + freq_power[5][0] + freq_power[6][0] + freq_power[7][0]);

  double cur_o6 = 0;
  for (int i=32; i<64; i++)
    cur_o6 += freq_power[i][0];
  o6_ewma_ = o6_ewma_ * one_minus_ewma_alpha_ + cur_o6 * ewma_alpha_;

  if (on_)
    warmup_blocks_left_ = kHumWarmupBlocks;
}

bool HumDetector::shouldTransitionOn()
{
  if (!(o1_ewma_ > o1_on_thresh_ && o2_ewma_ > o2_on_thresh_ &&
        o3_ewma_ < o3_limit_ && o6_ewma_ < o6_limit_))
  {
    warmup_blocks_left_ = kHumWarmupBlocks;
    return false;
  }
  if (require_warmup_ && --warmup_blocks_left_ >= 0)
    return false;
  return true;
}

bool HumDetector::shouldTransitionOff() const
{
  return o1_ewma_ < o1_off_thresh_;
}

int HumDetector::refracPeriodLengthBlocks() const { return 16; }

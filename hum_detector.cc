#include "hum_detector.h"

#include <cassert>
#include <cmath>

HumDetector::HumDetector(BlockingQueue<Action>* action_queue,
                         double o1_on_thresh, double o1_off_thresh, double o6_limit,
                         double ewma_alpha, std::vector<int>* cur_frame_dest)
  : Detector(action_queue, cur_frame_dest),
    action_on_(Action::RecordCurFrame), action_off_(Action::NoAction),
    o1_on_thresh_(o1_on_thresh), o1_off_thresh_(o1_off_thresh), o6_limit_(o6_limit),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_)
{}

HumDetector::HumDetector(BlockingQueue<Action>* action_queue,
                         Action action_on, Action action_off, double o1_on_thresh,
                         double o1_off_thresh, double o6_limit, double ewma_alpha)
  : Detector(action_queue), action_on_(action_on), action_off_(action_off),
    o1_on_thresh_(o1_on_thresh), o1_off_thresh_(o1_off_thresh), o6_limit_(o6_limit),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_)
{
  assert(action_on_ != Action::RecordCurFrame);
  assert(action_off_ != Action::RecordCurFrame);
}

void HumDetector::processFourier(const fftw_complex* freq_power)
{
  o1_ewma_ = o1_ewma_ * one_minus_ewma_alpha_ + freq_power[1][0] * ewma_alpha_;

  double cur_o6 = 0;
  for (int i=32; i<64; i++)
    cur_o6 += freq_power[i][0];
  o6_ewma_ = o6_ewma_ * one_minus_ewma_alpha_ + cur_o6 * ewma_alpha_;

  if (on_)
  {
    if (o1_ewma_ < o1_off_thresh_)
    {
      on_ = false;
      kickoffAction(action_off_);
    }
  }
  else
  {
    if (refrac_blocks_left_ > 0)
      refrac_blocks_left_--;
    else if (o1_ewma_ > o1_on_thresh_ && o6_ewma_ < o6_limit_)
    {
      on_ = true;
      kickoffAction(action_on_);
      refrac_blocks_left_ = kRefracBlocks;
    }
  }
}

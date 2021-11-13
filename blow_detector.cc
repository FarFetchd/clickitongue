#include "blow_detector.h"

#include <cassert>
#include <cmath>

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           double o5_on_thresh, double o5_off_thresh,
                           double o6_on_thresh, double o6_off_thresh,
                           double o7_on_thresh, double o7_off_thresh,
                           double ewma_alpha, std::vector<int>* cur_frame_dest)
  : Detector(action_queue, cur_frame_dest),
    action_on_(Action::RecordCurFrame), action_off_(Action::NoAction),
    o5_on_thresh_(o5_on_thresh), o5_off_thresh_(o5_off_thresh),
    o6_on_thresh_(o6_on_thresh), o6_off_thresh_(o6_off_thresh),
    o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_)
{}

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           Action action_on, Action action_off,
                           double o5_on_thresh, double o5_off_thresh,
                           double o6_on_thresh, double o6_off_thresh,
                           double o7_on_thresh, double o7_off_thresh,
                           double ewma_alpha)
  : Detector(action_queue), action_on_(action_on), action_off_(action_off),
    o5_on_thresh_(o5_on_thresh), o5_off_thresh_(o5_off_thresh),
    o6_on_thresh_(o6_on_thresh), o6_off_thresh_(o6_off_thresh),
    o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_)
{
  assert(action_on_ != Action::RecordCurFrame);
  assert(action_off_ != Action::RecordCurFrame);
}

void BlowDetector::processFourier(const fftw_complex* freq_power)
{
  double cur_o5 = 0;
  for (int i=16; i<32; i++)
    cur_o5 += freq_power[i][0];
  o5_ewma_ = o5_ewma_ * one_minus_ewma_alpha_ + cur_o5 * ewma_alpha_;

  double cur_o6 = 0;
  for (int i=32; i<64; i++)
    cur_o6 += freq_power[i][0];
  o6_ewma_ = o6_ewma_ * one_minus_ewma_alpha_ + cur_o6 * ewma_alpha_;

  double cur_o7 = 0;
  for (int i=64; i<128; i++)
    cur_o7 += freq_power[i][0];
  o7_ewma_ = o7_ewma_ * one_minus_ewma_alpha_ + cur_o7 * ewma_alpha_;

  if (on_)
  {
    if (o5_ewma_ < o5_off_thresh_ && o6_ewma_ < o6_off_thresh_ && o7_ewma_ < o7_off_thresh_)
    {
      on_ = false;
      kickoffAction(action_off_);
    }
  }
  else
  {
    if (refrac_blocks_left_ > 0)
      refrac_blocks_left_--;
    else if (o5_ewma_ > o5_on_thresh_ && o6_ewma_ > o6_on_thresh_ && o7_ewma_ > o7_on_thresh_)
    {
      on_ = true;
      kickoffAction(action_on_);
      refrac_blocks_left_ = kRefracBlocks;
    }
  }
}

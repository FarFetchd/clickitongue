#include "blow_detector.h"

#include <cassert>
#include <cmath>

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           double o5_on_thresh, double o5_off_thresh,
                           double o6_on_thresh, double o6_off_thresh,
                           double o7_on_thresh, double o7_off_thresh,
                           double ewma_alpha, std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
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
  : Detector(action_on, action_off, action_queue),
    o5_on_thresh_(o5_on_thresh), o5_off_thresh_(o5_off_thresh),
    o6_on_thresh_(o6_on_thresh), o6_off_thresh_(o6_off_thresh),
    o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_)
{}

void BlowDetector::updateState(const fftw_complex* freq_power)
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
}

bool BlowDetector::shouldTransitionOn() const
{
  return o5_ewma_ > o5_on_thresh_ &&
         o6_ewma_ > o6_on_thresh_ &&
         o7_ewma_ > o7_on_thresh_;
}

bool BlowDetector::shouldTransitionOff() const
{
  return o5_ewma_ < o5_off_thresh_ &&
         o6_ewma_ < o6_off_thresh_ &&
         o7_ewma_ < o7_off_thresh_;
}

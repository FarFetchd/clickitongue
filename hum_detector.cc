#include "hum_detector.h"

#include <cassert>
#include <cmath>

HumDetector::HumDetector(BlockingQueue<Action>* action_queue,
                         double o1_on_thresh, double o1_off_thresh, double o6_limit,
                         double ewma_alpha, std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
    o1_on_thresh_(o1_on_thresh), o1_off_thresh_(o1_off_thresh), o6_limit_(o6_limit),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_)
{}

HumDetector::HumDetector(BlockingQueue<Action>* action_queue,
                         Action action_on, Action action_off, double o1_on_thresh,
                         double o1_off_thresh, double o6_limit, double ewma_alpha)
  : Detector(action_on, action_off, action_queue),
    o1_on_thresh_(o1_on_thresh), o1_off_thresh_(o1_off_thresh), o6_limit_(o6_limit),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_)
{}

void HumDetector::updateState(const fftw_complex* freq_power)
{
  o1_ewma_ = o1_ewma_ * one_minus_ewma_alpha_ + freq_power[1][0] * ewma_alpha_;

  double cur_o6 = 0;
  for (int i=32; i<64; i++)
    cur_o6 += freq_power[i][0];
  o6_ewma_ = o6_ewma_ * one_minus_ewma_alpha_ + cur_o6 * ewma_alpha_;
}

bool HumDetector::shouldTransitionOn() const
{
  return o1_ewma_ > o1_on_thresh_ && o6_ewma_ < o6_limit_;
}

bool HumDetector::shouldTransitionOff() const
{
  return o1_ewma_ < o1_off_thresh_;
}

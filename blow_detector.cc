#include "blow_detector.h"

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           double o1_on_thresh,
                           double o6_on_thresh, double o6_off_thresh,
                           double o7_on_thresh, double o7_off_thresh,
                           double ewma_alpha, std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
    o1_on_thresh_(o1_on_thresh),
    o6_on_thresh_(o6_on_thresh), o6_off_thresh_(o6_off_thresh),
    o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_),
    activated_ewma_alpha_(ewma_alpha_*0.75),
    one_minus_activated_ewma_alpha_(1.0-activated_ewma_alpha_)
{}

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           Action action_on, Action action_off,
                           double o1_on_thresh,
                           double o6_on_thresh, double o6_off_thresh,
                           double o7_on_thresh, double o7_off_thresh,
                           double ewma_alpha)
  : Detector(action_on, action_off, action_queue),
    o1_on_thresh_(o1_on_thresh),
    o6_on_thresh_(o6_on_thresh), o6_off_thresh_(o6_off_thresh),
    o7_on_thresh_(o7_on_thresh), o7_off_thresh_(o7_off_thresh),
    ewma_alpha_(ewma_alpha), one_minus_ewma_alpha_(1.0-ewma_alpha_),
    activated_ewma_alpha_(ewma_alpha_*0.75),
    one_minus_activated_ewma_alpha_(1.0-activated_ewma_alpha_)
{}

void BlowDetector::updateState(const fftw_complex* freq_power)
{
  double cur_alpha, cur_1minus_alpha;
  if (on_)
  {
    cur_alpha = activated_ewma_alpha_;
    cur_1minus_alpha = one_minus_activated_ewma_alpha_;
  }
  else
  {
    cur_alpha = ewma_alpha_;
    cur_1minus_alpha = one_minus_ewma_alpha_;
  }

  double cur_o1 = freq_power[1][0];
  o1_ewma_ = o1_ewma_ * cur_1minus_alpha + cur_o1 * cur_alpha;

  double cur_o6 = 0;
  for (int i=32; i<64; i++)
    cur_o6 += freq_power[i][0];
  o6_ewma_ = o6_ewma_ * cur_1minus_alpha + cur_o6 * cur_alpha;

  double cur_o7 = 0;
  for (int i=64; i<128; i++)
    cur_o7 += freq_power[i][0];
  o7_ewma_ = o7_ewma_ * cur_1minus_alpha + cur_o7 * cur_alpha;
}

bool BlowDetector::shouldTransitionOn()
{
  return o1_ewma_ > o1_on_thresh_ &&
         o6_ewma_ > o6_on_thresh_ &&
         o7_ewma_ > o7_on_thresh_;
}

bool BlowDetector::shouldTransitionOff() const
{
  return o6_ewma_ < o6_off_thresh_ &&
         o7_ewma_ < o7_off_thresh_;
}

int BlowDetector::refracPeriodLengthBlocks() const { return 7; }

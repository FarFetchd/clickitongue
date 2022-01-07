#include "cat_detector.h"

CatDetector::CatDetector(BlockingQueue<Action>* action_queue,
                         double o5_on_thresh, double o6_on_thresh,
                         double o7_on_thresh, std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
    o5_on_thresh_(o5_on_thresh), o6_on_thresh_(o6_on_thresh),
    o7_on_thresh_(o7_on_thresh)
{}

CatDetector::CatDetector(BlockingQueue<Action>* action_queue, Action action_on,
                         Action action_off, double o5_on_thresh,
                         double o6_on_thresh, double o7_on_thresh)
  : Detector(action_on, action_off, action_queue), o5_on_thresh_(o5_on_thresh),
    o6_on_thresh_(o6_on_thresh), o7_on_thresh_(o7_on_thresh)
{}

void CatDetector::updateState(const fftw_complex* freq_power)
{
  o5_cur_ = 0;
  for (int i=16; i<32; i++)
    o5_cur_ += freq_power[i][0];

  o6_cur_ = 0;
  for (int i=32; i<64; i++)
    o6_cur_ += freq_power[i][0];

  o7_cur_ = 0;
  for (int i=64; i<128; i++)
    o7_cur_ += freq_power[i][0];
}

bool CatDetector::shouldTransitionOn()
{
  bool satisfied = o5_cur_ > o5_on_thresh_ &&
                   o6_cur_ > o6_on_thresh_ &&
                   o7_cur_ > o7_on_thresh_;
  if (warmed_up_)
  {
    warmed_up_ = false;
    return satisfied;
  }
  if (satisfied)
    warmed_up_ = true;
  return false;
}

bool CatDetector::shouldTransitionOff() const
{
  return o5_cur_ < o5_on_thresh_ &&
         o6_cur_ < o6_on_thresh_ &&
         o7_cur_ < o7_on_thresh_;
}

int CatDetector::refracPeriodLengthBlocks() const { return 7; }

void CatDetector::resetEWMAs() {}

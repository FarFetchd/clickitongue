#include "cat_detector.h"

CatDetector::CatDetector(BlockingQueue<Action>* action_queue,
                         double o7_on_thresh, double o1_limit,
                         std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
    o7_on_thresh_(o7_on_thresh), o1_limit_(o1_limit)
{}

CatDetector::CatDetector(BlockingQueue<Action>* action_queue, Action action_on,
                         Action action_off, double o7_on_thresh, double o1_limit)
  : Detector(action_on, action_off, action_queue),
    o7_on_thresh_(o7_on_thresh), o1_limit_(o1_limit)
{}

void CatDetector::updateState(const fftw_complex* freq_power)
{
  if (freq_power[1][0] > o1_limit_)
    o1_cooldown_blocks_ = 8;

  o7_cur_ = 0;
  for (int i=64; i<128; i++)
    o7_cur_ += freq_power[i][0];
}

bool CatDetector::shouldTransitionOn()
{
  if (o1_cooldown_blocks_ > 0 && --o1_cooldown_blocks_ > 0)
  {
    warmed_up_ = false;
    return false;
  }

  bool satisfied = o7_cur_ > o7_on_thresh_;
  if (warmed_up_)
  {
    warmed_up_ = false;
    return satisfied;
  }
  if (satisfied)
    warmed_up_ = true;
  return false;
}

bool CatDetector::shouldTransitionOff()
{
  return o7_cur_ < o7_on_thresh_;
}

int CatDetector::refracPeriodLengthBlocks() const { return 7; }

void CatDetector::resetEWMAs() {}

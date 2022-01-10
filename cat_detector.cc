#include "cat_detector.h"

CatDetector::CatDetector(BlockingQueue<Action>* action_queue,
                         double o7_on_thresh, double o1_limit, bool use_limit,
                         std::vector<int>* cur_frame_dest)
  : Detector(Action::RecordCurFrame, Action::NoAction, action_queue, cur_frame_dest),
    o7_on_thresh_(o7_on_thresh), o1_limit_(o1_limit), use_limit_(use_limit),
    cur_o7_thresh_(o7_on_thresh_)
{}

CatDetector::CatDetector(BlockingQueue<Action>* action_queue, Action action_on,
                         Action action_off, double o7_on_thresh, double o1_limit,
                         bool use_limit)
  : Detector(action_on, action_off, action_queue),
    o7_on_thresh_(o7_on_thresh), o1_limit_(o1_limit), use_limit_(use_limit),
    cur_o7_thresh_(o7_on_thresh_)
{}

void CatDetector::updateState(const fftw_complex* freq_power)
{
  if (use_limit_)
  {
    if (freq_power[1][0] > o1_limit_)
    {
      o1_cooldown_blocks_ = 7;
      cur_o7_thresh_ = o7_on_thresh_ + 7 * 2 * o7_on_thresh_;
      warmed_up_ = false;
    }
    else if (o1_cooldown_blocks_ > 0)
    {
      o1_cooldown_blocks_--;
      cur_o7_thresh_ -= 2 * o7_on_thresh_;
    }
  }

  o7_cur_ = 0;
  for (int i=64; i<128; i++)
    o7_cur_ += freq_power[i][0];
}

bool CatDetector::shouldTransitionOn()
{
  bool satisfied = o7_cur_ > cur_o7_thresh_;
  if (!use_limit_)
    return satisfied;

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

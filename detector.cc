#include "detector.h"

Detector::Detector(Action action_on, Action action_off,
                   BlockingQueue<Action>* action_queue,
                   std::vector<int>* cur_frame_dest)
  : action_on_(action_on), action_off_(action_off),
    action_queue_(action_queue), cur_frame_dest_(cur_frame_dest) {}

Detector::~Detector() {}

void Detector::processFourierOutputBlock(const fftw_complex* freq_power)
{
  cur_frame_ += kFourierBlocksize;
  updateState(freq_power);

  if (on_)
  {
    if (shouldTransitionOff())
    {
      on_ = false;
      kickoffAction(action_off_);
      resetEWMAs();
    }
  }
  else // off
  {
    if (refrac_blocks_left_ > 0)
      refrac_blocks_left_--;
    else if (shouldTransitionOn())
    {
      on_ = true;
      kickoffAction(action_on_);
    }
  }

  if (on_)
  {
    beginRefractoryPeriod(refracPeriodLengthBlocks());
    for (Detector* target : inhibition_targets_)
    {
      target->beginRefractoryPeriod(target->refracPeriodLengthBlocks());
      target->resetEWMAs();
    }
  }
}

void Detector::kickoffAction(Action action)
{
  if (action == Action::RecordCurFrame)
    cur_frame_dest_->push_back(cur_frame_);
  else if (action != Action::NoAction)
    action_queue_->enqueue(action);
}

void Detector::beginRefractoryPeriod(int length_blocks)
{
  refrac_blocks_left_ = length_blocks;
}

void Detector::addInhibitionTarget(Detector* target)
{
  inhibition_targets_.push_back(target);
}

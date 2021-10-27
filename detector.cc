#include "detector.h"

Detector::Detector(BlockingQueue<Action>* action_queue)
  : action_queue_(action_queue) {}

void Detector::kickoffAction(Action action)
{
  if (action == Action::RecordCurFrame)
    cur_frame_dest_->push_back(*cur_frame_src_);
  else if (action != Action::NoAction)
  {
    if (!action_queue_)
      printf("action_queue_ null, cannot kickoff action\n");
    else
      action_queue_->enqueue(action);
  }
}

void Detector::setCurFrameSource(int* src)
{
  cur_frame_src_ = src;
}
void Detector::setCurFrameDest(std::vector<int>* dest)
{
  cur_frame_dest_ = dest;
}

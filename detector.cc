#include "detector.h"

Detector::Detector(BlockingQueue<Action>* action_queue,
                   std::vector<int>* cur_frame_dest)
  : action_queue_(action_queue), cur_frame_dest_(cur_frame_dest) {}

void Detector::markFrameAndProcessFourier(const fftw_complex* freq_power)
{
  cur_frame_ += kFourierBlocksize;
  processFourier(freq_power);
}

void Detector::kickoffAction(Action action)
{
  if (action == Action::RecordCurFrame)
    cur_frame_dest_->push_back(cur_frame_);
  else if (action != Action::NoAction)
    action_queue_->enqueue(action);
}

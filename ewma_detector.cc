#include "ewma_detector.h"

#include "equalizer.h"

EwmaDetector::EwmaDetector(BlockingQueue<Action>* action_queue, float alpha,
                           int refractory_ms, float ewma_thresh_high,
                           float ewma_thresh_low, std::vector<int>* cur_frame_dest)
  : Detector(action_queue), ewma_alpha_(alpha), ewma_1_alpha_(1.0-alpha),
    refractory_period_frames_((int)(refractory_ms * ((kFramesPerSec * kNumChannels)
                                                     / 1000.0))),
    ewma_thresh_high_(ewma_thresh_high),
    ewma_thresh_low_(ewma_thresh_low), action_(Action::RecordCurFrame),
    refractory_frames_left_(refractory_period_frames_)
{
  setCurFrameDest(cur_frame_dest);
  setCurFrameSource(&cur_frame_);
}

EwmaDetector::EwmaDetector(BlockingQueue<Action>* action_queue, float alpha,
                           int refractory_ms, float ewma_thresh_high,
                           float ewma_thresh_low, Action action)
  : Detector(action_queue), ewma_alpha_(alpha), ewma_1_alpha_(1.0-alpha),
    refractory_period_frames_((int)(refractory_ms * ((kFramesPerSec * kNumChannels)
                                                     / 1000.0))),
    ewma_thresh_high_(ewma_thresh_high),
    ewma_thresh_low_(ewma_thresh_low), action_(action),
    refractory_frames_left_(refractory_period_frames_)
{
//    assert(action_ != Action::RecordCurFrame);
}

void EwmaDetector::processAudio(const Sample* cur_sample, int num_frames)
{
  static int print_once_per_10ms_chunks = 0;
  if (++print_once_per_10ms_chunks == 10)
  {
    printEqualizer(cur_sample, num_frames, kNumChannels);
    print_once_per_10ms_chunks=0;
  }

  for (int i=0; i<num_frames; i++,cur_frame_++)
  {
    ewma_ = ewma_1_alpha_ * ewma_ +
            ewma_alpha_ * (*cur_sample >= 0 ? *cur_sample: -*cur_sample);
    cur_sample++; // left or mono

    if (ewma_ > ewma_thresh_high_ && refractory_frames_left_ <= 0)
    {
      kickoffAction(action_);
      refractory_frames_left_ = refractory_period_frames_;
    }
    else if (ewma_ < ewma_thresh_low_ && refractory_frames_left_ > 0)
      refractory_frames_left_--;
//printf("%g\n", ewma_);
    if (kNumChannels == 2)
      cur_sample++; // skip right
  }
}

int ewmaUpdateCallback(const void* inputBuffer, void* outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags, void* userData)
{
  EwmaDetector* detector = (EwmaDetector*)userData;
  const Sample* rptr = (const Sample*)inputBuffer;
  if (inputBuffer != NULL)
    detector->processAudio(rptr, framesPerBuffer);
  return paContinue;
}

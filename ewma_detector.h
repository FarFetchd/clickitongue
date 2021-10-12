#ifndef CLICKITONGUE_EWMA_DETECTOR_H_
#define CLICKITONGUE_EWMA_DETECTOR_H_

#include "portaudio.h"

#include "actions.h"
#include "constants.h"
#include "detector.h"

class EwmaDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  EwmaDetector(BlockingQueue<Action>* action_queue, int refractory_ms,
               float ewma_thresh_low, float ewma_thresh_high, float alpha,
               std::vector<int>* cur_frame_dest);

  // Kicks off 'action' at each detected event.
  EwmaDetector(BlockingQueue<Action>* action_queue, int refractory_ms,
               float ewma_thresh_low, float ewma_thresh_high, float alpha,
               Action action);

  void processAudio(const Sample* cur_sample, int num_frames) override;

private:
  // for the record,  --refractory_ms=50 --ewma_thresh_low=0.25
  //   --ewma_thresh_high=0.55 --ewma_alpha=0.13 are pretty good for tongue clicks
  const int refractory_period_frames_;
  const float ewma_thresh_low_;
  const float ewma_thresh_high_;
  const float ewma_alpha_;
  const float ewma_1_alpha_;
  const Action action_;

  float ewma_ = 0;
  int refractory_frames_left_;

  // only needs to be kept up to date if you plan to use RecordCurFrame
  int cur_frame_ = 0;
  bool track_cur_frame_ = false;
};

int ewmaUpdateCallback(const void* inputBuffer, void* outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags, void* userData);

#endif // CLICKITONGUE_EWMA_DETECTOR_H_

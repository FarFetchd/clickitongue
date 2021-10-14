#ifndef CLICKITONGUE_BLOW_DETECTOR_H_
#define CLICKITONGUE_BLOW_DETECTOR_H_

#include "portaudio.h"

#include "actions.h"
#include "constants.h"
#include "detector.h"
#include "easy_fourier.h"

class BlowDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  BlowDetector(BlockingQueue<Action>* action_queue,
               float lowpass_percent, float highpass_percent,
               double low_on_thresh, double low_off_thresh,
               double high_on_thresh, double high_off_thresh,
               int blocksize, std::vector<int>* cur_frame_dest);

  // Kicks off 'action' at each detected event.
  BlowDetector(BlockingQueue<Action>* action_queue, Action action,
               float lowpass_percent, float highpass_percent,
               double low_on_thresh, double low_off_thresh,
               double high_on_thresh, double high_off_thresh,
               int blocksize);

  void processAudio(const Sample* cur_sample, int num_frames) override;

private:
  const Action action_;

  // the percentile bucket (rounded) of the Fourier output that is the highest
  // we should consider in the lowpass half.
  const float lowpass_percent_ = 0.035;
  // ditto, lowest for the highpass half.
  const float highpass_percent_ = 0.5;

  // The average value threshold needed to be crossed by the lowpass half in
  // order to move to "mouse down" state.
  const double low_on_thresh_ = 10;
  // etc
  const double low_off_thresh_ = 3;
  const double high_on_thresh_ = 0.75;
  const double high_off_thresh_ = 0.1;

  // our understanding of the current state of the mouse button
  bool mouse_down_ = false;

  // Number of frames we expect to get per callback, to feed to Fourier.
  // Should be a power of 2.
  const int blocksize_;
  const EasyFourier fourier_;

  // only needs to be kept up to date if you plan to use RecordCurFrame
  int cur_frame_ = 0;
  bool track_cur_frame_ = false;
};

int blowDetectorCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags, void* userData);

#endif // CLICKITONGUE_BLOW_DETECTOR_H_
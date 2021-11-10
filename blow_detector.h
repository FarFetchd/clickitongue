#ifndef CLICKITONGUE_BLOW_DETECTOR_H_
#define CLICKITONGUE_BLOW_DETECTOR_H_

#include "constants.h"
#include "detector.h"
#include "easy_fourier.h"
#include "tongue_detector.h"

class BlowDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  BlowDetector(BlockingQueue<Action>* action_queue,
               double lowpass_percent, double highpass_percent,
               double low_on_thresh, double low_off_thresh,
               double high_on_thresh, double high_off_thresh,
               double high_spike_frac, double high_spike_level,
               std::vector<int>* cur_frame_dest);

  // Kicks off action_on, action_off at each corresponding detected event.
  BlowDetector(BlockingQueue<Action>* action_queue,
               Action action_on, Action action_off,
               double lowpass_percent, double highpass_percent,
               double low_on_thresh, double low_off_thresh,
               double high_on_thresh, double high_off_thresh,
               double high_spike_frac, double high_spike_level);

  void processAudio(const Sample* cur_sample, int num_frames);
  void processFourier(const fftw_complex* fft_bins);

  void set_tongue_link(TongueDetector* val);

private:
  // action to be done when detector decides a blow has started...
  const Action action_on_;
  // ...or stopped.
  const Action action_off_;

  // the percentile bucket (rounded) of the Fourier output that is the highest
  // we should consider in the lowpass half.
  const double lowpass_percent_ = 0.035;
  // ditto, lowest for the highpass half.
  const double highpass_percent_ = 0.5;

  // The average value threshold needed to be crossed by the lowpass half in
  // order to move to "mouse down" state.
  const double low_on_thresh_ = 10;
  // etc
  const double low_off_thresh_ = 3;
  const double high_on_thresh_ = 0.75;
  const double high_off_thresh_ = 0.1;
  // Require this fraction of highpass'd bins to be above this level.
  const double high_spike_frac_ = 0.5;
  const double high_spike_level_ = 1.0;

  // our understanding of the current state of the mouse button
  bool mouse_down_ = false;
  bool mouse_down_at_least_one_block_ = false;

  // for coordinating with tongue detector (if it exists) by suppressing it
  TongueDetector* tongue_link_ = nullptr;

  // only needs to be kept up to date if you plan to use RecordCurFrame
  int cur_frame_ = 0;
  bool track_cur_frame_ = false;

  int refrac_blocks_left_ = 0;
};

#endif // CLICKITONGUE_BLOW_DETECTOR_H_

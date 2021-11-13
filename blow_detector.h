#ifndef CLICKITONGUE_BLOW_DETECTOR_H_
#define CLICKITONGUE_BLOW_DETECTOR_H_

#include "detector.h"

class BlowDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  BlowDetector(BlockingQueue<Action>* action_queue,
               double o5_on_thresh, double o5_off_thresh, double o6_on_thresh,
               double o6_off_thresh, double o7_on_thresh, double o7_off_thresh,
               double ewma_alpha, std::vector<int>* cur_frame_dest);

  // Kicks off action_on, action_off at each corresponding detected event.
  BlowDetector(BlockingQueue<Action>* action_queue,
               Action action_on, Action action_off,
               double o5_on_thresh, double o5_off_thresh, double o6_on_thresh,
               double o6_off_thresh, double o7_on_thresh, double o7_off_thresh,
               double ewma_alpha);

protected:
  // IMPORTANT: although the type is fftw_complex, in fact freq_power[i][0] for
  // each i is expected to be the squared magnitude (i.e. real^2 + imag_coeff^2)
  // of the original complex number output at bin i.
  // The imaginary coefficient (array index 1) is left untouched - although
  // you're likely not at all interested in it.
  void processFourier(const fftw_complex* freq_power) override;

private:
  // action to be done when detector decides a blow has started...
  const Action action_on_;
  // ...or stopped.
  const Action action_off_;

  // o5,6,7 are octaves. o1 is bin 1, o2 is bins 2+3, o3 is bins 4+5+6+7,...
  // ...o5 is bins 16+17+...+31, o6 is 32+...+63, o7 is 64+...+127.
  const double o5_on_thresh_;
  const double o5_off_thresh_;
  const double o6_on_thresh_;
  const double o6_off_thresh_;
  const double o7_on_thresh_;
  const double o7_off_thresh_;
  const double ewma_alpha_;
  const double one_minus_ewma_alpha_;

  double o5_ewma_ = 0;
  double o6_ewma_ = 0;
  double o7_ewma_ = 0;
  bool on_ = false;
  int refrac_blocks_left_ = 0;
};

#endif // CLICKITONGUE_BLOW_DETECTOR_H_

#ifndef CLICKITONGUE_CAT_DETECTOR_H_
#define CLICKITONGUE_CAT_DETECTOR_H_

#include "detector.h"

class CatDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  CatDetector(BlockingQueue<Action>* action_queue,
              double o1_limit, double o6_on_thresh,
              double o6_off_thresh, double o7_on_thresh, double o7_off_thresh,
              double ewma_alpha, std::vector<int>* cur_frame_dest);

  // Kicks off action_on, action_off at each corresponding detected event.
  CatDetector(BlockingQueue<Action>* action_queue,
              Action action_on, Action action_off,
              double o1_limit, double o6_on_thresh,
              double o6_off_thresh, double o7_on_thresh, double o7_off_thresh,
              double ewma_alpha);

protected:
  // IMPORTANT: although the type is fftw_complex, in fact freq_power[i][0] for
  // each i is expected to be the squared magnitude (i.e. real^2 + imag_coeff^2)
  // of the original complex number output at bin i.
  // The imaginary coefficient (array index 1) is left untouched - although
  // you're likely not at all interested in it.
  void updateState(const fftw_complex* freq_power) override;

  bool shouldTransitionOn() override;
  bool shouldTransitionOff() const override;
  int refracPeriodLengthBlocks() const override;

private:

  // o1,6,7 are octaves. o1 is bin 1, o2 is bins 2+3, o3 is bins 4+5+6+7,...
  // ...o1 is bins 16+17+...+31, o6 is 32+...+63, o7 is 64+...+127.
  const double o1_limit_;
  const double o6_on_thresh_;
  const double o6_off_thresh_;
  const double o7_on_thresh_;
  const double o7_off_thresh_;
  const double ewma_alpha_;
  const double one_minus_ewma_alpha_;

  double o1_ewma_ = 0;
  double o6_ewma_ = 0;
  double o7_ewma_ = 0;
};

#endif // CLICKITONGUE_CAT_DETECTOR_H_

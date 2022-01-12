#ifndef CLICKITONGUE_CAT_DETECTOR_H_
#define CLICKITONGUE_CAT_DETECTOR_H_

#include "detector.h"

class CatDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  CatDetector(BlockingQueue<Action>* action_queue,
              double o7_on_thresh, double o1_limit, bool use_limit, double scale,
              std::vector<int>* cur_frame_dest);

  // Kicks off action_on, action_off at each corresponding detected event.
  CatDetector(BlockingQueue<Action>* action_queue,
              Action action_on, Action action_off, double o7_on_thresh,
              double o1_limit, bool use_limit, double scale);

protected:
  // IMPORTANT: although the type is fftw_complex, in fact freq_power[i][0] for
  // each i is expected to be the squared magnitude (i.e. real^2 + imag_coeff^2)
  // of the original complex number output at bin i.
  // The imaginary coefficient (array index 1) is left untouched - although
  // you're likely not at all interested in it.
  void updateState(const fftw_complex* freq_power) override;

  bool shouldTransitionOn() override;
  bool shouldTransitionOff() override;
  int refracPeriodLengthBlocks() const override;

  void resetEWMAs() override;

private:
  // Normal activation threshold for octave 7. When we have recently seen o1's
  // limit get exceeded, we boost this threshold, and then decay it back down
  // in units of o7_on_thresh_.
  const double o7_on_thresh_;
  const double o1_limit_;

  // Whether the o1_limit_ logic actually applies.
  const bool use_limit_;

  // =o7_on_thresh_ normally, or a multiple of it if o1_limit recently exceeded.
  double cur_o7_thresh_;

  double o7_cur_ = 0;

  // Require satisfying activation criteria for two blocks in a row
  bool warmed_up_ = false;

  int o1_cooldown_blocks_ = 0;
};

#endif // CLICKITONGUE_CAT_DETECTOR_H_

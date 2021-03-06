#ifndef CLICKITONGUE_HUM_DETECTOR_H_
#define CLICKITONGUE_HUM_DETECTOR_H_

#include "detector.h"

constexpr int kHumDelayBlocks = 4;

class HumDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  HumDetector(BlockingQueue<Action>* action_queue,
              double o1_on_thresh, double o1_off_thresh, double o6_limit,
              double ewma_alpha, bool require_delay,
              std::vector<int>* cur_frame_dest);

  // Kicks off action_on, action_off at each corresponding detected event.
  HumDetector(BlockingQueue<Action>* action_queue,
              Action action_on, Action action_off,
              double o1_on_thresh, double o1_off_thresh, double o6_limit,
              double ewma_alpha, bool require_delay);

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
  // o5,6,7 are octaves. o1 is bin 1, o2 is bins 2+3, o3 is bins 4+5+6+7,...
  // ...o5 is bins 16+17+...+31, o6 is 32+...+63, o7 is 64+...+127.
  const double o1_on_thresh_;
  const double o1_off_thresh_;

  // While O6 EWMA above this value, the off->on transition is forbidden.
  // (Going above doesn't kick you out of an on state, though).
  const double o6_limit_;

  const double ewma_alpha_;
  const double one_minus_ewma_alpha_;

  double o1_ewma_ = 0;
  double o6_ewma_ = 0;

  // After exceeding the activation threshold, wait for a little while
  // before actually transitioning to 'on'. This gives the cat/blow detector a
  // chance to inhibit us in the case where the first instant of what will
  // become a cat/blow temporarily looks like a hum.
  int delay_blocks_left_ = -1;
  // Whether the delay_blocks_left_ logic is actually used.
  const bool require_delay_ = false;
};

#endif // CLICKITONGUE_HUM_DETECTOR_H_

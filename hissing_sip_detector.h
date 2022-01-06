#ifndef CLICKITONGUE_HISSING_SIP_DETECTOR_H_
#define CLICKITONGUE_HISSING_SIP_DETECTOR_H_

#include "detector.h"

constexpr int kHissingSipWarmupBlocks = 3;

class HissingSipDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  HissingSipDetector(BlockingQueue<Action>* action_queue,
                     double o7_on_thresh, double o7_off_thresh, double o1_limit,
                     double ewma_alpha, bool require_warmup,
                     std::vector<int>* cur_frame_dest);

  // Kicks off action_on, action_off at each corresponding detected event.
  HissingSipDetector(BlockingQueue<Action>* action_queue,
                     Action action_on, Action action_off,
                     double o7_on_thresh, double o7_off_thresh, double o1_limit,
                     double ewma_alpha, bool require_warmup);

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

  void resetEWMAs() override;

private:
  // o1,2,...,7 are octaves. o1 is bin 1, o2 is bins 2+3, o3 is bins 4+5+6+7,...
  // ...o5 is bins 16+17+...+31, o6 is 32+...+63, o7 is 64+...+127.
  const double o7_on_thresh_;
  const double o7_off_thresh_;

  // While O1 EWMA above this value, the off->on transition is forbidden.
  // (Going above doesn't kick you out of an on state, though).
  const double o1_limit_;

  const double ewma_alpha_;
  const double one_minus_ewma_alpha_;

  double o1_ewma_ = 0;
  double o7_ewma_ = 0;

  // Require consistently exceeding the activation threshold for a little while
  // before actually transitioning to 'on'. This gives the blow detector a
  // chance to inhibit us in the case where the first instant of what will
  // become a blow temporarily looks like a sip.
  int warmup_blocks_left_ = kHissingSipWarmupBlocks;
  // Whether the warmup_blocks_left_ logic is actually used.
  const bool require_warmup_ = false;
};

#endif // CLICKITONGUE_HISSING_SIP_DETECTOR_H_

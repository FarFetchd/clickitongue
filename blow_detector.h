#ifndef CLICKITONGUE_BLOW_DETECTOR_H_
#define CLICKITONGUE_BLOW_DETECTOR_H_

#include "detector.h"

constexpr int kBlowDelayBlocks = 3;
constexpr int kBlowDeactivateWarmupBlocks = 5;
constexpr int kForeverBlocksAgo = 999999999;

class BlowDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  BlowDetector(BlockingQueue<Action>* action_queue,
               double o1_on_thresh, double o7_on_thresh, double o7_off_thresh,
               int lookback_blocks, bool require_delay,
               std::vector<int>* cur_frame_dest);

  // Kicks off action_on, action_off at each corresponding detected event.
  BlowDetector(BlockingQueue<Action>* action_queue,
               Action action_on, Action action_off,
               double o1_on_thresh, double o7_on_thresh, double o7_off_thresh,
               int lookback_blocks, bool require_delay);

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
  // o1,6,7 are octaves. o1 is bin 1, o2 is bins 2+3, o3 is bins 4+5+6+7,...
  // ...o1 is bins 16+17+...+31, o6 is 32+...+63, o7 is 64+...+127.
  const double o1_on_thresh_;
  const double o7_on_thresh_;
  const double o7_off_thresh_;
  // How far in the past (in blocks) counts towards threshold activation.
  // '1' means only the current block or the previous one can be used.
  const int lookback_blocks_;

  double o1_cur_ = 0;
  double o7_cur_ = 0;

  int blocks_since_1above_ = kForeverBlocksAgo;
  int blocks_since_7above_ = kForeverBlocksAgo;

  // After exceeding the activation threshold, wait for a little while
  // before actually transitioning to 'on'. This gives the cat detector a
  // chance to inhibit us in the case where the first instant of what will
  // become a cat temporarily looks like a blow.
  int delay_blocks_left_ = -1;
  // Whether the delay_blocks_left_ logic is actually used.
  const bool require_delay_ = false;

  // How many more blocks we need to stay under the off thresholds before we
  // actually transition to off.
  int deactivate_warmup_blocks_left_ = kBlowDeactivateWarmupBlocks;
};

#endif // CLICKITONGUE_BLOW_DETECTOR_H_

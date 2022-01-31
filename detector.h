#ifndef CLICKITONGUE_DETECTOR_H_
#define CLICKITONGUE_DETECTOR_H_

#include "blocking_queue.h"
#include "constants.h"
#include "easy_fourier.h"

class Detector
{
public:
  // IMPORTANT: although the type is fftw_complex, in fact freq_power[i][0] for
  // each i is expected to be the squared magnitude (i.e. real^2 + imag_coeff^2)
  // of the original complex number output at bin i.
  // The imaginary coefficient (array index 1) is left untouched - although
  // you're likely not at all interested in it.
  void processFourierOutputBlock(const fftw_complex* freq_power);

  // After calling foo.addInhibitionTarget(bar), foo will keep bar's refractory
  // countdown maxed out for as long as foo is in the on state.
  void addInhibitionTarget(Detector* target);

  Detector() = delete;
  virtual ~Detector();

protected:
  Detector(Action action_on, Action action_off,
           BlockingQueue<Action>* action_queue,
           std::vector<int>* cur_frame_dest = nullptr);

  // IMPORTANT: although the type is fftw_complex, in fact freq_power[i][0] for
  // each i is expected to be the squared magnitude (i.e. real^2 + imag_coeff^2)
  // of the original complex number output at bin i.
  // The imaginary coefficient (array index 1) is left untouched - although
  // you're likely not at all interested in it.
  virtual void updateState(const fftw_complex* freq_power) = 0;

  virtual bool shouldTransitionOn() = 0;
  virtual bool shouldTransitionOff() = 0;

  // How long (in units of kFourierBlocksize) we must observe low energy after
  // an event before being willing to declare an event of this type.
  virtual int refracPeriodLengthBlocks() const = 0;

  virtual void resetEWMAs() = 0;

  bool on_ = false;

private:
  void kickoffAction(Action action);
  void beginRefractoryPeriod(int length_blocks);

  // Action to be done when Detector decides an event has started...
  const Action action_on_;
  // ...and stopped.
  const Action action_off_;

  BlockingQueue<Action>* action_queue_ = nullptr;
  int cur_frame_ = 0;
  std::vector<int>* cur_frame_dest_ = nullptr;
  std::vector<Detector*> inhibition_targets_;

  int refrac_blocks_left_ = 0;
  int blocks_since_last_transition_ = 0;
};

#endif // CLICKITONGUE_DETECTOR_H_

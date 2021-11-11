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
  void markFrameAndProcessFourier(const fftw_complex* freq_power);

  Detector() = delete;

protected:
  Detector(BlockingQueue<Action>* action_queue,
           std::vector<int>* cur_frame_dest = nullptr);
  void kickoffAction(Action action);

  // IMPORTANT: although the type is fftw_complex, in fact freq_power[i][0] for
  // each i is expected to be the squared magnitude (i.e. real^2 + imag_coeff^2)
  // of the original complex number output at bin i.
  // The imaginary coefficient (array index 1) is left untouched - although
  // you're likely not at all interested in it.
  virtual void processFourier(const fftw_complex* freq_power) = 0;

private:
  BlockingQueue<Action>* action_queue_ = nullptr;
  int cur_frame_ = 0;
  std::vector<int>* cur_frame_dest_ = nullptr;
};

#endif // CLICKITONGUE_DETECTOR_H_

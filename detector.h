#ifndef CLICKITONGUE_DETECTOR_H_
#define CLICKITONGUE_DETECTOR_H_

#include "blocking_queue.h"
#include "constants.h"

class Detector
{
public:
// I ended up not actually using this polymorphism, so let's drop it unless
// ever needed.
//   virtual void processAudio(const Sample* cur_sample, int num_frames) = 0;
//   virtual void processFourier(const fftw_complex* fft_bins) = 0;
  Detector() = delete;

protected:
  Detector(BlockingQueue<Action>* action_queue);
  void kickoffAction(Action action);

  void setCurFrameSource(int* src);
  void setCurFrameDest(std::vector<int>* dest);

private:
  std::vector<int>* cur_frame_dest_ = nullptr;
  int* cur_frame_src_ = nullptr;
  BlockingQueue<Action>* action_queue_ = nullptr;
};

#endif // CLICKITONGUE_DETECTOR_H_

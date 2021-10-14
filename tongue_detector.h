#ifndef CLICKITONGUE_TONGUE_DETECTOR_H_
#define CLICKITONGUE_TONGUE_DETECTOR_H_

#include "portaudio.h"

#include "actions.h"
#include "constants.h"
#include "detector.h"
#include "easy_fourier.h"

class TongueDetector : public Detector
{
public:
  // For training. Saves the frame indices of all detected events into
  // cur_frame_dest, and does nothing else.
  TongueDetector(BlockingQueue<Action>* action_queue,
                 double tongue_low_hz, double tongue_high_hz,
                 double tongue_hzenergy_high, double tongue_hzenergy_low,
                 int refrac_blocks, int blocksize, std::vector<int>* cur_frame_dest);

  // Kicks off 'action' at each detected event.
  TongueDetector(BlockingQueue<Action>* action_queue, Action action,
                 double tongue_low_hz, double tongue_high_hz,
                 double tongue_hzenergy_high, double tongue_hzenergy_low,
                 int refrac_blocks, int blocksize);

  void processAudio(const Sample* cur_sample, int num_frames) override;

private:
  const Action action_;

  // The range of Fourier output to consider.
  const double tongue_low_hz_; const double tongue_high_hz_;
  // The energy sum our considered range must overcome to carry out our action.
  const double tongue_hzenergy_high_;
  // The energy sum below which refractory period is allowed to count down.
  const double tongue_hzenergy_low_;
  // How long (in units of blocksize_; 2 means 2*blocksize_) we must observe low
  // energy after an event before being willing to declare a second event.
  const int refrac_blocks_;
  // Number of frames we expect to get per callback, to feed to Fourier.
  // Should be a power of 2.
  const int blocksize_;
  const EasyFourier fourier_;

  int refrac_blocks_left_ = 0;

  // only needs to be kept up to date if you plan to use RecordCurFrame
  int cur_frame_ = 0;
  bool track_cur_frame_ = false;
};

int tongueDetectorCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags, void* userData);

#endif // CLICKITONGUE_TONGUE_DETECTOR_H_

#ifndef CLICKITONGUE_TONGUE_DETECTOR_H_
#define CLICKITONGUE_TONGUE_DETECTOR_H_

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
                 double tongue_min_spikes_freq_frac,
                 double tongue_high_spike_frac,
                 double tongue_high_spike_level,
                 std::vector<int>* cur_frame_dest);

  // Kicks off 'action' at each detected event.
  TongueDetector(BlockingQueue<Action>* action_queue, Action action,
                 double tongue_low_hz, double tongue_high_hz,
                 double tongue_hzenergy_high, double tongue_hzenergy_low,
                 double tongue_min_spikes_freq_frac,
                 double tongue_high_spike_frac,
                 double tongue_high_spike_level);

  void processAudio(const Sample* cur_sample, int num_frames);
  void processFourier(const fftw_complex* fft_bins);

private:
  const Action action_;

  // The range of Fourier output to consider.
  const double tongue_low_hz_; const double tongue_high_hz_;
  // The energy sum our considered range must overcome to carry out our action.
  const double tongue_hzenergy_high_;
  // The energy sum below which refractory period is allowed to count down.
  const double tongue_hzenergy_low_;
  // Ignore the bottom min_spikes_freq_frac_ fraction of buckets when deciding
  // whether "high frequencies have too many spikes".
  const double tongue_min_spikes_freq_frac_;
  const double tongue_high_spike_frac_;
  const double tongue_high_spike_level_;

  int refrac_blocks_left_ = 0;
  int blocks_until_activation_ = -1;

  // only needs to be kept up to date if you plan to use RecordCurFrame
  int cur_frame_ = 0;
  bool track_cur_frame_ = false;
};

#endif // CLICKITONGUE_TONGUE_DETECTOR_H_

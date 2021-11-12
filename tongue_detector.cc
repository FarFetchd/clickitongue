#include "tongue_detector.h"

#include <cassert>
#include <cmath>

#include "easy_fourier.h"

TongueDetector::TongueDetector(BlockingQueue<Action>* action_queue,
                               double tongue_low_hz, double tongue_high_hz,
                               double tongue_hzenergy_high,
                               double tongue_hzenergy_low,
                               std::vector<int>* cur_frame_dest)
  : Detector(action_queue, cur_frame_dest), action_(Action::RecordCurFrame),
    tongue_low_hz_(tongue_low_hz), tongue_high_hz_(tongue_high_hz),
    tongue_hzenergy_high_(tongue_hzenergy_high),
    tongue_hzenergy_low_(tongue_hzenergy_low)
{}

TongueDetector::TongueDetector(BlockingQueue<Action>* action_queue, Action action,
                               double tongue_low_hz, double tongue_high_hz,
                               double tongue_hzenergy_high, double tongue_hzenergy_low)
  : Detector(action_queue), action_(action),
    tongue_low_hz_(tongue_low_hz), tongue_high_hz_(tongue_high_hz),
    tongue_hzenergy_high_(tongue_hzenergy_high),
    tongue_hzenergy_low_(tongue_hzenergy_low)
{
  assert(action_ != Action::RecordCurFrame);
}

void TongueDetector::processFourier(const fftw_complex* freq_power)
{
  int bin_ind = 1;
  while (tongue_low_hz_ > g_fourier->freqOfBin(bin_ind) + kBinWidth/2.0)
    bin_ind++;
  int low_bin_ind = bin_ind;
  while (tongue_high_hz_ < g_fourier->freqOfBin(bin_ind) - kBinWidth/2.0)
    bin_ind++;
  int high_bin_ind = bin_ind;

  double energy = 0;
  for (int i = low_bin_ind + 1; i < high_bin_ind; i++)
    energy += kBinWidth * freq_power[i][0];
  energy += (g_fourier->freqOfBin(low_bin_ind) + kBinWidth/2.0 - tongue_low_hz_)
            * freq_power[low_bin_ind][0];
  energy += (tongue_high_hz_ - (g_fourier->freqOfBin(high_bin_ind) - kBinWidth/2.0))
            * freq_power[high_bin_ind][0];

  if (refrac_blocks_left_ > 0)
  {
    if (energy < tongue_hzenergy_low_)
      refrac_blocks_left_--;
  }
  else if (energy > tongue_hzenergy_high_)
  {
    kickoffAction(action_);
    refrac_blocks_left_ = kRefracBlocks;
  }
}

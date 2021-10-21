#include "tongue_detector.h"

#include <cassert>
#include <cmath>

#include "easy_fourier.h"

TongueDetector::TongueDetector(BlockingQueue<Action>* action_queue,
                               double tongue_low_hz, double tongue_high_hz,
                               double tongue_hzenergy_high,
                               double tongue_hzenergy_low, int refrac_blocks,
                               std::vector<int>* cur_frame_dest)
  : Detector(action_queue), action_(Action::RecordCurFrame),
    tongue_low_hz_(tongue_low_hz), tongue_high_hz_(tongue_high_hz),
    tongue_hzenergy_high_(tongue_hzenergy_high),
    tongue_hzenergy_low_(tongue_hzenergy_low), refrac_blocks_(refrac_blocks)
{
  setCurFrameDest(cur_frame_dest);
  setCurFrameSource(&cur_frame_);
  track_cur_frame_ = true;
}

TongueDetector::TongueDetector(BlockingQueue<Action>* action_queue, Action action,
                               double tongue_low_hz, double tongue_high_hz,
                               double tongue_hzenergy_high, double tongue_hzenergy_low,
                               int refrac_blocks)
  : Detector(action_queue), action_(action),
    tongue_low_hz_(tongue_low_hz), tongue_high_hz_(tongue_high_hz),
    tongue_hzenergy_high_(tongue_hzenergy_high),
    tongue_hzenergy_low_(tongue_hzenergy_low), refrac_blocks_(refrac_blocks)
{
// TODO  assert tongue_low_hz_ not out of range, same for high
  assert(action_ != Action::RecordCurFrame);
}

void TongueDetector::processAudio(const Sample* cur_sample, int num_frames)
{
  if (num_frames != kFourierBlocksize)
  {
    printf("illegal num_frames: expected %d, got %d\n", kFourierBlocksize, num_frames);
    exit(1);
  }
  if (track_cur_frame_)
    cur_frame_ += num_frames;

  FourierLease lease = g_fourier->borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
    lease.in[i] = cur_sample[i * kNumChannels];
  lease.runFFT();

  int bin_ind = 1;
  while (tongue_low_hz_ > g_fourier->freqOfBin(bin_ind) + g_fourier->halfWidth())
    bin_ind++;
  int low_bin_ind = bin_ind;
  while (tongue_high_hz_ < g_fourier->freqOfBin(bin_ind) - g_fourier->halfWidth())
    bin_ind++;
  int high_bin_ind = bin_ind;

  double energy = 0;
  for (int i = low_bin_ind + 1; i < high_bin_ind; i++)
    energy += g_fourier->binWidth() * fabs(lease.out[i][0]);
  energy += (g_fourier->freqOfBin(low_bin_ind) + g_fourier->halfWidth() - tongue_low_hz_)
            * fabs(lease.out[low_bin_ind][0]);
  energy += (tongue_high_hz_ - (g_fourier->freqOfBin(high_bin_ind) - g_fourier->halfWidth()))
            * fabs(lease.out[high_bin_ind][0]);

//  static int print_once_per_10ms_chunks = 0;
//  if (++print_once_per_10ms_chunks == 4)
//   {
//     if (energy > 200)
//       printf("%g\n", energy > 500 ? energy : 0);
//    printEqualizerAlreadyFreq(lease.out, num_frames);
//    g_fourier->printMaxBucket(cur_sample);
//     print_once_per_10ms_chunks=0;
//   }

  if (refrac_blocks_left_ > 0)
  {
    if (energy < tongue_hzenergy_low_)
      refrac_blocks_left_--;
  }
  else
  {
    if (energy > tongue_hzenergy_high_)
    {
      kickoffAction(action_);
      refrac_blocks_left_ = refrac_blocks_;
    }
  }
}

int tongueDetectorCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags, void* userData)
{
  TongueDetector* detector = static_cast<TongueDetector*>(userData);
  const Sample* rptr = static_cast<const Sample*>(inputBuffer);
  if (inputBuffer != NULL)
    detector->processAudio(rptr, framesPerBuffer);
  return paContinue;
}

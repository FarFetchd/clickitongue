#include "tongue_detector.h"

#include <fftw3.h>

#include "equalizer.h"

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
                               double tongue_hzenergy_high,
                               double tongue_hzenergy_low, int refrac_blocks)
  : Detector(action_queue), action_(action),
    tongue_low_hz_(tongue_low_hz), tongue_high_hz_(tongue_high_hz),
    tongue_hzenergy_high_(tongue_hzenergy_high),
    tongue_hzenergy_low_(tongue_hzenergy_low), refrac_blocks_(refrac_blocks)
{
//    assert(action_ != Action::RecordCurFrame);
}

void TongueDetector::processAudio(const Sample* cur_sample, int num_frames)
{
  if (!(num_frames == 256 || num_frames == 128 || num_frames == 512))
  {
    printf("illegal num_frames: %d\n", num_frames);
    exit(1);
  }

  double* orig_real = new double[num_frames];
  for (int i=0; i<num_frames; i++)
    orig_real[i] = cur_sample[i * kNumChannels];
  int num_buckets = num_frames / 2 + 1;
  ComplexDouble* transformed = new ComplexDouble[num_buckets];

  fftw_plan fftw_forward =
    fftw_plan_dft_r2c_1d(num_frames, orig_real,
                         reinterpret_cast<fftw_complex*>(transformed),
                         FFTW_ESTIMATE); // TODO wisdom
  fftw_execute(fftw_forward);
  fftw_destroy_plan(fftw_forward);

  if (track_cur_frame_)
    cur_frame_ += num_frames;

  int bucket_ind = 0;
  while ((bucket_ind/(float)num_buckets)*44100.0f < tongue_low_hz_)
    bucket_ind++;
  int last_less_than_low = bucket_ind - 1;
  while ((bucket_ind/(float)num_buckets)*44100.0f < tongue_high_hz_)
    bucket_ind++;
  int last_less_than_high = bucket_ind - 1;

  double energy = 0;
  double bucket_width = 44100.0 / (double)num_buckets;
  for (int i = last_less_than_low + 1; i < last_less_than_high; i++)
    energy += bucket_width * fabs(transformed[bucket_ind].real());

  double low_bucket_width =
      bucket_width - (tongue_low_hz_ - (last_less_than_low /
                                        (float)num_buckets)*44100.0f);
  energy += low_bucket_width * fabs(transformed[last_less_than_low].real());
  double high_bucket_width = tongue_high_hz_ - (last_less_than_high/(float)num_buckets)*44100.0f;
  energy += high_bucket_width * fabs(transformed[last_less_than_high].real());

//   static int print_once_per_10ms_chunks = 0;
//   if (++print_once_per_10ms_chunks == 4)
//   {
//     printf("%g\n", energy > 500 ? energy : 0);
//     printEqualizerAlreadyFreq(transformed, num_frames);
//     printMaxBucket(cur_sample, num_frames);
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

  delete[] orig_real;
  delete[] transformed;
}

int tongueDetectorCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags, void* userData)
{
  TongueDetector* detector = (TongueDetector*)userData;
  const Sample* rptr = (const Sample*)inputBuffer;
  if (inputBuffer != NULL)
    detector->processAudio(rptr, framesPerBuffer);
  return paContinue;
}

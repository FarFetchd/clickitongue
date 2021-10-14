#include "blow_detector.h"

#include <cmath>

#include "easy_fourier.h"

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           float lowpass_percent, float highpass_percent,
                           double low_on_thresh, double low_off_thresh,
                           double high_on_thresh, double high_off_thresh,
                           int blocksize, std::vector<int>* cur_frame_dest)
  : Detector(action_queue), action_(Action::RecordCurFrame),
    lowpass_percent_(lowpass_percent), highpass_percent_(highpass_percent),
    low_on_thresh_(low_on_thresh), low_off_thresh_(low_off_thresh),
    high_on_thresh_(high_on_thresh), high_off_thresh_(high_off_thresh),
    blocksize_(blocksize), fourier_(blocksize_)
{
  setCurFrameDest(cur_frame_dest);
  setCurFrameSource(&cur_frame_);
  track_cur_frame_ = true;
}

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue, Action action,
                           float lowpass_percent, float highpass_percent,
                           double low_on_thresh, double low_off_thresh,
                           double high_on_thresh, double high_off_thresh,
                           int blocksize)
  : Detector(action_queue), action_(action),
    lowpass_percent_(lowpass_percent), highpass_percent_(highpass_percent),
    low_on_thresh_(low_on_thresh), low_off_thresh_(low_off_thresh),
    high_on_thresh_(high_on_thresh), high_off_thresh_(high_off_thresh),
    blocksize_(blocksize), fourier_(blocksize_)
{
//    assert(action_ != Action::RecordCurFrame);
}

void BlowDetector::processAudio(const Sample* cur_sample, int num_frames)
{
  if (num_frames != blocksize_)
  {
    printf("illegal num_frames: expected %d, got %d\n", blocksize_, num_frames);
    exit(1);
  }
  if (track_cur_frame_)
    cur_frame_ += blocksize_;

  double orig_real[blocksize_];
  for (int i=0; i<blocksize_; i++)
    orig_real[i] = cur_sample[i * kNumChannels];
  const int num_buckets = blocksize_ / 2 + 1;
  fftw_complex transformed[num_buckets];

  fourier_.doFFT(orig_real, transformed);

  static int print_once_per_10ms_chunks = 0;
  if (++print_once_per_10ms_chunks == 20)
  {
    fourier_.printEqualizerAlreadyFreq(transformed);
    print_once_per_10ms_chunks=0;
  }

  int last_low_bucket = round(lowpass_percent_ * num_buckets);
  double avg_low = 0;
  for (int i = 0; i <= last_low_bucket; i++)
    avg_low += fabs(transformed[i][0]);
  avg_low /= (double)(last_low_bucket + 1);

  int first_high_bucket = round(highpass_percent_ * num_buckets);
  double avg_high = 0;
  int high_above_1 = 0;
  for (int i = first_high_bucket; i < num_buckets; i++)
  {
    double val = fabs(transformed[i][0]);
    avg_high += val;
    if (val > 1.0)
      high_above_1++;
  }
  avg_high /= (double)(num_buckets - first_high_bucket);

  // TODO make this a configurable param
  bool many_high_spikes = (float)high_above_1 / (float)(num_buckets - first_high_bucket) > 0.5;

  if (avg_low > low_on_thresh_ && avg_high > high_on_thresh_ && many_high_spikes && !mouse_down_)
  {
    if (action_ == Action::RecordCurFrame)
      kickoffAction(Action::RecordCurFrame);
    else
      kickoffAction(Action::LeftDown);
    mouse_down_ = true;
  }
  else if (avg_low < low_off_thresh_ && avg_high < high_off_thresh_ && mouse_down_)
  {
    if (action_ != Action::RecordCurFrame)
      kickoffAction(Action::LeftUp);
    mouse_down_ = false;
  }
}

int blowDetectorCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags, void* userData)
{
  BlowDetector* detector = (BlowDetector*)userData;
  const Sample* rptr = (const Sample*)inputBuffer;
  if (inputBuffer != NULL)
    detector->processAudio(rptr, framesPerBuffer);
  return paContinue;
}
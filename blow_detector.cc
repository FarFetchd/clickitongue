#include "blow_detector.h"

#include <fftw3.h>

#include "equalizer.h"

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue,
                           float lowpass_percent, float highpass_percent,
                           double low_on_thresh, double low_off_thresh,
                           double high_on_thresh, double high_off_thresh,
                           std::vector<int>* cur_frame_dest)
  : Detector(action_queue), action_(Action::RecordCurFrame),
    lowpass_percent_(lowpass_percent), highpass_percent_(highpass_percent),
    low_on_thresh_(low_on_thresh), low_off_thresh_(low_off_thresh),
    high_on_thresh_(high_on_thresh), high_off_thresh_(high_off_thresh)
{
  setCurFrameDest(cur_frame_dest);
  setCurFrameSource(&cur_frame_);
  track_cur_frame_ = true;
}

BlowDetector::BlowDetector(BlockingQueue<Action>* action_queue, Action action,
                           float lowpass_percent, float highpass_percent,
                           double low_on_thresh, double low_off_thresh,
                           double high_on_thresh, double high_off_thresh)
  : Detector(action_queue), action_(action),
    lowpass_percent_(lowpass_percent), highpass_percent_(highpass_percent),
    low_on_thresh_(low_on_thresh), low_off_thresh_(low_off_thresh),
    high_on_thresh_(high_on_thresh), high_off_thresh_(high_off_thresh)
{
//    assert(action_ != Action::RecordCurFrame);
}

void BlowDetector::processAudio(const Sample* cur_sample, int num_frames)
{
  if (!(num_frames == 256 || num_frames == 128 || num_frames == 512 || num_frames == 1024))
    return;

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

  static int print_once_per_10ms_chunks = 0;
  if (++print_once_per_10ms_chunks == 20)
  {
    printEqualizerAlreadyFreq(transformed, num_frames);
    print_once_per_10ms_chunks=0;
  }

  if (track_cur_frame_)
    cur_frame_ += num_frames;

  int last_low_bucket = round(lowpass_percent_ * num_buckets);
  double avg_low = 0;
  for (int i = 0; i <= last_low_bucket; i++)
    avg_low += fabs(transformed[i].real());
  avg_low /= (double)(last_low_bucket + 1);

  int first_high_bucket = round(highpass_percent_ * num_buckets);
  double avg_high = 0;
  int high_above_1 = 0;
  for (int i = first_high_bucket; i < num_buckets; i++)
  {
    double val = fabs(transformed[i].real());
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
  delete[] orig_real;
  delete[] transformed;
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

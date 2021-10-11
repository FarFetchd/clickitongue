#include "ewma_trainer.h"

#include <cmath>
#include <thread>

#include "audio_input.h"
#include "getch.h"

EWMATrainer::EWMATrainer(BlockingQueue<Action>* action_queue)
  : action_queue_(action_queue), recording_done_(false) {}

void EWMATrainer::recordKeyHits(long* index_ptr)
{
  printf("recordKeyHits now listening for getch()\n");
  make_getchar_like_getch();
  while (!recording_done_)
  {
    getchar();
    if (!recording_done_)
      key_hit_indices_.push_back(*index_ptr);
  }
  resetTermios();
  printf("recordKeyHits done listening for getch()\n");
}

std::vector<int> EWMATrainer::detectEWMA(std::vector<Sample> const& s)
{
  std::vector<int> ewma_event_indices;
  EwmaDetector detector(action_queue_, ewma_alpha_, refractory_per_ms_,
                        ewma_thresh_high_, ewma_thresh_low_, &ewma_event_indices);
  detector.processAudio(s.data(), s.size() / kNumChannels);
  return ewma_event_indices;
}

unsigned long long EWMATrainer::trainingRun(std::vector<Sample> const& s)
{
  std::vector<int> ewma_event_indices = detectEWMA(s);
  if (ewma_event_indices.size() != key_hit_indices_.size())
  {
//       printf("wanted %ld hits, but we detected %ld\n", key_hit_indices_.size(),
//                                                        ewma_event_indices.size());
    return 999999999999999ull;
  }
//     printf("==============TRAINING RUN===================\n");
//     printf("ref_ms %d, thresh %d, alpha %g\n",
//            refractory_per_ms_, ewmaX100_thresh_, ewma_alpha_);

  unsigned long long rsquared = 0;
  for (int i = 0; i < key_hit_indices_.size(); i++)
  {
//       printf("EWMA sample %d, user sample %d\n",
//              ewma_event_indices[i], key_hit_indices_[i]);
    unsigned long long to_add = (ewma_event_indices[i] - key_hit_indices_[i]) *
                                (ewma_event_indices[i] - key_hit_indices_[i]);
    rsquared += to_add;
  }
  return rsquared;
}

void EWMATrainer::trainParams(std::vector<Sample> const& s)
{
  unsigned long long best_rsquared = 999999999999999ull;
  int best_refrac = -1;
  float best_thresh_low = -1;
  float best_thresh_high = -1;
  float best_alpha = -1.0;


  for (refractory_per_ms_ = 30; refractory_per_ms_ <= 100; refractory_per_ms_ += 10)
    for (ewma_thresh_low_ = 0.01; ewma_thresh_low_ <= 0.06; ewma_thresh_low_+=0.005)
      for (ewma_thresh_high_ = 0.2; ewma_thresh_high_ <= 0.5; ewma_thresh_high_ += 0.05)
        for (ewma_alpha_ = 0.05; ewma_alpha_ <= 0.4; ewma_alpha_ += 0.025)
        {
          unsigned long long rsquared = trainingRun(s);
          if (rsquared < best_rsquared)
          {
            best_rsquared = rsquared;
            best_refrac = refractory_per_ms_;
            best_thresh_low = ewma_thresh_low_;
            best_thresh_high = ewma_thresh_high_;
            best_alpha = ewma_alpha_;
          }
        }

  printf("avg variance: %g frames, avg deviation: %g ms, %ld click samples\n",
         best_rsquared/(float)key_hit_indices_.size(),
         sqrt(best_rsquared/(float)key_hit_indices_.size()) /
           (kFramesPerSec * kNumChannels),
         key_hit_indices_.size());
  printf("best refractory period: %d ms\n", best_refrac);
  printf("best threshold low: %g\n", best_thresh_low);
  printf("best threshold high: %g\n", best_thresh_high);
  printf("best EWMA alpha: %g\n", best_alpha);
}

void EWMATrainer::finishRecording()
{
  recording_done_ = true;
}

void doRecordKeyHits(EWMATrainer* me, long* index_ptr)
{
  me->recordKeyHits(index_ptr);
}

void trainMain(int seconds_to_train, BlockingQueue<Action>* action_queue)
{
  AudioInput audio_input(seconds_to_train);
  EWMATrainer ewma_trainer(action_queue);
  std::thread coord_thread(doRecordKeyHits, &ewma_trainer,
                           audio_input.frame_index_ptr());
  printf("Now recording for training!\n");
  while (audio_input.active())
    Pa_Sleep(100);

  ewma_trainer.finishRecording();
  coord_thread.join();

  ewma_trainer.trainParams(audio_input.recordedSamples());
}

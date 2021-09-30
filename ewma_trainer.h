#ifndef CLICKITONGUE_EWMA_TRAINING_H_
#define CLICKITONGUE_EWMA_TRAINING_H_

#include <atomic>
#include <vector>

#include "blocking_queue.h"
#include "ewma_detector.h"
#include "getch.h"

class EWMATrainer
{
public:
  EWMATrainer(BlockingQueue<Action>* action_queue);

  void recordKeyHits(long* index_ptr);
  void guideRhythmic(long* index_ptr);

  // writes sample frame index of each EWMA event into returned vector
  std::vector<int> detectEWMA(std::vector<Sample> const& s);

  // returns sum of squared differences between key hits and EWMA detections
  unsigned long long trainingRun(std::vector<Sample> const& s);

  void trainParams(std::vector<Sample> const& s);

  void finishRecording();

private:
  std::vector<int> key_hit_indices_; // sample frame indices
  float ewma_thresh_low_;
  float ewma_thresh_high_;
  int refractory_per_ms_;
  float ewma_alpha_;
  BlockingQueue<Action>* const action_queue_;
  std::atomic<bool> recording_done_;
};

void trainMain(int seconds_to_train, BlockingQueue<Action>* action_queue);

#endif // CLICKITONGUE_EWMA_TRAINING_H_

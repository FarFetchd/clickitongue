#include "iterative_ewma_trainer.h"

#include <unordered_map>
#include <vector>

#include "audio_input.h"
#include "ewma_detector.h"

struct TrainParams
{
public:
  TrainParams(int rpms, float etl, float eth, float alpha)
    : refractory_per_ms(rpms), ewma_thresh_low(etl), ewma_thresh_high(eth), ewma_alpha(alpha) {}

  bool operator==(TrainParams const& other) const
  {
    return refractory_per_ms == other.refractory_per_ms &&
           ewma_thresh_low == other.ewma_thresh_low &&
           ewma_thresh_high == other.ewma_thresh_high &&
           ewma_alpha == other.ewma_alpha;
  }

  int refractory_per_ms;
  float ewma_thresh_low;
  float ewma_thresh_high;
  float ewma_alpha;
};
struct TrainParamsHash
{
  size_t operator()(TrainParams const& tp) const
  {
    return std::hash<int>()(tp.refractory_per_ms) ^
           std::hash<float>()(tp.ewma_thresh_low + 11) ^
           std::hash<float>()(tp.ewma_thresh_high + 22) ^
           std::hash<float>()(tp.ewma_alpha + 33);
  }
};
using ParamViolationsMap = std::unordered_map<TrainParams, int, TrainParamsHash>;

int detectEvents(TrainParams params, std::vector<Sample> const& samples)
{
  std::vector<int> event_frames;
  EwmaDetector detector(nullptr, params.ewma_alpha, params.refractory_per_ms,
                        params.ewma_thresh_high, params.ewma_thresh_low, &event_frames);
  detector.processAudio(samples.data(), samples.size() / kNumChannels);
  return event_frames.size();
}

void oneIteration(ParamViolationsMap* violations, int desired_events)
{
  AudioInput audio_input(5);
  static int it_num = 0;
  printf("Now recording training iteration %d! Please do %d clicks.\n",
         ++it_num, desired_events);
  while (audio_input.active())
    Pa_Sleep(100);
  printf("Recording done, now computing...\n");

  for (auto it = violations->begin(); it != violations->end(); ++it)
  {
    int num_events = detectEvents(it->first, audio_input.recordedSamples());
    for (; num_events < desired_events; num_events++)
      it->second++;
    for (; num_events > desired_events; num_events--)
      it->second++;
  }
  printf("Computing done.\n");
}

void iterativeTrainMain()
{
  ParamViolationsMap violations;
  for (int refractory_per_ms = 30; refractory_per_ms <= 80; refractory_per_ms += 10)
    for (float ewma_thresh_low = 0.05; ewma_thresh_low <= 0.15; ewma_thresh_low += 0.02)
      for (float ewma_thresh_high = 0.4; ewma_thresh_high <= 0.50001; ewma_thresh_high += 0.05)
        for (float ewma_alpha = 0.05; ewma_alpha <= 0.4; ewma_alpha += 0.025)
        {
          TrainParams tp(refractory_per_ms, ewma_thresh_low, ewma_thresh_high, ewma_alpha);
          violations[tp] = 0;
        }
  oneIteration(&violations, 0);
  oneIteration(&violations, 1);
  oneIteration(&violations, 2);
  oneIteration(&violations, 3);
  oneIteration(&violations, 0);
  oneIteration(&violations, 1);
  oneIteration(&violations, 2);
  oneIteration(&violations, 3);

  int min_violations = 999999999;
  for (auto it = violations.begin(); it != violations.end(); ++it)
    min_violations = std::min(min_violations, it->second);
  std::vector<TrainParams> best;
  for (auto it = violations.begin(); it != violations.end(); ++it)
    if (it->second == min_violations)
      best.push_back(it->first);
  for (auto x : best)
  {
    printf("refractory_per_ms %d, ewma_thresh_low %g, ewma_thresh_high %g, ewma_alpha %g\n",
           x.refractory_per_ms, x.ewma_thresh_low, x.ewma_thresh_high, x.ewma_alpha);
  }
  printf("the above all had %d violations\n", min_violations);
}

#include "iterative_tongue_trainer.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "audio_recording.h"
#include "getch.h"
#include "tongue_detector.h"

namespace {

struct TrainParams
{
public:
  TrainParams(double tl, double th, double teh, double tel, int rb, int bs)
    : tongue_low_hz(tl), tongue_high_hz(th), tongue_hzenergy_high(teh),
      tongue_hzenergy_low(tel), refrac_blocks(rb), blocksize(bs) {}

  bool operator==(TrainParams const& other) const
  {
    return tongue_low_hz == other.tongue_low_hz &&
           tongue_high_hz == other.tongue_high_hz &&
           tongue_hzenergy_high == other.tongue_hzenergy_high &&
           tongue_hzenergy_low == other.tongue_hzenergy_low &&
           refrac_blocks == other.refrac_blocks &&
           blocksize == other.blocksize;
  }
  friend bool operator<(TrainParams const& l, TrainParams const& r)
  {
    return std::tie(l.tongue_low_hz, l.tongue_high_hz, l.tongue_hzenergy_high,
                    l.tongue_hzenergy_low, l.refrac_blocks, l.blocksize)
               <
           std::tie(r.tongue_low_hz, r.tongue_high_hz, r.tongue_hzenergy_high,
                    r.tongue_hzenergy_low, r.refrac_blocks, r.blocksize);
  }
  double tongue_low_hz;
  double tongue_high_hz;
  double tongue_hzenergy_high;
  double tongue_hzenergy_low;
  int refrac_blocks;
  int blocksize;
};

struct TrainParamsHash
{
  size_t operator()(TrainParams const& tp) const
  {
    return std::hash<double>()(tp.tongue_low_hz + 11) ^
           std::hash<double>()(tp.tongue_high_hz + 22) ^
           std::hash<double>()(tp.tongue_hzenergy_high + 33) ^
           std::hash<double>()(tp.tongue_hzenergy_low + 44) ^
           std::hash<int>()(tp.refrac_blocks + 55) ^
           std::hash<int>()(tp.blocksize + 66);
  }
};
using ParamViolationsMap = std::unordered_map<TrainParams, int, TrainParamsHash>;

int detectEvents(TrainParams params, std::vector<Sample> const& samples)
{
  std::vector<int> event_frames;
  TongueDetector detector(nullptr, params.tongue_low_hz, params.tongue_high_hz,
                          params.tongue_hzenergy_high, params.tongue_hzenergy_low,
                          params.refrac_blocks, params.blocksize, &event_frames);
  for (int sample_ind = 0;
       sample_ind + params.blocksize * kNumChannels < samples.size();
       sample_ind += params.blocksize * kNumChannels)
  {
    detector.processAudio(samples.data() + sample_ind, params.blocksize);
  }
  return event_frames.size();
}

ParamViolationsMap getEmptyViolations()
{
  ParamViolationsMap violations;
  for(double tongue_low_hz = 500; tongue_low_hz <= 1500; tongue_low_hz += 500)
  for(double tongue_high_hz = tongue_low_hz + 500; tongue_high_hz < 2500; tongue_high_hz += 500)
  for(double tongue_hzenergy_low = 100; tongue_hzenergy_low <= 2100; tongue_hzenergy_low += 1000)
  for(double tongue_hzenergy_high = tongue_hzenergy_low; tongue_hzenergy_high <= 4100; tongue_hzenergy_high += 1000)
  {
    int refrac_blocks = 12;
    int blocksize = 256;

    TrainParams tp(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                   tongue_hzenergy_low, refrac_blocks, blocksize);
    violations[tp] = 0;
  }
  return violations;
}

std::vector<TrainParams> computeViolations(std::vector<std::pair<RecordedAudio, int>> const& examples)
{
  ParamViolationsMap violations = getEmptyViolations();
  printf("Now computing...\n");
  for (int i = 0; i < examples.size(); i++)
  {
    for (auto it = violations.begin(); it != violations.end(); ++it)
    {
      int num_events = detectEvents(it->first, examples[i].first.samples());
      for (; num_events < examples[i].second; num_events++)
        it->second++;
      for (; num_events > examples[i].second; num_events--)
        it->second++;
    }
    printf("done with recording %d\n", i+1);
  }

  int min_violations = 999999999;
  for (auto it = violations.begin(); it != violations.end(); ++it)
    min_violations = std::min(min_violations, it->second);
  std::vector<TrainParams> best;
  for (auto it = violations.begin(); it != violations.end(); ++it)
    if (it->second == min_violations)
      best.push_back(it->first);
  printf("best of this batch have %d violations\n", min_violations);
  return best;
}

void printBest(std::vector<TrainParams> best)
{
  for (auto x : best)
  {
    printf("--tongue_low_hz=%g --tongue_high_hz=%g --tongue_hzenergy_high=%g "
           "--tongue_hzenergy_low=%g --refrac_blocks=%d --blocksize=%d\n",
           x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
           x.tongue_hzenergy_low, x.refrac_blocks, x.blocksize);
  }
}

// RecordedAudio recordExample(int desired_events)
// {
//   static int it_num = 0;
//   printf("Now recording training iteration %d! Please do %d clicks.\n",
//          ++it_num, desired_events);
//   RecordedAudio recorder(5);
//   printf("Recording done. Press any key to continue.\n");
//   make_getchar_like_getch(); getchar(); resetTermios();
//   return recorder;
// }

} // namespace

void iterativeTongueTrainMain()
{
  // for easy development of the code
  RecordedAudio clicks0a("clicks_0a.pcm");
  RecordedAudio clicks0b("clicks_0b.pcm");
  RecordedAudio clicks1a("clicks_1a.pcm");
  RecordedAudio clicks1b("clicks_1b.pcm");
  RecordedAudio clicks2a("clicks_2a.pcm");
  RecordedAudio clicks2b("clicks_2b.pcm");
  RecordedAudio clicks3a("clicks_3a.pcm");
  RecordedAudio clicks3b("clicks_3b.pcm");
  // for actual use
//   RecordedAudio clicks0a = recordExample(0);
//   RecordedAudio clicks1a = recordExample(1);
//   RecordedAudio clicks2a = recordExample(2);
//   RecordedAudio clicks3a = recordExample(3);
//   RecordedAudio clicks0b = recordExample(0);
//   RecordedAudio clicks1b = recordExample(1);
//   RecordedAudio clicks2b = recordExample(2);
//   RecordedAudio clicks3b = recordExample(3);

  RecordedAudio noise1("falls_of_fall.pcm");
  RecordedAudio noise2("brandenburg.pcm");

  std::vector<TrainParams> best_clean;
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    // TODO / NOTE: not clear if the two examples per target number of clicks
    //              is actually worth doubling the computation time...
    examples.emplace_back(clicks0a, 0);
    examples.emplace_back(clicks0b, 0);
    examples.emplace_back(clicks1a, 1);
    examples.emplace_back(clicks1b, 1);
    examples.emplace_back(clicks2a, 2);
    examples.emplace_back(clicks2b, 2);
    examples.emplace_back(clicks3a, 3);
    examples.emplace_back(clicks3b, 3);
    best_clean = computeViolations(examples);
  }
  std::vector<TrainParams> best_noise1;
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(clicks0a, 0); examples.back().first += noise1;
    examples.emplace_back(clicks0b, 0); examples.back().first += noise1;
    examples.emplace_back(clicks1a, 1); examples.back().first += noise1;
    examples.emplace_back(clicks1b, 1); examples.back().first += noise1;
    examples.emplace_back(clicks2a, 2); examples.back().first += noise1;
    examples.emplace_back(clicks2b, 2); examples.back().first += noise1;
    examples.emplace_back(clicks3a, 3); examples.back().first += noise1;
    examples.emplace_back(clicks3b, 3); examples.back().first += noise1;
    best_noise1 = computeViolations(examples);
  }
  std::vector<TrainParams> best_noise2;
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(clicks0a, 0); examples.back().first += noise2;
    examples.emplace_back(clicks0b, 0); examples.back().first += noise2;
    examples.emplace_back(clicks1a, 1); examples.back().first += noise2;
    examples.emplace_back(clicks1b, 1); examples.back().first += noise2;
    examples.emplace_back(clicks2a, 2); examples.back().first += noise2;
    examples.emplace_back(clicks2b, 2); examples.back().first += noise2;
    examples.emplace_back(clicks3a, 3); examples.back().first += noise2;
    examples.emplace_back(clicks3b, 3); examples.back().first += noise2;
    best_noise2 = computeViolations(examples);
  }
  std::sort(best_clean.begin(), best_clean.end());
  std::sort(best_noise1.begin(), best_noise1.end());
  std::sort(best_noise2.begin(), best_noise2.end());
  std::vector<TrainParams> best_01, best_02, best_12, best_012;
  std::set_intersection(best_clean.begin(), best_clean.end(),
                        best_noise1.begin(), best_noise1.end(),
                        std::back_inserter(best_01));
  std::set_intersection(best_clean.begin(), best_clean.end(),
                        best_noise2.begin(), best_noise2.end(),
                        std::back_inserter(best_02));
  std::set_intersection(best_noise1.begin(), best_noise1.end(),
                        best_noise2.begin(), best_noise2.end(),
                        std::back_inserter(best_12));
  std::set_intersection(best_01.begin(), best_01.end(),
                        best_12.begin(), best_12.end(),
                        std::back_inserter(best_012));

  if (!best_012.empty())
    printBest(best_012);
  else if (best_01.empty() && best_02.empty())
  {
    printf("falling back to best of clean only. you'll have to keep your "
           "environment as noise-free as when you recorded these examples!\n");
    printBest(best_clean);
  }
  else
  {
    printf("falling back. best of clean + noise1:\n");
    printBest(best_01);
    printf("falling back. best of clean + noise2:\n");
    printBest(best_02);
  }
}

// TODO update the other trainer to be like this one (or reuse somehow?)

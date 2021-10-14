#include "iterative_tongue_trainer.h"

#include <unordered_map>
#include <vector>

#include "audio_input.h"
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

std::unique_ptr<AudioInput> recordExample(int desired_events)
{
  auto recorder = std::make_unique<AudioInput>(5);
  static int it_num = 0;
  printf("Now recording training iteration %d! Please do %d clicks.\n",
         ++it_num, desired_events);
  while (recorder->active())
    Pa_Sleep(100);
  recorder->closeStream();
  printf("Recording done. Press any key to continue.\n");
  make_getchar_like_getch(); getchar(); resetTermios();
  return recorder;
}

} // namespace

void iterativeTongueTrainMain()
{
  ParamViolationsMap violations;
  for(double tongue_low_hz = 500; tongue_low_hz <= 1500; tongue_low_hz += 500)
  for(double tongue_high_hz = tongue_low_hz + 500; tongue_high_hz < 2500; tongue_high_hz += 500)
  for(double tongue_hzenergy_low = 100; tongue_hzenergy_low <= 2100; tongue_hzenergy_low += 1000)
  for(double tongue_hzenergy_high = tongue_hzenergy_low; tongue_hzenergy_high <= 4100; tongue_hzenergy_high += 1000)
  {
    int refrac_blocks = 6;
    int blocksize = 256;

    TrainParams tp(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                   tongue_hzenergy_low, refrac_blocks, blocksize);
    violations[tp] = 0;
  }

  std::vector<std::unique_ptr<AudioInput>> examples;
  examples.push_back(recordExample(0));
  examples.push_back(recordExample(1));
  examples.push_back(recordExample(2));
  examples.push_back(recordExample(3));
  examples.push_back(recordExample(0));
  examples.push_back(recordExample(1));
  examples.push_back(recordExample(2));
  examples.push_back(recordExample(3));

  printf("Now computing...\n");
  for (int i = 0; i < examples.size(); i++)
  {
    for (auto it = violations.begin(); it != violations.end(); ++it)
    {
      int num_events = detectEvents(it->first, examples[i]->recordedSamples());
      for (; num_events < i % 4; num_events++)
        it->second++;
      for (; num_events > i % 4; num_events--)
        it->second++;
    }
    printf("done with recording %d\n", i+1);
  }

  // TODO add standard noise (people talking, music, ...)
  // TODO pick best (or best couple) and do a finer grid search around them
  // TODO update the other trainer to be like this one (or reuse somehow?)

  int min_violations = 999999999;
  for (auto it = violations.begin(); it != violations.end(); ++it)
    min_violations = std::min(min_violations, it->second);
  std::vector<TrainParams> best;
  for (auto it = violations.begin(); it != violations.end(); ++it)
    if (it->second == min_violations)
      best.push_back(it->first);
  for (auto x : best)
  {
    printf("tongue_low_hz %g, tongue_high_hz %g, tongue_hzenergy_high %g, "
           "tongue_hzenergy_low %g, refrac_blocks %d, blocksize %d\n",
           x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
           x.tongue_hzenergy_low, x.refrac_blocks, x.blocksize);
  }
  printf("the above all had %d violations\n", min_violations);
}

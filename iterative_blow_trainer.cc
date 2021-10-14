#include "iterative_blow_trainer.h"

#include <unordered_map>
#include <vector>

#include "audio_input.h"
#include "blow_detector.h"

namespace {

struct TrainParams
{
public:
  TrainParams(float lpp, float hpp, double lont, double lofft,
              double hont, double hofft, int bs)
    : lowpass_percent(lpp), highpass_percent(hpp), low_on_thresh(lont),
      low_off_thresh(lofft), high_on_thresh(hont), high_off_thresh(hofft),
      blocksize(bs) {}

  bool operator==(TrainParams const& other) const
  {
    return lowpass_percent == other.lowpass_percent &&
           highpass_percent == other.highpass_percent &&
           low_on_thresh == other.low_on_thresh &&
           low_off_thresh == other.low_off_thresh &&
           high_on_thresh == other.high_on_thresh &&
           high_off_thresh == other.high_off_thresh &&
           blocksize == other.blocksize;
  }
  float lowpass_percent;
  float highpass_percent;
  double low_on_thresh;
  double low_off_thresh;
  double high_on_thresh;
  double high_off_thresh;
  int blocksize;
};
struct TrainParamsHash
{
  size_t operator()(TrainParams const& tp) const
  {
    return std::hash<float>()(tp.lowpass_percent + 11) ^
           std::hash<float>()(tp.highpass_percent + 22) ^
           std::hash<double>()(tp.low_on_thresh + 33) ^
           std::hash<double>()(tp.low_off_thresh + 44) ^
           std::hash<double>()(tp.high_on_thresh + 55) ^
           std::hash<double>()(tp.high_off_thresh + 66) ^
           std::hash<int>()(tp.blocksize + 77);
  }
};
using ParamViolationsMap = std::unordered_map<TrainParams, int, TrainParamsHash>;

int detectEvents(TrainParams params, std::vector<Sample> const& samples)
{
  std::vector<int> event_frames;
  BlowDetector detector(nullptr, params.lowpass_percent, params.highpass_percent,
                        params.low_on_thresh, params.low_off_thresh,
                        params.high_on_thresh, params.high_off_thresh,
                        params.blocksize, &event_frames);
  for (int sample_ind = 0;
       sample_ind + params.blocksize * kNumChannels < samples.size();
       sample_ind += params.blocksize * kNumChannels)
  {
    detector.processAudio(samples.data() + sample_ind, params.blocksize);
  }
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

} // namespace

void iterativeBlowTrainMain()
{
  ParamViolationsMap violations;

  float lowpass_percent = 0.035;//for(float lowpass_percent = 0.035; lowpass_percent <= 0.05; lowpass_percent += 0.015)
  float highpass_percent = 0.5;//for(float highpass_percent = 0.6; highpass_percent <= 0.8; highpass_percent += 0.2)
  double low_off_thresh = 3;//for(double low_off_thresh = 2; low_off_thresh <= 6; low_off_thresh += 2)
  for(double low_on_thresh = 7; low_on_thresh <= 15; low_on_thresh += 4)
  for(double high_on_thresh = 0.5; high_on_thresh <= 0.75; high_on_thresh += 0.25)
  for(double high_off_thresh = 0.1; high_off_thresh <= 0.2; high_off_thresh += 0.1)
  for(int blocksize = 128; blocksize <= 256; blocksize *= 2)
  {
    TrainParams tp(lowpass_percent, highpass_percent, low_on_thresh,
                   low_off_thresh, high_on_thresh, high_off_thresh, blocksize);
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
    printf("x.lowpass_percent %g, x.highpass_percent %g, x.low_on_thresh %g, "
           "x.low_off_thresh %g, x.high_on_thresh %g, x.high_off_thresh %g, "
           "x.blocksize %d\n",
           x.lowpass_percent, x.highpass_percent, x.low_on_thresh,
           x.low_off_thresh, x.high_on_thresh, x.high_off_thresh,
           x.blocksize);
  }
  printf("the above all had %d violations\n", min_violations);
}

#include "iterative_tongue_trainer.h"

#include <cassert>
#include <algorithm>
#include <random>
#include <set>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "getch.h"
#include "tongue_detector.h"

namespace {

struct TrainParams
{
public:
  TrainParams(double tl, double th, double teh, double tel, int rb)
    : tongue_low_hz(tl), tongue_high_hz(th), tongue_hzenergy_high(teh),
      tongue_hzenergy_low(tel), refrac_blocks(rb) {}

  bool operator==(TrainParams const& other) const
  {
    return tongue_low_hz == other.tongue_low_hz &&
           tongue_high_hz == other.tongue_high_hz &&
           tongue_hzenergy_high == other.tongue_hzenergy_high &&
           tongue_hzenergy_low == other.tongue_hzenergy_low &&
           refrac_blocks == other.refrac_blocks;
  }
  bool roughlyEquals(TrainParams const& other) const
  {
    return fabs(tongue_low_hz - other.tongue_low_hz) < 2.0 &&
           fabs(tongue_high_hz - other.tongue_high_hz) < 4.0 &&
           fabs(tongue_hzenergy_high - other.tongue_hzenergy_high) < 10.0 &&
           fabs(tongue_hzenergy_low - other.tongue_hzenergy_low) < 1.0 &&
           refrac_blocks == other.refrac_blocks;
  }
  friend bool operator<(TrainParams const& l, TrainParams const& r)
  {
    assert(!l.score.empty());
    assert(!r.score.empty());
    assert(l.score.size() == r.score.size());
    int l_sum = std::accumulate(l.score.begin(), l.score.end(), 0);
    int r_sum = std::accumulate(r.score.begin(), r.score.end(), 0);
    if (l_sum < r_sum)
      return true;
    else if (l_sum > r_sum)
      return false;
    for (int i=0; i<l.score.size(); i++)
    {
      if (l.score[i] < r.score[i])
        return true;
    }
    return false;
  }

  int detectEvents(std::vector<Sample> const& samples)
  {
    std::vector<int> event_frames;
    TongueDetector detector(nullptr, tongue_low_hz, tongue_high_hz,
                            tongue_hzenergy_high, tongue_hzenergy_low,
                            refrac_blocks, &event_frames);
    for (int sample_ind = 0;
        sample_ind + kFourierBlocksize * kNumChannels < samples.size();
        sample_ind += kFourierBlocksize * kNumChannels)
    {
      detector.processAudio(samples.data() + sample_ind, kFourierBlocksize);
    }
    return event_frames.size();
  }

  // for each example-set, detectEvents on each example, and sum up that sets'
  // total violations. the score is a vector of those violations, one count per example-set.
  void computeScore(std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
  {
    score.clear();
    for (auto const& examples : example_sets)
    {
      int violations = 0;
      for (auto const& example_and_expected : examples)
      {
        int events = detectEvents(example_and_expected.first.samples());
        violations += abs(events - example_and_expected.second);
      }
      score.push_back(violations);
    }
  }
  // (score)
  std::string toString()
  {
    std::string ret = "{";
    for (int x : score)
      ret += std::to_string(x) + ",";
    return ret + "}";
  }
  void printParams()
  {
    printf("--tongue_low_hz=%g --tongue_high_hz=%g --tongue_hzenergy_high=%g "
           "--tongue_hzenergy_low=%g --refrac_blocks=%d\n",
           tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
           tongue_hzenergy_low, refrac_blocks);
  }

  double tongue_low_hz;
  double tongue_high_hz;
  double tongue_hzenergy_high;
  double tongue_hzenergy_low;
  int refrac_blocks;
  std::vector<int> score;
};

std::random_device* getRandomDev()
{
  std::random_device* dev = new std::random_device;
  return dev;
}
class RandomStuff
{
public:
  RandomStuff(double low, double high)
    : mt_((*getRandomDev())()), dist_(low, high) {}

  double random() { return dist_(mt_); }

private:
  std::mt19937 mt_;
  std::uniform_real_distribution<double> dist_;
};

const double kMinLowHz = 300;
const double kMaxLowHz = 2500;
const double kMinHighHz = 700;
const double kMaxHighHz = 5500;
const double kMinLowEnergy = 100;
const double kMaxLowEnergy = 4100;
const double kMinHighEnergy = 300;
const double kMaxHighEnergy = 16000;
double randomLowHz()
{
  static RandomStuff* r = new RandomStuff(kMinHighHz, kMaxLowHz);
  return r->random();
}
double randomHighHz(double low)
{
  static RandomStuff* r = new RandomStuff(kMinHighHz, kMaxHighHz);
  double ret = low;
  while (ret - 100 < low)
    ret = r->random();
  return ret;
}
double randomLowEnergy()
{
  static RandomStuff* r = new RandomStuff(kMinLowEnergy, kMaxLowEnergy);
  return r->random();
}
double randomHighEnergy(double low)
{
  static RandomStuff* r = new RandomStuff(kMinHighEnergy, kMaxHighEnergy);
  double ret = low;
  while (ret - 100 < low)
    ret = r->random();
  return ret;
}

void runComputeScore(
    TrainParams* me,
    std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
{
  me->computeScore(example_sets);
}

class TrainParamsCocoon
{
public:
  TrainParamsCocoon(
      double tongue_low_hz, double tongue_high_hz, double tongue_hzenergy_high,
      double tongue_hzenergy_low, int refrac_blocks,
      std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
          tongue_hzenergy_low, refrac_blocks)),
    score_computer_(std::make_unique<std::thread>(runComputeScore, pupa_.get(), example_sets)) {}

  TrainParams awaitHatch() { score_computer_->join(); return *pupa_; }

private:
  std::unique_ptr<TrainParams> pupa_;
  std::unique_ptr<std::thread> score_computer_;
};

#define KEEP_IN_BOUNDS(mn,x,mx) do { x = std::min(x, mx); x = std::max(x, mn); } while(false)

class TrainParamsFactory
{
public:
  TrainParamsFactory(std::vector<std::vector<std::pair<RecordedAudio, int>>> const& examples_sets)
  : examples_sets_(examples_sets) {}

  TrainParamsCocoon randomParams()
  {
    double tongue_low_hz = randomLowHz();
    double tongue_high_hz = randomHighHz(tongue_low_hz);
    double tongue_hzenergy_low = randomLowEnergy();
    double tongue_hzenergy_high = randomHighEnergy(tongue_hzenergy_low);
    int refrac_blocks = 12;
    return TrainParamsCocoon(
        tongue_low_hz, tongue_high_hz, tongue_hzenergy_high, tongue_hzenergy_low,
        refrac_blocks, examples_sets_);
  }

  std::vector<TrainParamsCocoon> startingSet()
  {
    int refrac_blocks = 12;
    std::vector<TrainParamsCocoon> ret;
    for (double tongue_low_hz = kMinLowHz + 0.25*(kMaxLowHz-kMinLowHz);
                tongue_low_hz <= kMinLowHz + 0.75*(kMaxLowHz-kMinLowHz);
                tongue_low_hz += 0.5*(kMaxLowHz-kMinLowHz))
    for (double tongue_high_hz = kMinHighHz + 0.25*(kMaxHighHz-kMinHighHz);
                tongue_high_hz <= kMinHighHz + 0.75*(kMaxHighHz-kMinHighHz);
                tongue_high_hz += 0.5*(kMaxHighHz-kMinHighHz))
    for (double tongue_hzenergy_high = kMinHighEnergy + 0.25*(kMaxHighEnergy-kMinHighEnergy);
                tongue_hzenergy_high <= kMinHighEnergy + 0.75*(kMaxHighEnergy-kMinHighEnergy);
                tongue_hzenergy_high += 0.5*(kMaxHighEnergy-kMinHighEnergy))
    for (double tongue_hzenergy_low = kMinLowEnergy + 0.25*(kMaxLowEnergy-kMinLowEnergy);
                tongue_hzenergy_low <= kMinLowEnergy + 0.75*(kMaxLowEnergy-kMinLowEnergy);
                tongue_hzenergy_low += 0.5*(kMaxLowEnergy-kMinLowEnergy))
    {
      if (tongue_low_hz < tongue_high_hz && tongue_hzenergy_low < tongue_hzenergy_high)
      {
        ret.emplace_back(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                         tongue_hzenergy_low, refrac_blocks, examples_sets_);
      }
    }
    ret.emplace_back((kMaxLowHz-kMinLowHz)/2.0, (kMaxHighHz-kMinHighHz)/2.0,
                     (kMaxHighEnergy-kMinHighEnergy)/2.0,
                     (kMaxLowEnergy-kMinLowEnergy)/2.0, refrac_blocks, examples_sets_);
    for (int i = 0; i < 16; i++)
      ret.push_back(randomParams());
    return ret;
  }

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_tongue_low_hz = x.tongue_low_hz - (kMaxLowHz-kMinLowHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowHz, left_tongue_low_hz, kMaxLowHz);
    ret.emplace_back(
        left_tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.refrac_blocks, examples_sets_);
    double rite_tongue_low_hz = x.tongue_low_hz + (kMaxLowHz-kMinLowHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowHz, rite_tongue_low_hz, kMaxLowHz);
    if (rite_tongue_low_hz < x.tongue_high_hz) ret.emplace_back(
        rite_tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.refrac_blocks, examples_sets_);

    double left_tongue_high_hz = x.tongue_high_hz - (kMaxHighHz-kMinHighHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighHz, left_tongue_high_hz, kMaxHighHz);
    if (x.tongue_low_hz < left_tongue_high_hz) ret.emplace_back(
        x.tongue_low_hz, left_tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.refrac_blocks, examples_sets_);
    double rite_tongue_high_hz = x.tongue_high_hz + (kMaxHighHz-kMinHighHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighHz, rite_tongue_high_hz, kMaxHighHz);
    ret.emplace_back(
        x.tongue_low_hz, rite_tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.refrac_blocks, examples_sets_);

    double left_hzenergy_high = x.tongue_hzenergy_high - (kMaxHighEnergy-kMinHighEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighEnergy, left_hzenergy_high, kMaxHighEnergy);
    if (x.tongue_hzenergy_low < left_hzenergy_high) ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, left_hzenergy_high,
        x.tongue_hzenergy_low, x.refrac_blocks, examples_sets_);
    double rite_hzenergy_high = x.tongue_hzenergy_high + (kMaxHighEnergy-kMinHighEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighEnergy, rite_hzenergy_high, kMaxHighEnergy);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, rite_hzenergy_high,
        x.tongue_hzenergy_low, x.refrac_blocks, examples_sets_);

    double left_hzenergy_low = x.tongue_hzenergy_low - (kMaxLowEnergy-kMinLowEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowEnergy, left_hzenergy_low, kMaxLowEnergy);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        left_hzenergy_low, x.refrac_blocks, examples_sets_);
    double rite_hzenergy_low = x.tongue_hzenergy_low + (kMaxLowEnergy-kMinLowEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowEnergy, rite_hzenergy_low, kMaxLowEnergy);
    if (rite_hzenergy_low < x.tongue_hzenergy_high) ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        rite_hzenergy_low, x.refrac_blocks, examples_sets_);

    return ret;
  }

  void shrinkSteps() { pattern_divisor_ *= 2.0; }

private:
  // A vector of example-sets. Each example-set is a vector of samples of audio,
  // paired with how many events are expected to be in that audio.
  std::vector<std::vector<std::pair<RecordedAudio, int>>> const& examples_sets_;
  // Each variable's offset will be (kVarMax-kVarMin)/pattern_divisor_
  double pattern_divisor_ = 4.0;
};

RecordedAudio recordExample(int desired_events)
{
  const int kSecondsToRecord = 4;
  if (desired_events == 0)
  {
    printf("About to record! Will record for %d seconds. During that time, please "
           "don't do ANY tongue clicks; this is just to record your typical "
           "background sounds. Press any key when you are ready to start.\n",
           kSecondsToRecord);
  }
  else
  {
    printf("About to record! Will record for %d seconds. During that time, please "
           "do %d tongue clicks. Press any key when you are ready to start.\n",
           kSecondsToRecord, desired_events);
  }
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("Now recording..."); fflush(stdout);
  RecordedAudio recorder(kSecondsToRecord);
  printf("recording done.\n");
  return recorder;
}

constexpr bool DOING_DEVELOPMENT_TESTING = false;
} // namespace

void iterativeTongueTrainMain()
{
  std::vector<RecordedAudio> audio_examples;
  if (DOING_DEVELOPMENT_TESTING) // for easy development of the code
  {
    audio_examples.emplace_back("clicks_0a.pcm");
    audio_examples.emplace_back("clicks_1a.pcm");
    audio_examples.emplace_back("clicks_2a.pcm");
    audio_examples.emplace_back("clicks_3a.pcm");
  }
  else // for actual use
  {
    audio_examples.push_back(recordExample(0));
    audio_examples.push_back(recordExample(1));
    audio_examples.push_back(recordExample(2));
    audio_examples.push_back(recordExample(3));
  }

  std::vector<std::string> noise_names = {"falls_of_fall.pcm", "brandenburg.pcm"};
  std::vector<RecordedAudio> noises;
  if (DOING_DEVELOPMENT_TESTING)
    for (auto name : noise_names)
      noises.emplace_back(name);

  std::vector<std::vector<std::pair<RecordedAudio, int>>> examples_sets;
  // (first, add examples without any noise)
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(audio_examples[0], 0);
    examples.emplace_back(audio_examples[1], 1);
    examples.emplace_back(audio_examples[2], 2);
    examples.emplace_back(audio_examples[3], 3);
    examples_sets.push_back(examples);
  }
  for (auto const& noise : noises) // now a set of our examples for each noise
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(audio_examples[0], 0); examples.back().first.scale(0.7); examples.back().first += noise;
    examples.emplace_back(audio_examples[1], 1); examples.back().first.scale(0.7); examples.back().first += noise;
    examples.emplace_back(audio_examples[2], 2); examples.back().first.scale(0.7); examples.back().first += noise;
    examples.emplace_back(audio_examples[3], 3); examples.back().first.scale(0.7); examples.back().first += noise;
    examples_sets.push_back(examples);
  }
  // finally, a quiet version, for a challenge/tie breaker
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(audio_examples[0], 0); examples.back().first.scale(0.3);
    examples.emplace_back(audio_examples[1], 1); examples.back().first.scale(0.3);
    examples.emplace_back(audio_examples[2], 2); examples.back().first.scale(0.3);
    examples.emplace_back(audio_examples[3], 3); examples.back().first.scale(0.3);
    examples_sets.push_back(examples);
  }

  TrainParamsFactory factory(examples_sets);

  printf("beginning optimization computations...\n");
  // The optimization algorithm: first, seed with 10 random points.
  std::vector<TrainParamsCocoon> starting_cocoons = factory.startingSet();
  std::set<TrainParams> candidates;
  for (auto& cocoon : starting_cocoons)
    candidates.insert(cocoon.awaitHatch());

  int shrinks = 0;
  while (true)
  {
    auto it = candidates.begin();
    TrainParams best = *it;
    TrainParams second = *(++it);

    // begin parallel computations
    std::vector<TrainParamsCocoon> step_cocoons1 = factory.patternAround(best);
    // gather results of parallel computations
    for (auto& cocoon : step_cocoons1)
      candidates.insert(cocoon.awaitHatch());

    auto new_it = candidates.begin();
    TrainParams new_best = *new_it;
    TrainParams new_second = *(++new_it);
    if (new_best == best)
    {
      shrinks++;
      factory.shrinkSteps();
    }

    printf("current best: ");
    best.printParams();
    if (DOING_DEVELOPMENT_TESTING)
    {
      printf("scores: best: %s, 2nd: %s\n",
            best.toString().c_str(), new_second.toString().c_str());
    }

    if (shrinks >= 5)
      break; // we're probably close to an optimum
  }
  printf("converged; done.\n");
}

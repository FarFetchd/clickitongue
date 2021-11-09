#include "train_tongue.h"

#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "interaction.h"
#include "tongue_detector.h"

namespace {

class TrainParams
{
public:
  TrainParams(double tl, double th, double teh, double tel)
    : tongue_low_hz(tl), tongue_high_hz(th), tongue_hzenergy_high(teh),
      tongue_hzenergy_low(tel) {}

  bool operator==(TrainParams const& other) const
  {
    return tongue_low_hz == other.tongue_low_hz &&
           tongue_high_hz == other.tongue_high_hz &&
           tongue_hzenergy_high == other.tongue_hzenergy_high &&
           tongue_hzenergy_low == other.tongue_hzenergy_low;
  }
  friend bool operator<(TrainParams const& l, TrainParams const& r)
  {
    assert(!l.score.empty());
    assert(!r.score.empty());
    assert(l.score.size() == r.score.size());

    // the first (i.e. noiseless) example set is the most important, even
    // more important than sum of violations across all examples.
    if (l.score[0] < r.score[0])
        return true;

    int l_sum = std::accumulate(l.score.begin(), l.score.end(), 0);
    int r_sum = std::accumulate(r.score.begin(), r.score.end(), 0);
    if (l_sum < r_sum)
      return true;
    else if (l_sum > r_sum)
      return false;

    for (int i=1; i<l.score.size(); i++)
      if (l.score[i] < r.score[i])
        return true;

    return false;
  }

  int detectEvents(std::vector<Sample> const& samples)
  {
    std::vector<int> event_frames;
    TongueDetector detector(nullptr, tongue_low_hz, tongue_high_hz,
                            tongue_hzenergy_high, tongue_hzenergy_low,
                            &event_frames);
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
  void computeScore(std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
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
           "--tongue_hzenergy_low=%g\n",
           tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
           tongue_hzenergy_low);
  }

  double tongue_low_hz;
  double tongue_high_hz;
  double tongue_hzenergy_high;
  double tongue_hzenergy_low;
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

const double kMinLowHz = 500;
const double kMaxLowHz = 1500;
const double kMinHighHz = 1000;
const double kMaxHighHz = 4000;
const double kMinLowEnergy = 50;
const double kMaxLowEnergy = 2000;
const double kMinHighEnergy = 1000;
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
    std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
{
  me->computeScore(example_sets);
}

class TrainParamsCocoon
{
public:
  TrainParamsCocoon(
      double tongue_low_hz, double tongue_high_hz, double tongue_hzenergy_high,
      double tongue_hzenergy_low,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
          tongue_hzenergy_low)),
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
  TrainParamsFactory(std::vector<std::pair<AudioRecording, int>> const& raw_examples,
                     std::vector<std::string> const& noise_fnames)
  {
    std::vector<AudioRecording> noises;
    for (auto name : noise_fnames)
      noises.emplace_back(name);

    // (first, add the base examples, without any noise)
    examples_sets_.push_back(raw_examples);
    for (auto const& noise : noises) // now a set of our examples for each noise
    {
      std::vector<std::pair<AudioRecording, int>> examples = raw_examples;
      for (auto& x : examples)
        x.first += noise;
      examples_sets_.push_back(examples);
    }
    // finally, a quiet version, for a challenge/tie breaker
    {
      std::vector<std::pair<AudioRecording, int>> examples = raw_examples;
      for (auto& x : examples)
        x.first.scale(0.5);
      examples_sets_.push_back(examples);
    }
  }

  TrainParamsCocoon randomParams()
  {
    double tongue_low_hz = randomLowHz();
    double tongue_high_hz = randomHighHz(tongue_low_hz);
    double tongue_hzenergy_low = randomLowEnergy();
    double tongue_hzenergy_high = randomHighEnergy(tongue_hzenergy_low);
    return TrainParamsCocoon(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                             tongue_hzenergy_low, examples_sets_);
  }

  std::vector<TrainParamsCocoon> startingSet()
  {
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
                         tongue_hzenergy_low, examples_sets_);
      }
    }
    ret.emplace_back((kMaxLowHz-kMinLowHz)/2.0, (kMaxHighHz-kMinHighHz)/2.0,
                     (kMaxHighEnergy-kMinHighEnergy)/2.0,
                     (kMaxLowEnergy-kMinLowEnergy)/2.0, examples_sets_);
    for (int i = 0; i < 15; i++)
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
        x.tongue_hzenergy_low, examples_sets_);
    double rite_tongue_low_hz = x.tongue_low_hz + (kMaxLowHz-kMinLowHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowHz, rite_tongue_low_hz, kMaxLowHz);
    if (rite_tongue_low_hz < x.tongue_high_hz) ret.emplace_back(
        rite_tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, examples_sets_);

    double left_tongue_high_hz = x.tongue_high_hz - (kMaxHighHz-kMinHighHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighHz, left_tongue_high_hz, kMaxHighHz);
    if (x.tongue_low_hz < left_tongue_high_hz) ret.emplace_back(
        x.tongue_low_hz, left_tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, examples_sets_);
    double rite_tongue_high_hz = x.tongue_high_hz + (kMaxHighHz-kMinHighHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighHz, rite_tongue_high_hz, kMaxHighHz);
    ret.emplace_back(
        x.tongue_low_hz, rite_tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, examples_sets_);

    double left_hzenergy_high = x.tongue_hzenergy_high - (kMaxHighEnergy-kMinHighEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighEnergy, left_hzenergy_high, kMaxHighEnergy);
    if (x.tongue_hzenergy_low < left_hzenergy_high) ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, left_hzenergy_high,
        x.tongue_hzenergy_low, examples_sets_);
    double rite_hzenergy_high = x.tongue_hzenergy_high + (kMaxHighEnergy-kMinHighEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighEnergy, rite_hzenergy_high, kMaxHighEnergy);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, rite_hzenergy_high,
        x.tongue_hzenergy_low, examples_sets_);

    double left_hzenergy_low = x.tongue_hzenergy_low - (kMaxLowEnergy-kMinLowEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowEnergy, left_hzenergy_low, kMaxLowEnergy);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        left_hzenergy_low, examples_sets_);
    double rite_hzenergy_low = x.tongue_hzenergy_low + (kMaxLowEnergy-kMinLowEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowEnergy, rite_hzenergy_low, kMaxLowEnergy);
    if (rite_hzenergy_low < x.tongue_hzenergy_high) ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        rite_hzenergy_low, examples_sets_);

    return ret;
  }

  void shrinkSteps() { pattern_divisor_ *= 2.0; }

private:
  // A vector of example-sets. Each example-set is a vector of samples of audio,
  // paired with how many events are expected to be in that audio.
  std::vector<std::vector<std::pair<AudioRecording, int>>> examples_sets_;
  // Each variable's offset will be (kVarMax-kVarMin)/pattern_divisor_
  double pattern_divisor_ = 4.0;
};

// the following is actual code, not just a header:
#include "train_common.h"

} // namespace

AudioRecording recordExampleTongue(int desired_events)
{
  return recordExampleCommon(desired_events, "tongue clicks", "tongue click");
}

TongueConfig trainTongue(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                         Action action, bool verbose)
{
  std::vector<std::string> noise_fnames =
      {"data/noise1.pcm", "data/noise2.pcm", "data/noise3.pcm"};

  TrainParamsFactory factory(audio_examples, noise_fnames);
  TrainParams best = patternSearch(factory, verbose);

  TongueConfig ret;
  ret.action = action;
  ret.tongue_low_hz = best.tongue_low_hz;
  ret.tongue_high_hz = best.tongue_high_hz;
  ret.tongue_hzenergy_high = best.tongue_hzenergy_high;
  ret.tongue_hzenergy_low = best.tongue_hzenergy_low;

  ret.enabled = (best.score[0] == 0 && best.score[1] < 2);
  return ret;
}

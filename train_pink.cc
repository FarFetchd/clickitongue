#include "train_pink.h"

#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "pink_detector.h"
#include "fft_result_distributor.h"
#include "interaction.h"

namespace {

class TrainParams
{
public:
  TrainParams(double o5on, double o5off, double o6on, double o6off,
              double o7on, double o7off, double alpha)
  : o5_on_thresh(o5on), o5_off_thresh(o5off), o6_on_thresh(o6on),
    o6_off_thresh(o6off), o7_on_thresh(o7on), o7_off_thresh(o7off),
    ewma_alpha(alpha) {}

  bool operator==(TrainParams const& other) const
  {
    return o5_on_thresh == other.o5_on_thresh &&
           o5_off_thresh == other.o5_off_thresh &&
           o6_on_thresh == other.o6_on_thresh &&
           o6_off_thresh == other.o6_off_thresh &&
           o7_on_thresh == other.o7_on_thresh &&
           o7_off_thresh == other.o7_off_thresh &&
           ewma_alpha == other.ewma_alpha;
  }
  friend bool operator<(TrainParams const& l, TrainParams const& r)
  {
    assert(!l.score.empty());
    assert(!r.score.empty());
    assert(l.score.size() == r.score.size());

    // the first (i.e. noiseless) example set is the most important, even
    // more important than sum of violations across all examples.
    if (l.score[0] != r.score[0])
      return l.score[0] < r.score[0];

    int l_sum = std::accumulate(l.score.begin(), l.score.end(), 0);
    int r_sum = std::accumulate(r.score.begin(), r.score.end(), 0);
    if (l_sum != r_sum)
      return l_sum < r_sum;

    for (int i=1; i<l.score.size(); i++)
      if (l.score[i] < r.score[i])
        return true;

    return false;
  }

  int detectEvents(std::vector<Sample> const& samples)
  {
    std::vector<int> event_frames;
    std::vector<std::unique_ptr<Detector>> just_one_detector;
    just_one_detector.emplace_back(std::make_unique<PinkDetector>(
        nullptr, o5_on_thresh, o5_off_thresh, o6_on_thresh, o6_off_thresh,
        o7_on_thresh, o7_off_thresh, ewma_alpha, &event_frames));

    FFTResultDistributor wrapper(std::move(just_one_detector));

    for (int sample_ind = 0;
         sample_ind + kFourierBlocksize * kNumChannels < samples.size();
         sample_ind += kFourierBlocksize * kNumChannels)
    {
      wrapper.processAudio(samples.data() + sample_ind, kFourierBlocksize);
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
  std::string toString() const
  {
    std::string ret = "{";
    for (int x : score)
      ret += std::to_string(x) + ",";
    return ret + "}";
  }
  void printParams()
  {
    printf("--o5_on_thresh=%g --o5_off_thresh=%g --o6_on_thresh=%g "
           "--o6_off_thresh=%g --o7_on_thresh=%g --o7_off_thresh=%g "
           "--ewma_alpha=%g\n",
           o5_on_thresh, o5_off_thresh, o6_on_thresh, o6_off_thresh,
           o7_on_thresh, o7_off_thresh, ewma_alpha);
  }

  double o5_on_thresh;
  double o5_off_thresh;
  double o6_on_thresh;
  double o6_off_thresh;
  double o7_on_thresh;
  double o7_off_thresh;
  double ewma_alpha;

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

const double kMinO5On = 100;
const double kMaxO5On = 1000;
const double kMinO5Off = 20;
const double kMaxO5Off = 200;
const double kMinO6On = 50;
const double kMaxO6On = 500;
const double kMinO6Off = 10;
const double kMaxO6Off = 100;
const double kMinO7On = 20;
const double kMaxO7On = 200;
const double kMinO7Off = 5;
const double kMaxO7Off = 50;
const double kMinAlpha = 0.05;
const double kMaxAlpha = 0.5;

double randomO5On()
{
  static RandomStuff* r = new RandomStuff(kMinO5On, kMaxO5On);
  return r->random();
}
double randomO5Off()
{
  static RandomStuff* r = new RandomStuff(kMinO5Off, kMaxO5Off);
  return r->random();
}
double randomO6On()
{
  static RandomStuff* r = new RandomStuff(kMinO6On, kMaxO6On);
  return r->random();
}
double randomO6Off()
{
  static RandomStuff* r = new RandomStuff(kMinO6Off, kMaxO6Off);
  return r->random();
}
double randomO7On()
{
  static RandomStuff* r = new RandomStuff(kMinO7On, kMaxO7On);
  return r->random();
}
double randomO7Off()
{
  static RandomStuff* r = new RandomStuff(kMinO7Off, kMaxO7Off);
  return r->random();
}
double randomAlpha()
{
  static RandomStuff* r = new RandomStuff(kMinAlpha, kMaxAlpha);
  return r->random();
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
      double o5_on_thresh, double o5_off_thresh, double o6_on_thresh,
      double o6_off_thresh, double o7_on_thresh, double o7_off_thresh,
      double ewma_alpha,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(o5_on_thresh, o5_off_thresh, o6_on_thresh,
                                        o6_off_thresh, o7_on_thresh, o7_off_thresh, ewma_alpha)),
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
        x.first.scale(0.75);
      examples_sets_.push_back(examples);
    }
  }

  bool emplaceIfValid(
      std::vector<TrainParamsCocoon>& ret, double o5_on_thresh, double o5_off_thresh,
      double o6_on_thresh, double o6_off_thresh, double o7_on_thresh,
      double o7_off_thresh, double ewma_alpha)
  {
    if (o5_off_thresh < o5_on_thresh && o6_off_thresh < o6_on_thresh &&
        o7_off_thresh < o7_on_thresh)
    {
      ret.emplace_back(o5_on_thresh, o5_off_thresh, o6_on_thresh, o6_off_thresh,
                       o7_on_thresh, o7_off_thresh, ewma_alpha, examples_sets_);
      return true;
    }
    return false;
  }

  void emplaceRandomParams(std::vector<TrainParamsCocoon>& ret)
  {
    while (true)
    {
      double o5_on_thresh = randomO5On();
      double o5_off_thresh = randomO5Off();
      double o6_on_thresh = randomO6On();
      double o6_off_thresh = randomO6Off();
      double o7_on_thresh = randomO7On();
      double o7_off_thresh = randomO7Off();
      double ewma_alpha = randomAlpha();
      if (emplaceIfValid(ret, o5_on_thresh, o5_off_thresh, o6_on_thresh,
                         o6_off_thresh, o7_on_thresh, o7_off_thresh, ewma_alpha))
      {
        break;
      }
    }
  }

#define HIDEOUS_FOR(x, mn, mx) for (double x = mn + 0.25*( mx - mn ); x <= mn + 0.75*( mx - mn ); x += 0.5*( mx - mn ))

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    HIDEOUS_FOR(o5_on_thresh, kMinO5On, kMaxO5On)
    HIDEOUS_FOR(o5_off_thresh, kMinO5Off, kMaxO5Off)
    HIDEOUS_FOR(o6_on_thresh, kMinO6On, kMaxO6On)
    HIDEOUS_FOR(o6_off_thresh, kMinO6Off, kMaxO6Off)
    HIDEOUS_FOR(o7_on_thresh, kMinO7On, kMaxO7On)
    HIDEOUS_FOR(o7_off_thresh, kMinO7Off, kMaxO7Off)
    HIDEOUS_FOR(ewma_alpha, kMinAlpha, kMaxAlpha)
    {
      emplaceIfValid(ret, o5_on_thresh, o5_off_thresh, o6_on_thresh,
                     o6_off_thresh, o7_on_thresh, o7_off_thresh, ewma_alpha);
    }

    ret.emplace_back(0.5*(kMaxO5On-kMinO5On),
                     0.5*(kMaxO5Off-kMinO5Off),
                     0.5*(kMaxO6On-kMinO6On),
                     0.5*(kMaxO6Off-kMinO6Off),
                     0.5*(kMaxO7On-kMinO7On),
                     0.5*(kMaxO7Off-kMinO7Off),
                     0.5*(kMaxAlpha-kMinAlpha),
                     examples_sets_);
    for (int i = 0; i < 15; i++)
      emplaceRandomParams(ret);
    return ret;
  }

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_o5_on_thresh = x.o5_on_thresh - (kMaxO5On-kMinO5On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO5On, left_o5_on_thresh, kMaxO5On);
    emplaceIfValid(ret, left_o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);
    double rite_o5_on_thresh = x.o5_on_thresh + (kMaxO5On-kMinO5On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO5On, rite_o5_on_thresh, kMaxO5On);
    emplaceIfValid(ret, rite_o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);

    double left_o5_off_thresh = x.o5_off_thresh - (kMaxO5Off-kMinO5Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO5Off, left_o5_off_thresh, kMaxO5Off);
    emplaceIfValid(ret, x.o5_on_thresh, left_o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);
    double rite_o5_off_thresh = x.o5_off_thresh + (kMaxO5Off-kMinO5Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO5Off, rite_o5_off_thresh, kMaxO5Off);
    emplaceIfValid(ret, x.o5_on_thresh, rite_o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);

    double left_o6_on_thresh = x.o6_on_thresh - (kMaxO6On-kMinO6On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6On, left_o6_on_thresh, kMaxO6On);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, left_o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);
    double rite_o6_on_thresh = x.o6_on_thresh + (kMaxO6On-kMinO6On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6On, rite_o6_on_thresh, kMaxO6On);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, rite_o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);

    double left_o6_off_thresh = x.o6_off_thresh - (kMaxO6Off-kMinO6Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Off, left_o6_off_thresh, kMaxO6Off);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   left_o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);
    double rite_o6_off_thresh = x.o6_off_thresh + (kMaxO6Off-kMinO6Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Off, rite_o6_off_thresh, kMaxO6Off);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   rite_o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);

    double left_o7_on_thresh = x.o7_on_thresh - (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, left_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, left_o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);
    double rite_o7_on_thresh = x.o7_on_thresh + (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, rite_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, rite_o7_on_thresh, x.o7_off_thresh, x.ewma_alpha);

    double left_o7_off_thresh = x.o7_off_thresh - (kMaxO7Off-kMinO7Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7Off, left_o7_off_thresh, kMaxO7Off);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, left_o7_off_thresh, x.ewma_alpha);
    double rite_o7_off_thresh = x.o7_off_thresh + (kMaxO7Off-kMinO7Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7Off, rite_o7_off_thresh, kMaxO7Off);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, rite_o7_off_thresh, x.ewma_alpha);

    double left_ewma_alpha = x.ewma_alpha - (kMaxAlpha-kMinAlpha)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinAlpha, left_ewma_alpha, kMaxAlpha);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, left_ewma_alpha);
    double rite_ewma_alpha = x.ewma_alpha + (kMaxAlpha-kMinAlpha)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinAlpha, rite_ewma_alpha, kMaxAlpha);
    emplaceIfValid(ret, x.o5_on_thresh, x.o5_off_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, rite_ewma_alpha);

    // and some random points within the pattern grid, the idea being that if
    // there are two params that only improve when changed together, pattern
    // search would miss it. TODO trim? or use?
//     for (int i=0; i<10; i++)
//     {
//       double lowpass_percent = randomBetween(left_lowpass_percent, rite_lowpass_percent);
//       double highpass_percent = randomBetween(left_highpass_percent, rite_highpass_percent);
//       double low_off_thresh = randomBetween(left_low_off_thresh, rite_low_off_thresh);
//       double low_on_thresh = randomBetween(left_low_on_thresh, rite_low_on_thresh);
//       double high_off_thresh = randomBetween(left_high_off_thresh, rite_high_off_thresh);
//       double high_on_thresh = randomBetween(left_high_on_thresh, rite_high_on_thresh);
//       double high_spike_frac = randomBetween(left_high_spike_frac, rite_high_spike_frac);
//       double high_spike_level = randomBetween(left_high_spike_level, rite_high_spike_level);
//       ret.emplace_back(lowpass_percent, highpass_percent, low_off_thresh, low_on_thresh,
//                        high_off_thresh, high_on_thresh, high_spike_frac, high_spike_level, examples_sets_);
//     }

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

AudioRecording recordExamplePink(int desired_events)
{
  return recordExampleCommon(desired_events, "blowing", "blow on the mic");
}

PinkConfig trainPink(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                     bool verbose)
{
  std::vector<std::string> noise_fnames =
      {"data/noise1.pcm", "data/noise2.pcm", "data/noise3.pcm"};

  TrainParamsFactory factory(audio_examples, noise_fnames);
  TrainParams best = patternSearch(factory, verbose);

  PinkConfig ret;
  ret.action_on = Action::LeftDown;
  ret.action_off = Action::LeftUp;
  ret.o5_on_thresh = best.o5_on_thresh;
  ret.o5_off_thresh = best.o5_off_thresh;
  ret.o6_on_thresh = best.o6_on_thresh;
  ret.o6_off_thresh = best.o6_off_thresh;
  ret.o7_on_thresh = best.o7_on_thresh;
  ret.o7_off_thresh = best.o7_off_thresh;
  ret.ewma_alpha = best.ewma_alpha;

  ret.enabled = (best.score[0] == 0 && best.score[1] < 3);
  return ret;
}

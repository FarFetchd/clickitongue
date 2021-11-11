#include "train_blow.h"

#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "blow_detector.h"
#include "fft_result_distributor.h"
#include "interaction.h"

namespace {

class TrainParams
{
public:
  TrainParams(double lpp, double hpp, double lont, double lofft,
              double hont, double hofft, double hsf, double hsl)
  : lowpass_percent(lpp), highpass_percent(hpp), low_on_thresh(lont),
    low_off_thresh(lofft), high_on_thresh(hont), high_off_thresh(hofft),
    high_spike_frac(hsf), high_spike_level(hsl) {}

  bool operator==(TrainParams const& other) const
  {
    return lowpass_percent == other.lowpass_percent &&
           highpass_percent == other.highpass_percent &&
           low_on_thresh == other.low_on_thresh &&
           low_off_thresh == other.low_off_thresh &&
           high_on_thresh == other.high_on_thresh &&
           high_off_thresh == other.high_off_thresh &&
           high_spike_frac == other.high_spike_frac &&
           high_spike_level == other.high_spike_level;
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
    just_one_detector.emplace_back(std::make_unique<BlowDetector>(
        nullptr, lowpass_percent, highpass_percent, low_on_thresh,
        low_off_thresh, high_on_thresh, high_off_thresh, high_spike_frac,
        high_spike_level, &event_frames));

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
    printf("--lowpass_percent=%g --highpass_percent=%g --low_on_thresh=%g "
           "--low_off_thresh=%g --high_on_thresh=%g --high_off_thresh=%g "
           "--high_spike_frac=%g --high_spike_level=%g\n",
           lowpass_percent, highpass_percent, low_on_thresh, low_off_thresh,
           high_on_thresh, high_off_thresh, high_spike_frac, high_spike_level);
  }

  double lowpass_percent;
  double highpass_percent;
  double low_on_thresh;
  double low_off_thresh;
  double high_on_thresh;
  double high_off_thresh;
  double high_spike_frac;
  double high_spike_level;
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
double randomBetween(double a, double b) { return RandomStuff(a, b).random(); }

const double kMinLowPassPercent = 0.03;
const double kMaxLowPassPercent = 0.1;
const double kMinHighPassPercent = 0.3;
const double kMaxHighPassPercent = 0.8;
const double kMinLowOffThresh = 1;
const double kMaxLowOffThresh = 8;
const double kMinLowOnThresh = 6;
const double kMaxLowOnThresh = 20;
const double kMinHighOffThresh = 0.05;
const double kMaxHighOffThresh = 1;
const double kMinHighOnThresh = 0.2;
const double kMaxHighOnThresh = 2;
const double kMinHighSpikeFrac = 0.1;
const double kMaxHighSpikeFrac = 0.9;
const double kMinHighSpikeLevel = 0.2;
const double kMaxHighSpikeLevel = 2;

double randomLowPassPercent()
{
  static RandomStuff* r = new RandomStuff(kMinLowPassPercent, kMaxLowPassPercent);
  return r->random();
}
double randomHighPassPercent(double low)
{
  static RandomStuff* r = new RandomStuff(kMinHighPassPercent, kMaxHighPassPercent);
  double ret = low;
  while (ret - 0.05 < low)
    ret = r->random();
  return ret;
}
double randomLowOffThresh()
{
  static RandomStuff* r = new RandomStuff(kMinLowOffThresh, kMaxLowOffThresh);
  return r->random();
}
double randomLowOnThresh(double low)
{
  static RandomStuff* r = new RandomStuff(kMinLowOnThresh, kMaxLowOnThresh);
  double ret = low;
  while (ret - 1 < low)
    ret = r->random();
  return ret;
}
double randomHighOffThresh()
{
  static RandomStuff* r = new RandomStuff(kMinHighOffThresh, kMaxHighOffThresh);
  return r->random();
}
double randomHighOnThresh(double low)
{
  static RandomStuff* r = new RandomStuff(kMinHighOnThresh, kMaxHighOnThresh);
  double ret = low;
  while (ret - 1 < low)
    ret = r->random();
  return ret;
}
double randomHighSpikeFrac()
{
  static RandomStuff* r = new RandomStuff(kMinHighSpikeFrac, kMaxHighSpikeFrac);
  return r->random();
}
double randomHighSpikeLevel()
{
  static RandomStuff* r = new RandomStuff(kMinHighSpikeLevel, kMaxHighSpikeLevel);
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
      double lowpass_percent, double highpass_percent, double low_on_thresh,
      double low_off_thresh, double high_on_thresh, double high_off_thresh,
      double high_spike_frac, double high_spike_level,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(lowpass_percent, highpass_percent,
          low_on_thresh, low_off_thresh, high_on_thresh, high_off_thresh,
          high_spike_frac, high_spike_level)),
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

  TrainParamsCocoon randomParams()
  {
    double lowpass_percent = randomLowPassPercent();
    double highpass_percent = randomHighPassPercent(lowpass_percent);
    double low_off_thresh = randomLowOffThresh();
    double low_on_thresh = randomLowOnThresh(low_off_thresh);
    double high_off_thresh = randomHighOffThresh();
    double high_on_thresh = randomHighOnThresh(high_off_thresh);
    double high_spike_frac = randomHighSpikeFrac();
    double high_spike_level = randomHighSpikeLevel();

    return TrainParamsCocoon(
        lowpass_percent, highpass_percent, low_on_thresh, low_off_thresh,
        high_on_thresh, high_off_thresh, high_spike_frac, high_spike_level,
        examples_sets_);
  }

#define HIDEOUS_FOR(x, mn, mx) for (double x = mn + 0.25*( mx - mn ); x <= mn + 0.75*( mx - mn ); x += 0.5*( mx - mn ))

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    HIDEOUS_FOR(lowpass_percent, kMinLowPassPercent, kMaxLowPassPercent)
    HIDEOUS_FOR(highpass_percent, kMinHighPassPercent, kMaxHighPassPercent)
    HIDEOUS_FOR(low_off_thresh, kMinLowOffThresh, kMaxLowOffThresh)
    HIDEOUS_FOR(low_on_thresh, kMinLowOnThresh, kMaxLowOnThresh)
    HIDEOUS_FOR(high_on_thresh, kMinHighOnThresh, kMaxHighOnThresh)
    HIDEOUS_FOR(high_off_thresh, kMinHighOffThresh, kMaxHighOffThresh)
    HIDEOUS_FOR(high_spike_frac, kMinHighSpikeFrac, kMaxHighSpikeFrac)
    HIDEOUS_FOR(high_spike_level, kMinHighSpikeLevel, kMaxHighSpikeLevel)
    {
      if (lowpass_percent < highpass_percent && low_off_thresh < low_on_thresh &&
          high_off_thresh < high_on_thresh)
      {
        ret.emplace_back(lowpass_percent, highpass_percent, low_on_thresh,
                         low_off_thresh, high_on_thresh, high_off_thresh,
                         high_spike_frac, high_spike_level, examples_sets_);
      }
    }
    ret.emplace_back(0.5*(kMaxLowPassPercent-kMinLowPassPercent),
                     0.5*(kMaxHighPassPercent-kMinHighPassPercent),
                     0.5*(kMaxLowOffThresh-kMinLowOffThresh),
                     0.5*(kMaxLowOnThresh-kMinLowOnThresh),
                     0.5*(kMaxHighOnThresh-kMinHighOnThresh),
                     0.5*(kMaxHighOffThresh-kMinHighOffThresh),
                     0.5*(kMaxHighSpikeFrac-kMinHighSpikeFrac),
                     0.5*(kMaxHighSpikeLevel-kMinHighSpikeLevel),
                     examples_sets_);
    for (int i = 0; i < 15; i++)
      ret.push_back(randomParams());
    return ret;
  }

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_lowpass_percent = x.lowpass_percent - (kMaxLowPassPercent-kMinLowPassPercent)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowPassPercent, left_lowpass_percent, kMaxLowPassPercent);
    ret.emplace_back(
        left_lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);
    double rite_lowpass_percent = x.lowpass_percent + (kMaxLowPassPercent-kMinLowPassPercent)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowPassPercent, rite_lowpass_percent, kMaxLowPassPercent);
    if (rite_lowpass_percent < x.highpass_percent) ret.emplace_back(
        rite_lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);

    double left_highpass_percent = x.highpass_percent - (kMaxHighPassPercent-kMinHighPassPercent)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighPassPercent, left_highpass_percent, kMaxHighPassPercent);
    if (x.lowpass_percent < left_highpass_percent) ret.emplace_back(
        x.lowpass_percent, left_highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);
    double rite_highpass_percent = x.highpass_percent + (kMaxHighPassPercent-kMinHighPassPercent)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighPassPercent, rite_highpass_percent, kMaxHighPassPercent);
    ret.emplace_back(
        x.lowpass_percent, rite_highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);

    double left_low_off_thresh = x.low_off_thresh - (kMaxLowOffThresh-kMinLowOffThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowOffThresh, left_low_off_thresh, kMaxLowOffThresh);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, left_low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);
    double rite_low_off_thresh = x.low_off_thresh + (kMaxLowOffThresh-kMinLowOffThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowOffThresh, rite_low_off_thresh, kMaxLowOffThresh);
    if (rite_low_off_thresh < x.low_on_thresh) ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, rite_low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);

    double left_low_on_thresh = x.low_on_thresh - (kMaxLowOnThresh-kMinLowOnThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowOnThresh, left_low_on_thresh, kMaxLowOnThresh);
    if (x.low_off_thresh < left_low_on_thresh) ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, left_low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);
    double rite_low_on_thresh = x.low_on_thresh + (kMaxLowOnThresh-kMinLowOnThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowOnThresh, rite_low_on_thresh, kMaxLowOnThresh);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, rite_low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);

    double left_high_off_thresh = x.high_off_thresh - (kMaxHighOffThresh-kMinHighOffThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighOffThresh, left_high_off_thresh, kMaxHighOffThresh);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, left_high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);
    double rite_high_off_thresh = x.high_off_thresh + (kMaxHighOffThresh-kMinHighOffThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighOffThresh, rite_high_off_thresh, kMaxHighOffThresh);
    if (rite_high_off_thresh < x.high_on_thresh) ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, rite_high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);

    double left_high_on_thresh = x.high_on_thresh - (kMaxHighOnThresh-kMinHighOnThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighOnThresh, left_high_on_thresh, kMaxHighOnThresh);
    if (x.high_off_thresh < left_high_on_thresh) ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        left_high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);
    double rite_high_on_thresh = x.high_on_thresh + (kMaxHighOnThresh-kMinHighOnThresh)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighOnThresh, rite_high_on_thresh, kMaxHighOnThresh);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        rite_high_on_thresh, x.high_off_thresh, x.high_spike_frac, x.high_spike_level,
        examples_sets_);

    double left_high_spike_frac = x.high_spike_frac - (kMaxHighSpikeFrac-kMinHighSpikeFrac)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighSpikeFrac, left_high_spike_frac, kMaxHighSpikeFrac);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, left_high_spike_frac, x.high_spike_level,
        examples_sets_);
    double rite_high_spike_frac = x.high_spike_frac + (kMaxHighSpikeFrac-kMinHighSpikeFrac)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighSpikeFrac, rite_high_spike_frac, kMaxHighSpikeFrac);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, rite_high_spike_frac, x.high_spike_level,
        examples_sets_);

    double left_high_spike_level = x.high_spike_level - (kMaxHighSpikeLevel-kMinHighSpikeLevel)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighSpikeLevel, left_high_spike_level, kMaxHighSpikeLevel);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, left_high_spike_level,
        examples_sets_);
    double rite_high_spike_level = x.high_spike_level + (kMaxHighSpikeLevel-kMinHighSpikeLevel)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighSpikeLevel, rite_high_spike_level, kMaxHighSpikeLevel);
    ret.emplace_back(
        x.lowpass_percent, x.highpass_percent, x.low_on_thresh, x.low_off_thresh,
        x.high_on_thresh, x.high_off_thresh, x.high_spike_frac, rite_high_spike_level,
        examples_sets_);

    // and some random points within the pattern grid, the idea being that if
    // there are two params that only improve when changed together, pattern
    // search would miss it.
    for (int i=0; i<10; i++)
    {
      double lowpass_percent = randomBetween(left_lowpass_percent, rite_lowpass_percent);
      double highpass_percent = randomBetween(left_highpass_percent, rite_highpass_percent);
      double low_off_thresh = randomBetween(left_low_off_thresh, rite_low_off_thresh);
      double low_on_thresh = randomBetween(left_low_on_thresh, rite_low_on_thresh);
      double high_off_thresh = randomBetween(left_high_off_thresh, rite_high_off_thresh);
      double high_on_thresh = randomBetween(left_high_on_thresh, rite_high_on_thresh);
      double high_spike_frac = randomBetween(left_high_spike_frac, rite_high_spike_frac);
      double high_spike_level = randomBetween(left_high_spike_level, rite_high_spike_level);
      ret.emplace_back(lowpass_percent, highpass_percent, low_off_thresh, low_on_thresh,
                       high_off_thresh, high_on_thresh, high_spike_frac, high_spike_level, examples_sets_);
    }

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

AudioRecording recordExampleBlow(int desired_events)
{
  return recordExampleCommon(desired_events, "blowing", "blow on the mic");
}

BlowConfig trainBlow(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                     bool verbose)
{
  std::vector<std::string> noise_fnames =
      {"data/noise1.pcm", "data/noise2.pcm", "data/noise3.pcm"};

  TrainParamsFactory factory(audio_examples, noise_fnames);
  TrainParams best = patternSearch(factory, verbose);

  BlowConfig ret;
  ret.action_on = Action::LeftDown;
  ret.action_off = Action::LeftUp;
  ret.lowpass_percent = best.lowpass_percent;
  ret.highpass_percent = best.highpass_percent;
  ret.low_on_thresh = best.low_on_thresh;
  ret.low_off_thresh = best.low_off_thresh;
  ret.high_on_thresh = best.high_on_thresh;
  ret.high_off_thresh = best.high_off_thresh;
  ret.high_spike_frac = best.high_spike_frac;
  ret.high_spike_level = best.high_spike_level;

  ret.enabled = (best.score[0] < 2 && best.score[1] < 3);
  return ret;
}

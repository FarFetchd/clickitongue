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
  TrainParams(double tl, double th, double teh, double tel,
              double tmsff, double thsf, double thsl)
    : tongue_low_hz(tl), tongue_high_hz(th), tongue_hzenergy_high(teh),
      tongue_hzenergy_low(tel), tongue_min_spikes_freq_frac(tmsff),
      tongue_high_spike_frac(thsf), tongue_high_spike_level(thsl) {}

  bool operator==(TrainParams const& other) const
  {
    return tongue_low_hz == other.tongue_low_hz &&
           tongue_high_hz == other.tongue_high_hz &&
           tongue_hzenergy_high == other.tongue_hzenergy_high &&
           tongue_hzenergy_low == other.tongue_hzenergy_low &&
           tongue_min_spikes_freq_frac == other.tongue_min_spikes_freq_frac &&
           tongue_high_spike_frac == other.tongue_high_spike_frac &&
           tongue_high_spike_level == other.tongue_high_spike_level;
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
    TongueDetector detector(
        nullptr, tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
        tongue_hzenergy_low, tongue_min_spikes_freq_frac, tongue_high_spike_frac,
        tongue_high_spike_level, &event_frames);
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
           "--tongue_hzenergy_low=%g --tongue_min_spikes_freq_frac=%g "
           "--tongue_high_spike_frac=%g --tongue_high_spike_level=%g\n",
           tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
           tongue_hzenergy_low, tongue_min_spikes_freq_frac,
           tongue_high_spike_frac, tongue_high_spike_level);
  }

  double tongue_low_hz;
  double tongue_high_hz;
  double tongue_hzenergy_high;
  double tongue_hzenergy_low;
  double tongue_min_spikes_freq_frac;
  double tongue_high_spike_frac;
  double tongue_high_spike_level;
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
const double kMinSpikesFreqFrac = 0.25;
const double kMaxSpikesFreqFrac = 0.85;
const double kMinSpikeFrac = 0.2;
const double kMaxSpikeFrac = 0.8;
const double kMinSpikeLevel = 0.3;
const double kMaxSpikeLevel = 3.0;
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
double randomSpikesFreqFrac()
{
  static RandomStuff* r = new RandomStuff(kMinSpikesFreqFrac, kMaxSpikesFreqFrac);
  return r->random();
}
double randomSpikeFrac()
{
  static RandomStuff* r = new RandomStuff(kMinSpikeFrac, kMaxSpikeFrac);
  return r->random();
}
double randomSpikeLevel()
{
  static RandomStuff* r = new RandomStuff(kMinSpikeLevel, kMaxSpikeLevel);
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
      double tongue_low_hz, double tongue_high_hz, double tongue_hzenergy_high,
      double tongue_hzenergy_low, double tongue_min_spikes_freq_frac,
      double tongue_high_spike_frac, double tongue_high_spike_level,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(
          tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
          tongue_hzenergy_low, tongue_min_spikes_freq_frac,
          tongue_high_spike_frac, tongue_high_spike_level)),
    score_computer_(std::make_unique<std::thread>(runComputeScore, pupa_.get(),
                                                  example_sets))
    {}

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
    double tongue_min_spikes_freq_frac = randomSpikesFreqFrac();
    double tongue_high_spike_frac = randomSpikeFrac();
    double tongue_high_spike_level = randomSpikeLevel();
    return TrainParamsCocoon(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                             tongue_hzenergy_low, tongue_min_spikes_freq_frac,
                             tongue_high_spike_frac, tongue_high_spike_level,
                             examples_sets_);
  }

#define HIDEOUS_FOR(x, mn, mx) for (double x = mn + 0.25*( mx - mn ); x <= mn + 0.75*( mx - mn ); x += 0.5*( mx - mn ))

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    HIDEOUS_FOR(tongue_low_hz, kMinLowHz, kMaxLowHz)
    HIDEOUS_FOR(tongue_high_hz, kMinHighHz, kMaxHighHz)
    HIDEOUS_FOR(tongue_hzenergy_high, kMinHighEnergy, kMaxHighEnergy)
    HIDEOUS_FOR(tongue_hzenergy_low, kMinLowEnergy, kMaxLowEnergy)
    HIDEOUS_FOR(tongue_low_hz, kMinLowHz, kMaxLowHz)
    HIDEOUS_FOR(tongue_min_spikes_freq_frac, kMinSpikesFreqFrac, kMaxSpikesFreqFrac)
    HIDEOUS_FOR(tongue_high_spike_frac, kMinSpikeFrac, kMaxSpikeFrac)
    HIDEOUS_FOR(tongue_high_spike_level, kMinSpikeLevel, kMaxSpikeLevel)
    {
      if (tongue_low_hz < tongue_high_hz && tongue_hzenergy_low < tongue_hzenergy_high)
      {
        ret.emplace_back(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                         tongue_hzenergy_low, tongue_min_spikes_freq_frac,
                         tongue_high_spike_frac, tongue_high_spike_level,
                         examples_sets_);
      }
    }
    ret.emplace_back((kMaxLowHz-kMinLowHz)/2.0, (kMaxHighHz-kMinHighHz)/2.0,
                     (kMaxHighEnergy-kMinHighEnergy)/2.0,
                     (kMaxLowEnergy-kMinLowEnergy)/2.0,
                     (kMaxSpikesFreqFrac-kMinSpikesFreqFrac)/2.0,
                     (kMaxSpikeFrac-kMinSpikeFrac)/2.0,
                     (kMaxSpikeLevel-kMinSpikeLevel)/2.0, examples_sets_);
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
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);
    double rite_tongue_low_hz = x.tongue_low_hz + (kMaxLowHz-kMinLowHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowHz, rite_tongue_low_hz, kMaxLowHz);
    if (rite_tongue_low_hz < x.tongue_high_hz) ret.emplace_back(
        rite_tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);

    double left_tongue_high_hz = x.tongue_high_hz - (kMaxHighHz-kMinHighHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighHz, left_tongue_high_hz, kMaxHighHz);
    if (x.tongue_low_hz < left_tongue_high_hz) ret.emplace_back(
        x.tongue_low_hz, left_tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);
    double rite_tongue_high_hz = x.tongue_high_hz + (kMaxHighHz-kMinHighHz)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighHz, rite_tongue_high_hz, kMaxHighHz);
    ret.emplace_back(
        x.tongue_low_hz, rite_tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);

    double left_hzenergy_high = x.tongue_hzenergy_high - (kMaxHighEnergy-kMinHighEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighEnergy, left_hzenergy_high, kMaxHighEnergy);
    if (x.tongue_hzenergy_low < left_hzenergy_high) ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, left_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);
    double rite_hzenergy_high = x.tongue_hzenergy_high + (kMaxHighEnergy-kMinHighEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinHighEnergy, rite_hzenergy_high, kMaxHighEnergy);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, rite_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);

    double left_hzenergy_low = x.tongue_hzenergy_low - (kMaxLowEnergy-kMinLowEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowEnergy, left_hzenergy_low, kMaxLowEnergy);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        left_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);
    double rite_hzenergy_low = x.tongue_hzenergy_low + (kMaxLowEnergy-kMinLowEnergy)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinLowEnergy, rite_hzenergy_low, kMaxLowEnergy);
    if (rite_hzenergy_low < x.tongue_hzenergy_high) ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        rite_hzenergy_low, x.tongue_min_spikes_freq_frac, x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);

    double left_spikes_freq_frac = x.tongue_min_spikes_freq_frac - (kMaxSpikesFreqFrac-kMinSpikesFreqFrac)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinSpikesFreqFrac, left_spikes_freq_frac, kMaxSpikesFreqFrac);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, left_spikes_freq_frac,
        x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);
    double rite_spikes_freq_frac = x.tongue_min_spikes_freq_frac + (kMaxSpikesFreqFrac-kMinSpikesFreqFrac)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinSpikesFreqFrac, rite_spikes_freq_frac, kMaxSpikesFreqFrac);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, rite_spikes_freq_frac,
        x.tongue_high_spike_frac, x.tongue_high_spike_level, examples_sets_);

    double left_spike_frac = x.tongue_high_spike_frac - (kMaxSpikeFrac-kMinSpikeFrac)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinSpikeFrac, left_spike_frac, kMaxSpikeFrac);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac,
        left_spike_frac, x.tongue_high_spike_level, examples_sets_);
    double rite_spike_frac = x.tongue_high_spike_frac + (kMaxSpikeFrac-kMinSpikeFrac)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinSpikeFrac, rite_spike_frac, kMaxSpikeFrac);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac,
        rite_spike_frac, x.tongue_high_spike_level, examples_sets_);

    double left_spike_level = x.tongue_high_spike_level - (kMaxSpikeLevel-kMinSpikeLevel)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinSpikeLevel, left_spike_level, kMaxSpikeLevel);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac,
        x.tongue_high_spike_frac, left_spike_level, examples_sets_);
    double rite_spike_level = x.tongue_high_spike_level + (kMaxSpikeLevel-kMinSpikeLevel)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinSpikeLevel, rite_spike_level, kMaxSpikeLevel);
    ret.emplace_back(
        x.tongue_low_hz, x.tongue_high_hz, x.tongue_hzenergy_high,
        x.tongue_hzenergy_low, x.tongue_min_spikes_freq_frac,
        x.tongue_high_spike_frac, rite_spike_level, examples_sets_);

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
  ret.tongue_min_spikes_freq_frac = best.tongue_min_spikes_freq_frac;
  ret.tongue_high_spike_frac = best.tongue_high_spike_frac;
  ret.tongue_high_spike_level = best.tongue_high_spike_level;

  ret.enabled = (best.score[0] == 0 && best.score[1] < 2);
  return ret;
}

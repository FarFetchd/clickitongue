#include "train_blow.h"

#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "getch.h"
#include "blow_detector.h"

namespace {

constexpr bool DOING_DEVELOPMENT_TESTING = false;

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
    int l_sum = std::accumulate(l.score.begin(), l.score.end(), 0);
    int r_sum = std::accumulate(r.score.begin(), r.score.end(), 0);
    if (l_sum < r_sum)
      return true;
    else if (l_sum > r_sum)
      return false;

    for (int i=0; i<l.score.size(); i++)
      if (l.score[i] < r.score[i])
        return true;

    return false;
  }

  int detectEvents(std::vector<Sample> const& samples)
  {
    std::vector<int> event_frames;
    BlowDetector detector(nullptr, lowpass_percent, highpass_percent,
                          low_on_thresh, low_off_thresh, high_on_thresh,
                          high_off_thresh, high_spike_frac, high_spike_level,
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
    std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
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
      std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
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
  TrainParamsFactory(std::vector<std::pair<RecordedAudio, int>> const& raw_examples,
                     std::vector<std::string> const& noise_fnames)
  {
    std::vector<RecordedAudio> noises;
    if (DOING_DEVELOPMENT_TESTING)
      for (auto name : noise_fnames)
        noises.emplace_back(name);

    // (first, add the base examples, without any noise)
    examples_sets_.push_back(raw_examples);
    for (auto const& noise : noises) // now a set of our examples for each noise
    {
      std::vector<std::pair<RecordedAudio, int>> examples = raw_examples;
      for (auto& x : examples)
      {
        x.first.scale(0.7);
        x.first += noise;
      }
      examples_sets_.push_back(examples);
    }
    // finally, a quiet version, for a challenge/tie breaker
    {
      std::vector<std::pair<RecordedAudio, int>> examples = raw_examples;
      for (auto& x : examples)
        x.first.scale(0.3);
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

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    for (double lowpass_percent = kMinLowPassPercent + 0.25*(kMaxLowPassPercent-kMinLowPassPercent);
                lowpass_percent <= kMinLowPassPercent + 0.75*(kMaxLowPassPercent-kMinLowPassPercent);
                lowpass_percent += 0.5*(kMaxLowPassPercent-kMinLowPassPercent))
    for (double highpass_percent = kMinHighPassPercent + 0.25*(kMaxHighPassPercent-kMinHighPassPercent);
                highpass_percent <= kMinHighPassPercent + 0.75*(kMaxHighPassPercent-kMinHighPassPercent);
                highpass_percent += 0.5*(kMaxHighPassPercent-kMinHighPassPercent))
    for (double low_off_thresh = kMinLowOffThresh + 0.25*(kMaxLowOffThresh-kMinLowOffThresh);
                low_off_thresh <= kMinLowOffThresh + 0.75*(kMaxLowOffThresh-kMinLowOffThresh);
                low_off_thresh += 0.5*(kMaxLowOffThresh-kMinLowOffThresh))
    for (double low_on_thresh = kMinLowOnThresh + 0.25*(kMaxLowOnThresh-kMinLowOnThresh);
                low_on_thresh <= kMinLowOnThresh + 0.75*(kMaxLowOnThresh-kMinLowOnThresh);
                low_on_thresh += 0.5*(kMaxLowOnThresh-kMinLowOnThresh))
    for (double high_on_thresh = kMinHighOnThresh + 0.25*(kMaxHighOnThresh-kMinHighOnThresh);
                high_on_thresh <= kMinHighOnThresh + 0.75*(kMaxHighOnThresh-kMinHighOnThresh);
                high_on_thresh += 0.5*(kMaxHighOnThresh-kMinHighOnThresh))
    for (double high_off_thresh = kMinHighOffThresh + 0.25*(kMaxHighOffThresh-kMinHighOffThresh);
                high_off_thresh <= kMinHighOffThresh + 0.75*(kMaxHighOffThresh-kMinHighOffThresh);
                high_off_thresh += 0.5*(kMaxHighOffThresh-kMinHighOffThresh))
    for (double high_spike_frac = kMinHighSpikeFrac + 0.25*(kMaxHighSpikeFrac-kMinHighSpikeFrac);
                high_spike_frac <= kMinHighSpikeFrac + 0.75*(kMaxHighSpikeFrac-kMinHighSpikeFrac);
                high_spike_frac += 0.5*(kMaxHighSpikeFrac-kMinHighSpikeFrac))
    for (double high_spike_level = kMinHighSpikeLevel + 0.25*(kMaxHighSpikeLevel-kMinHighSpikeLevel);
                high_spike_level <= kMinHighSpikeLevel + 0.75*(kMaxHighSpikeLevel-kMinHighSpikeLevel);
                high_spike_level += 0.5*(kMaxHighSpikeLevel-kMinHighSpikeLevel))
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

    return ret;
  }

  void shrinkSteps() { pattern_divisor_ *= 2.0; }

private:
  // A vector of example-sets. Each example-set is a vector of samples of audio,
  // paired with how many events are expected to be in that audio.
  std::vector<std::vector<std::pair<RecordedAudio, int>>> examples_sets_;
  // Each variable's offset will be (kVarMax-kVarMin)/pattern_divisor_
  double pattern_divisor_ = 4.0;
};

RecordedAudio recordExample(int desired_events)
{
  const int kSecondsToRecord = 4;
  if (desired_events == 0)
  {
    printf("About to record! Will record for %d seconds. During that time, please "
           "don't do ANY blowing; this is just to record your typical "
           "background sounds. Press any key when you are ready to start.\n",
           kSecondsToRecord);
  }
  else
  {
    printf("About to record! Will record for %d seconds. During that time, please "
           "blow on the mic %d times. Press any key when you are ready to start.\n",
           kSecondsToRecord, desired_events);
  }
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("Now recording..."); fflush(stdout);
  RecordedAudio recorder(kSecondsToRecord);
  printf("recording done.\n");
  return recorder;
}

// the following is actual code, not just a header:
#include "train_common.h"

} // namespace

void trainBlow()
{
  // the int is number of events that are actually in each example
  std::vector<std::pair<RecordedAudio, int>> audio_examples;
  if (DOING_DEVELOPMENT_TESTING) // for easy development of the code
  {
    audio_examples.emplace_back(RecordedAudio("blows_0.pcm"), 0);
    audio_examples.emplace_back(RecordedAudio("blows_1.pcm"), 1);
    audio_examples.emplace_back(RecordedAudio("blows_2.pcm"), 2);
    audio_examples.emplace_back(RecordedAudio("blows_3.pcm"), 3);
  }
  else // for actual use
  {
    audio_examples.emplace_back(recordExample(0), 0);
    audio_examples.emplace_back(recordExample(1), 1);
    audio_examples.emplace_back(recordExample(2), 2);
    audio_examples.emplace_back(recordExample(3), 3);
  }
  std::vector<std::string> noise_fnames = {"falls_of_fall.pcm", "brandenburg.pcm"};
  TrainParamsFactory factory(audio_examples, noise_fnames);

  patternSearch(factory);
}

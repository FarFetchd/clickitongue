#include "train_hum.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "hum_detector.h"
#include "fft_result_distributor.h"
#include "interaction.h"

namespace {

class TrainParams
{
public:
  TrainParams(double o1on, double o1off, double o6lim, double alpha)
  : o1_on_thresh(o1on), o1_off_thresh(o1off), o6_limit(o6lim),
    ewma_alpha(alpha) {}

  bool operator==(TrainParams const& other) const
  {
    return o1_on_thresh == other.o1_on_thresh &&
           o1_off_thresh == other.o1_off_thresh &&
           o6_limit == other.o6_limit &&
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
    just_one_detector.emplace_back(std::make_unique<HumDetector>(
        nullptr, o1_on_thresh, o1_off_thresh, o6_limit, ewma_alpha, &event_frames));

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
    printf("--o1_on_thresh=%g --o1_off_thresh=%g --o6_limit=%g --ewma_alpha=%g\n",
           o1_on_thresh, o1_off_thresh, o6_limit, ewma_alpha);
  }

  double o1_on_thresh;
  double o1_off_thresh;
  double o6_limit;
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

const double kMinO1On = 500;
const double kMaxO1On = 12000;
const double kMinO1Off = 100;
const double kMaxO1Off = 2000;
const double kMinO6Limit = 20;
const double kMaxO6Limit = 400;
const double kMinAlpha = 0.05;
const double kMaxAlpha = 0.5;

double randomO1On()
{
  static RandomStuff* r = new RandomStuff(kMinO1On, kMaxO1On);
  return r->random();
}
double randomO1Off()
{
  static RandomStuff* r = new RandomStuff(kMinO1Off, kMaxO1Off);
  return r->random();
}
double randomO6Limit()
{
  static RandomStuff* r = new RandomStuff(kMinO6Limit, kMaxO6Limit);
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
      double o1_on_thresh, double o1_off_thresh, double o6_limit, double ewma_alpha,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(o1_on_thresh, o1_off_thresh, o6_limit, ewma_alpha)),
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

  bool emplaceIfValid(std::vector<TrainParamsCocoon>& ret, double o1_on_thresh,
                      double o1_off_thresh, double o6_limit, double ewma_alpha)
  {
    if (o1_off_thresh < o1_on_thresh)
    {
      ret.emplace_back(o1_on_thresh, o1_off_thresh, o6_limit,
                       ewma_alpha, examples_sets_);
      return true;
    }
    return false;
  }

  void emplaceRandomParams(std::vector<TrainParamsCocoon>& ret)
  {
    while (true)
    {
      double o1_on_thresh = randomO1On();
      double o1_off_thresh = randomO1Off();
      double o6_limit = randomO6Limit();
      double ewma_alpha = randomAlpha();
      if (emplaceIfValid(ret, o1_on_thresh, o1_off_thresh, o6_limit, ewma_alpha))
        break;
    }
  }

#define HIDEOUS_FOR(x, mn, mx) for (double x = mn + 0.25*( mx - mn ); x <= mn + 0.75*( mx - mn ); x += 0.5*( mx - mn ))

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    HIDEOUS_FOR(o1_on_thresh, kMinO1On, kMaxO1On)
    HIDEOUS_FOR(o1_off_thresh, kMinO1Off, kMaxO1Off)
    HIDEOUS_FOR(o6_limit, kMinO6Limit, kMaxO6Limit)
    HIDEOUS_FOR(ewma_alpha, kMinAlpha, kMaxAlpha)
    {
      emplaceIfValid(ret, o1_on_thresh, o1_off_thresh, o6_limit, ewma_alpha);
    }

    ret.emplace_back(0.5*(kMaxO1On-kMinO1On),
                     0.5*(kMaxO1Off-kMinO1Off),
                     0.5*(kMaxO6Limit-kMinO6Limit),
                     0.5*(kMaxAlpha-kMinAlpha),
                     examples_sets_);
    for (int i = 0; i < 15; i++)
      emplaceRandomParams(ret);
    return ret;
  }

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_o1_on_thresh = x.o1_on_thresh - (kMaxO1On-kMinO1On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1On, left_o1_on_thresh, kMaxO1On);
    emplaceIfValid(ret, left_o1_on_thresh, x.o1_off_thresh, x.o6_limit, x.ewma_alpha);
    double rite_o1_on_thresh = x.o1_on_thresh + (kMaxO1On-kMinO1On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1On, rite_o1_on_thresh, kMaxO1On);
    emplaceIfValid(ret, rite_o1_on_thresh, x.o1_off_thresh, x.o6_limit, x.ewma_alpha);

    double left_o1_off_thresh = x.o1_off_thresh - (kMaxO1Off-kMinO1Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1Off, left_o1_off_thresh, kMaxO1Off);
    emplaceIfValid(ret, x.o1_on_thresh, left_o1_off_thresh, x.o6_limit, x.ewma_alpha);
    double rite_o1_off_thresh = x.o1_off_thresh + (kMaxO1Off-kMinO1Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1Off, rite_o1_off_thresh, kMaxO1Off);
    emplaceIfValid(ret, x.o1_on_thresh, rite_o1_off_thresh, x.o6_limit, x.ewma_alpha);

    double left_o6_limit = x.o6_limit - (kMaxO6Limit-kMinO6Limit)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Limit, left_o6_limit, kMaxO6Limit);
    emplaceIfValid(ret, x.o1_on_thresh, x.o1_off_thresh, left_o6_limit, x.ewma_alpha);
    double rite_o6_limit = x.o6_limit + (kMaxO6Limit-kMinO6Limit)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Limit, rite_o6_limit, kMaxO6Limit);
    emplaceIfValid(ret, x.o1_on_thresh, x.o1_off_thresh, rite_o6_limit, x.ewma_alpha);

    double left_ewma_alpha = x.ewma_alpha - (kMaxAlpha-kMinAlpha)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinAlpha, left_ewma_alpha, kMaxAlpha);
    emplaceIfValid(ret, x.o1_on_thresh, x.o1_off_thresh, x.o6_limit, left_ewma_alpha);
    double rite_ewma_alpha = x.ewma_alpha + (kMaxAlpha-kMinAlpha)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinAlpha, rite_ewma_alpha, kMaxAlpha);
    emplaceIfValid(ret, x.o1_on_thresh, x.o1_off_thresh, x.o6_limit, rite_ewma_alpha);

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

double pickScalingFactor(AudioRecording const& rec)
{
  FourierLease lease = g_fourier->borrowWorker();

  std::vector<double> o1;
  std::vector<float> const& samples = rec.samples();
  for (int i = 0; i < samples.size(); i += kFourierBlocksize * kNumChannels)
  {
    for (int j=0; j<kFourierBlocksize; j++)
      lease.in[j] = samples[i + j * kNumChannels];
    lease.runFFT();

    o1.push_back(lease.out[1][0]*lease.out[1][0] + lease.out[1][1]*lease.out[1][1]);
  }

  std::sort(o1.begin(), o1.end());

  double max_gap = 0;
  int ind_before_gap = 0;
  for (int j = (int)(0.1*o1.size()); j < (int)(0.95*o1.size()); j+=10)
  {
    if (o1[j+10] - o1[j] > max_gap)
    {
      max_gap = o1[j+10] - o1[j];
      ind_before_gap = j;
    }
  }
  double median_loud = o1[ind_before_gap+10+(o1.size()-(ind_before_gap+10))/2];

  const std::array<double, 22> kScales = {0.001, 0.002, 0.005, 0.01, 0.02, 0.05,
    0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
  const double kCanonicalHumO1 = 14689;
  double best_scale = 1;
  double best_gap = 999999999;
  for (double scale : kScales)
  {
    double gap = fabs(median_loud * scale - kCanonicalHumO1);
    if (gap < best_gap)
    {
      best_gap = gap;
      best_scale = scale;
    }
  }
  return best_scale;
}

// the following is actual code, not just a header:
#include "train_common.h"

} // namespace

AudioRecording recordExampleHum(int desired_events)
{
  return recordExampleCommon(desired_events, "humming", "hum");
}

HumConfig trainHum(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   Action hum_on_action, Action hum_off_action, bool verbose)
{
  int most_examples_ind = 0;
  for (int i = 0; i < audio_examples.size(); i++)
    if (audio_examples[i].second > audio_examples[most_examples_ind].second)
      most_examples_ind = i;
  double scale = pickScalingFactor(audio_examples[most_examples_ind].first);
  printf("hum scale: %g\n", scale);

  std::vector<std::string> noise_fnames =
      {"data/noise1.pcm", "data/noise2.pcm", "data/noise3.pcm"};

  TrainParamsFactory factory(audio_examples, noise_fnames);
  TrainParams best = patternSearch(factory, verbose);

  HumConfig ret;
  ret.action_on = hum_on_action;
  ret.action_off = hum_off_action;
  ret.o1_on_thresh = best.o1_on_thresh;
  ret.o1_off_thresh = best.o1_off_thresh;
  ret.o6_limit = best.o6_limit;
  ret.ewma_alpha = best.ewma_alpha;

  ret.enabled = (best.score[0] == 0 && best.score[1] < 3);
  return ret;
}

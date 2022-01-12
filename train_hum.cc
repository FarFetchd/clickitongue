#include "train_hum.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "hum_detector.h"
#include "fft_result_distributor.h"
#include "interaction.h"

namespace {

constexpr double kEwmaAlpha = 0.3;

constexpr double kMinO1On = 100;
constexpr double kMaxO1On = 10000;
constexpr double kMinO1Off = 10;
constexpr double kMaxO1Off = 1000;
constexpr double kMinO6Limit = 10;
constexpr double kMaxO6Limit = 500;

class TrainParams
{
public:
  TrainParams(double o1on, double o1off, double o6lim, double scl)
  : o1_on_thresh(o1on), o1_off_thresh(o1off), o6_limit(o6lim), scale(scl) {}

  bool operator==(TrainParams const& other) const
  {
    return o1_on_thresh == other.o1_on_thresh &&
           o1_off_thresh == other.o1_off_thresh &&
           o6_limit == other.o6_limit;
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
        nullptr, o1_on_thresh, o1_off_thresh, o6_limit, kEwmaAlpha,
        /*require_delay=*/true, &event_frames));

    FFTResultDistributor wrapper(std::move(just_one_detector), scale,
                                 /*training=*/true);
    for (int sample_ind = 0;
         sample_ind + kFourierBlocksize * g_num_channels < samples.size();
         sample_ind += kFourierBlocksize * g_num_channels)
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
  std::string scoreToString() const
  {
    std::string ret = "{";
    for (int x : score)
      ret += std::to_string(x) + ",";
    return ret + "}";
  }
  std::string paramsToString()
  {
    return std::string(
        "hum_o1_on_thresh: ") + std::to_string(o1_on_thresh) +
        " hum_o1_off_thresh: " + std::to_string(o1_off_thresh) +
        " hum_o6_limit: " + std::to_string(o6_limit);
  }

  double o1_on_thresh;
  double o1_off_thresh;
  double o6_limit;
  double scale;

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
      double o1_on_thresh, double o1_off_thresh, double o6_limit, double scale,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(o1_on_thresh, o1_off_thresh, o6_limit, scale)),
    score_computer_(std::make_unique<std::thread>(runComputeScore, pupa_.get(),
                                                  example_sets)) {}
  TrainParams awaitHatch()
  {
    PRINTF("."); fflush(stdout);
    score_computer_->join();
    return *pupa_;
  }

private:
  std::unique_ptr<TrainParams> pupa_;
  std::unique_ptr<std::thread> score_computer_;
};

class TrainParamsFactory
{
public:
  TrainParamsFactory(std::vector<std::pair<AudioRecording, int>> const& raw_examples,
                     double scale, bool mic_near_mouth);

  bool emplaceIfValid(std::vector<TrainParamsCocoon>& ret, double o1_on_thresh,
                      double o1_off_thresh, double o6_limit)
  {
    if (o1_off_thresh < o1_on_thresh)
    {
      ret.emplace_back(o1_on_thresh, o1_off_thresh, o6_limit,
                       scale_, examples_sets_);
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
      if (emplaceIfValid(ret, o1_on_thresh, o1_off_thresh, o6_limit))
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
    {
      emplaceIfValid(ret, o1_on_thresh, o1_off_thresh, o6_limit);
    }

    ret.emplace_back(0.5*(kMaxO1On-kMinO1On),
                     0.5*(kMaxO1Off-kMinO1Off),
                     0.5*(kMaxO6Limit-kMinO6Limit),
                     scale_, examples_sets_);
    for (int i = 0; i < 20; i++)
      emplaceRandomParams(ret);
    return ret;
  }

#define KEEP_IN_BOUNDS(mn,x,mx) do { x = std::min(x, mx); x = std::max(x, mn); } while(false)

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_o1_on_thresh = x.o1_on_thresh - (kMaxO1On-kMinO1On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1On, left_o1_on_thresh, kMaxO1On);
    emplaceIfValid(ret, left_o1_on_thresh, x.o1_off_thresh, x.o6_limit);
    double rite_o1_on_thresh = x.o1_on_thresh + (kMaxO1On-kMinO1On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1On, rite_o1_on_thresh, kMaxO1On);
    emplaceIfValid(ret, rite_o1_on_thresh, x.o1_off_thresh, x.o6_limit);

    double left_o1_off_thresh = x.o1_off_thresh - (kMaxO1Off-kMinO1Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1Off, left_o1_off_thresh, kMaxO1Off);
    emplaceIfValid(ret, x.o1_on_thresh, left_o1_off_thresh, x.o6_limit);
    double rite_o1_off_thresh = x.o1_off_thresh + (kMaxO1Off-kMinO1Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1Off, rite_o1_off_thresh, kMaxO1Off);
    emplaceIfValid(ret, x.o1_on_thresh, rite_o1_off_thresh, x.o6_limit);

    double left_o6_limit = x.o6_limit - (kMaxO6Limit-kMinO6Limit)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Limit, left_o6_limit, kMaxO6Limit);
    emplaceIfValid(ret, x.o1_on_thresh, x.o1_off_thresh, left_o6_limit);
    double rite_o6_limit = x.o6_limit + (kMaxO6Limit-kMinO6Limit)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Limit, rite_o6_limit, kMaxO6Limit);
    emplaceIfValid(ret, x.o1_on_thresh, x.o1_off_thresh, rite_o6_limit);

    // and some random points within the pattern grid, the idea being that if
    // there are two params that only improve when changed together, pattern
    // search would miss it.
    for (int i=0; i<15; i++)
    {
      double o1on = randomBetween(left_o1_on_thresh, rite_o1_on_thresh);
      double o1off = randomBetween(left_o1_off_thresh, rite_o1_off_thresh);
      double o6lim = randomBetween(left_o6_limit, rite_o6_limit);
      emplaceIfValid(ret, o1on, o1off, o6lim);
    }
    // and also just some random ones for extra exploration
    for (int i = 0; i < 5; i++)
      emplaceRandomParams(ret);

    return ret;
  }

  void shrinkSteps() { pattern_divisor_ *= 2.0; }

  // A vector of example-sets. Each example-set is a vector of samples of audio,
  // paired with how many events are expected to be in that audio.
  std::vector<std::vector<std::pair<AudioRecording, int>>> examples_sets_;
private:
  // The factor that all Fourier power outputs will be multiplied by.
  const double scale_;
  // Each variable's offset will be (kVarMax-kVarMin)/pattern_divisor_
  double pattern_divisor_ = 4.0;
};

// the following is actual code, not just a header:
#include "train_common.h"

TrainParamsFactory::TrainParamsFactory(
    std::vector<std::pair<AudioRecording, int>> const& raw_examples,
    double scale, bool mic_near_mouth)
  : scale_(scale)
{
  TrainParamsFactoryCtorCommon(&examples_sets_, raw_examples, scale, mic_near_mouth);
}

} // namespace

double pickHumScalingFactor(std::vector<std::pair<AudioRecording, int>>
                            const& audio_examples)
{
  int most_examples_ind = 0;
  for (int i = 0; i < audio_examples.size(); i++)
    if (audio_examples[i].second > audio_examples[most_examples_ind].second)
      most_examples_ind = i;
  AudioRecording const& rec = audio_examples[most_examples_ind].first;

  FourierLease lease = g_fourier->borrowWorker();

  std::vector<double> o1;
  std::vector<float> const& samples = rec.samples();
  for (int i = 0;
       i + kFourierBlocksize*g_num_channels < samples.size();
       i += kFourierBlocksize * g_num_channels)
  {
    for (int j=0; j<kFourierBlocksize; j++)
    {
      if (g_num_channels == 2)
        lease.in[j] = (samples[i + j*g_num_channels] + samples[i + j*g_num_channels + 1]) / 2.0;
      else
        lease.in[j] = samples[i + j];
    }
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

AudioRecording recordExampleHum(int desired_events, bool prolonged)
{
  return recordExampleCommon(desired_events, "humming", "hum", prolonged);
}

HumConfig trainHum(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   double scale, bool mic_near_mouth)
{
  TrainParamsFactory factory(audio_examples, scale, mic_near_mouth);
  g_training_log = fopen("clickitongue_training.log", "at");
  TrainParams best = patternSearch(factory, "hum");

  MIDDLETUNE(best, o6_limit, "o6_limit", kMinO6Limit, kMaxO6Limit);

  HumConfig ret;
  ret.scale = scale;
  ret.action_on = Action::NoAction;
  ret.action_off = Action::NoAction;
  ret.o1_on_thresh = best.o1_on_thresh;
  ret.o1_off_thresh = best.o1_off_thresh;
  ret.o6_limit = best.o6_limit;
  ret.ewma_alpha = kEwmaAlpha;

  fclose(g_training_log);
  ret.enabled = (best.score[0] <= 1);
  return ret;
}

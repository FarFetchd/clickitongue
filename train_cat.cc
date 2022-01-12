#include "train_cat.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "cat_detector.h"
#include "fft_result_distributor.h"
#include "interaction.h"

namespace {

constexpr double kMinO7On = 5;
constexpr double kMaxO7On = 1000;
constexpr double kMinO1Limit = 20;
constexpr double kMaxO1Limit = 3000;

class TrainParams
{
public:
  TrainParams(double o7on, double o1lim, bool use_lim, double scl)
  : o7_on_thresh(o7on), o1_limit(o1lim), use_limit(use_lim), scale(scl) {}

  bool operator==(TrainParams const& other) const
  {
    return o7_on_thresh == other.o7_on_thresh &&
           o1_limit == other.o1_limit &&
           use_limit == other.use_limit;
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
    just_one_detector.emplace_back(std::make_unique<CatDetector>(nullptr,
        o7_on_thresh, o1_limit, use_limit, &event_frames));

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

  // For each example-set, detectEvents on each example, and sum up that sets'
  // total violations. The score is a vector of those violations, one count per
  // example-set.
  void computeScore(std::vector<std::vector<std::pair<AudioRecording, int>>>
                    const& example_sets)
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
    PRINTF("cat_o7_on_thresh: %g cat_o1_limit: %g use_limit: %s\n",
           o7_on_thresh, o1_limit, use_limit ? "true" : "false");
  }

  double o7_on_thresh;
  double o1_limit;
  bool use_limit;
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

double randomO7On()
{
  static RandomStuff* r = new RandomStuff(kMinO7On, kMaxO7On);
  return r->random();
}
double randomO1Limit()
{
  static RandomStuff* r = new RandomStuff(kMinO1Limit, kMaxO1Limit);
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
      double o7_on_thresh, double o1_limit, bool use_limit, double scale,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(o7_on_thresh, o1_limit, use_limit, scale)),
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

  bool emplaceIfValid(std::vector<TrainParamsCocoon>& ret,
                      double o7_on_thresh, double o1_limit)
  {
    if (true)
    {
      ret.emplace_back(o7_on_thresh, o1_limit, true, scale_, examples_sets_);
      ret.emplace_back(o7_on_thresh, o1_limit, false, scale_, examples_sets_);
      return true;
    }
  }

  void emplaceRandomParams(std::vector<TrainParamsCocoon>& ret)
  {
    while (true)
    {
      double o7_on_thresh = randomO7On();
      double o1_limit = randomO1Limit();
      if (emplaceIfValid(ret, o7_on_thresh, o1_limit))
        break;
    }
  }

#define HIDEOUS_FOR(x, mn, mx) for (double x = mn + 0.25*( mx - mn ); x <= mn + 0.75*( mx - mn ); x += 0.5*( mx - mn ))

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    HIDEOUS_FOR(o7_on_thresh, kMinO7On, kMaxO7On)
    HIDEOUS_FOR(o1_limit, kMinO1Limit, kMaxO1Limit)
    {
      emplaceIfValid(ret, o7_on_thresh, o1_limit);
    }

    emplaceIfValid(ret, 0.5*(kMaxO7On-kMinO7On),
                        0.5*(kMaxO1Limit-kMinO1Limit));
    for (int i = 0; i < 15; i++)
      emplaceRandomParams(ret);
    return ret;
  }

#define KEEP_IN_BOUNDS(mn,x,mx) do { x = std::min(x, mx); x = std::max(x, mn); } while(false)

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_o7_on_thresh = x.o7_on_thresh - (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, left_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, left_o7_on_thresh, x.o1_limit);
    double rite_o7_on_thresh = x.o7_on_thresh + (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, rite_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, rite_o7_on_thresh, x.o1_limit);

    double left_o1_limit = x.o1_limit - (kMaxO1Limit-kMinO1Limit)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1Limit, left_o1_limit, kMaxO1Limit);
    emplaceIfValid(ret, x.o7_on_thresh, left_o1_limit);
    double rite_o1_limit = x.o1_limit + (kMaxO1Limit-kMinO1Limit)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1Limit, rite_o1_limit, kMaxO1Limit);
    emplaceIfValid(ret, x.o7_on_thresh, rite_o1_limit);

    // and some random points within the pattern grid, the idea being that if
    // there are two params that only improve when changed together, pattern
    // search would miss it.
    for (int i=0; i<10; i++)
    {
      double o7on = randomBetween(left_o7_on_thresh, rite_o7_on_thresh);
      double o1lim = randomBetween(left_o1_limit, rite_o1_limit);
      emplaceIfValid(ret, o7on, o1lim);
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

AudioRecording recordExampleCat(int desired_events)
{
  return recordExampleCommon(desired_events, "cat-attention-getting",
                             "get a cat's attention", /*prolonged=*/false);
}

CatConfig trainCat(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   double scale, bool mic_near_mouth)
{
  TrainParamsFactory factory(audio_examples, scale, mic_near_mouth);
  TrainParams best = patternSearch(factory);

  // prefer not to use limit if same scores
  if (best.use_limit)
  {
    TrainParams no_limit = best;
    no_limit.use_limit = false;
    no_limit.computeScore(factory.examples_sets_);
    if (!(best < no_limit))
      best = no_limit;
  }

  MIDDLETUNE(best, o7_on_thresh, "o7_on_thresh", kMinO7On, kMaxO7On);

  CatConfig ret;
  ret.scale = scale;
  ret.action_on = Action::NoAction;
  ret.action_off = Action::NoAction;
  ret.o7_on_thresh = best.o7_on_thresh;
  ret.o1_limit = best.o1_limit;
  ret.use_limit = best.use_limit;

  ret.enabled = (best.score[0] <= 1);
  return ret;
}

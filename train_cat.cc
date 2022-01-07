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

constexpr double kMinO5On = 0.2;
constexpr double kMaxO5On = 1500;
constexpr double kMinO6On = 2;
constexpr double kMaxO6On = 500;
constexpr double kMinO7On = 5;
constexpr double kMaxO7On = 1000;

class TrainParams
{
public:
  TrainParams(double o5on, double o6on, double o7on, double scl)
  : o5_on_thresh(o5on), o6_on_thresh(o6on), o7_on_thresh(o7on), scale(scl) {}

  bool operator==(TrainParams const& other) const
  {
    return o5_on_thresh == other.o5_on_thresh &&
           o6_on_thresh == other.o6_on_thresh &&
           o7_on_thresh == other.o7_on_thresh;
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
        o5_on_thresh, o6_on_thresh, o7_on_thresh, &event_frames));

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
    PRINTF("cat_o5_on_thresh: %g cat_o6_on_thresh: %g cat_o7_on_thresh: %g\n",
           o5_on_thresh, o6_on_thresh, o7_on_thresh);
  }

  double o5_on_thresh;
  double o6_on_thresh;
  double o7_on_thresh;
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

double randomO5On()
{
  static RandomStuff* r = new RandomStuff(kMinO5On, kMaxO5On);
  return r->random();
}
double randomO6On()
{
  static RandomStuff* r = new RandomStuff(kMinO6On, kMaxO6On);
  return r->random();
}
double randomO7On()
{
  static RandomStuff* r = new RandomStuff(kMinO7On, kMaxO7On);
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
      double o5_on_thresh, double o6_on_thresh, double o7_on_thresh, double scale,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(o5_on_thresh, o6_on_thresh, o7_on_thresh,
                                        scale)),
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
                      double o5_on_thresh, double o6_on_thresh, double o7_on_thresh)
  {
    if (true)
    {
      ret.emplace_back(o5_on_thresh, o6_on_thresh, o7_on_thresh,
                       scale_, examples_sets_);
      return true;
    }
  }

  void emplaceRandomParams(std::vector<TrainParamsCocoon>& ret)
  {
    while (true)
    {
      double o5_on_thresh = randomO5On();
      double o6_on_thresh = randomO6On();
      double o7_on_thresh = randomO7On();
      if (emplaceIfValid(ret, o5_on_thresh, o6_on_thresh, o7_on_thresh))
        break;
    }
  }

#define HIDEOUS_FOR(x, mn, mx) for (double x = mn + 0.25*( mx - mn ); x <= mn + 0.75*( mx - mn ); x += 0.5*( mx - mn ))

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    HIDEOUS_FOR(o5_on_thresh, kMinO5On, kMaxO5On)
    HIDEOUS_FOR(o6_on_thresh, kMinO6On, kMaxO6On)
    HIDEOUS_FOR(o7_on_thresh, kMinO7On, kMaxO7On)
    {
      emplaceIfValid(ret, o5_on_thresh, o6_on_thresh, o7_on_thresh);
    }

    ret.emplace_back(0.5*(kMaxO5On-kMinO5On),
                     0.5*(kMaxO6On-kMinO6On),
                     0.5*(kMaxO7On-kMinO7On),
                     scale_, examples_sets_);
    for (int i = 0; i < 15; i++)
      emplaceRandomParams(ret);
    return ret;
  }

#define KEEP_IN_BOUNDS(mn,x,mx) do { x = std::min(x, mx); x = std::max(x, mn); } while(false)

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_o5_on_thresh = x.o5_on_thresh - (kMaxO5On-kMinO5On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO5On, left_o5_on_thresh, kMaxO5On);
    emplaceIfValid(ret, left_o5_on_thresh, x.o6_on_thresh, x.o7_on_thresh);
    double rite_o5_on_thresh = x.o5_on_thresh + (kMaxO5On-kMinO5On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO5On, rite_o5_on_thresh, kMaxO5On);
    emplaceIfValid(ret, rite_o5_on_thresh, x.o6_on_thresh, x.o7_on_thresh);

    double left_o6_on_thresh = x.o6_on_thresh - (kMaxO6On-kMinO6On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6On, left_o6_on_thresh, kMaxO6On);
    emplaceIfValid(ret, x.o5_on_thresh, left_o6_on_thresh, x.o7_on_thresh);
    double rite_o6_on_thresh = x.o6_on_thresh + (kMaxO6On-kMinO6On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6On, rite_o6_on_thresh, kMaxO6On);
    emplaceIfValid(ret, x.o5_on_thresh, rite_o6_on_thresh, x.o7_on_thresh);

    double left_o7_on_thresh = x.o7_on_thresh - (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, left_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, x.o5_on_thresh, x.o6_on_thresh, left_o7_on_thresh);
    double rite_o7_on_thresh = x.o7_on_thresh + (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, rite_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, x.o5_on_thresh, x.o6_on_thresh, rite_o7_on_thresh);

    // and some random points within the pattern grid, the idea being that if
    // there are two params that only improve when changed together, pattern
    // search would miss it.
    for (int i=0; i<10; i++)
    {
      double o5on = randomBetween(left_o5_on_thresh, rite_o5_on_thresh);
      double o6on = randomBetween(left_o6_on_thresh, rite_o6_on_thresh);
      double o7on = randomBetween(left_o7_on_thresh, rite_o7_on_thresh);
      emplaceIfValid(ret, o5on, o6on, o7on);
    }
    // and also just some random ones for extra exploration
    for (int i = 0; i < 5; i++)
      emplaceRandomParams(ret);

    return ret;
  }

  TrainParams tuneOn5(TrainParams start, double min_on, double max_on)
  {
    double lo_on = min_on;
    double hi_on = max_on;
    TrainParams cur = start;
    int iterations = 0;
    while (hi_on - lo_on > 0.02 * (max_on - min_on))
    {
      if (++iterations > 40)
        break;
      double cur_on = (lo_on + hi_on) / 2.0;
      cur.o5_on_thresh = cur_on;
      cur.computeScore(examples_sets_);
      if (start < cur)
      {
        hi_on = (cur_on - min_on) / 2.0;
        lo_on = min_on;
      }
      else
        lo_on = cur_on;
    }
    // pull back from our tuned result by half to be on the safe side
    cur.o5_on_thresh = lo_on + (lo_on - start.o5_on_thresh) / 2.0;

    cur.computeScore(examples_sets_);
    if (start < cur)
    {
      PRINTF("o5_on tuning unsuccessful; leaving it alone\n");
      return start;
    }
    PRINTF("tuned o5_on from %g up to %g\n", start.o5_on_thresh, cur.o5_on_thresh);
    return cur;
  }

  TrainParams tuneOn6(TrainParams start, double min_on, double max_on)
  {
    double lo_on = min_on;
    double hi_on = max_on;
    TrainParams cur = start;
    int iterations = 0;
    while (hi_on - lo_on > 0.02 * (max_on - min_on))
    {
      if (++iterations > 40)
        break;
      double cur_on = (lo_on + hi_on) / 2.0;
      cur.o6_on_thresh = cur_on;
      cur.computeScore(examples_sets_);
      if (start < cur)
      {
        hi_on = (cur_on - min_on) / 2.0;
        lo_on = min_on;
      }
      else
        lo_on = cur_on;
    }
    // pull back from our tuned result by half to be on the safe side
    cur.o6_on_thresh = lo_on + (lo_on - start.o6_on_thresh) / 2.0;

    cur.computeScore(examples_sets_);
    if (start < cur)
    {
      PRINTF("o6_on tuning unsuccessful; leaving it alone\n");
      return start;
    }
    PRINTF("tuned o6_on from %g up to %g\n", start.o6_on_thresh, cur.o6_on_thresh);
    return cur;
  }

  TrainParams tuneOn7(TrainParams start, double min_on, double max_on)
  {
    double lo_on = min_on;
    double hi_on = max_on;
    TrainParams cur = start;
    int iterations = 0;
    while (hi_on - lo_on > 0.02 * (max_on - min_on))
    {
      if (++iterations > 40)
        break;
      double cur_on = (lo_on + hi_on) / 2.0;
      cur.o7_on_thresh = cur_on;
      cur.computeScore(examples_sets_);
      if (start < cur)
      {
        hi_on = (cur_on - min_on) / 2.0;
        lo_on = min_on;
      }
      else
        lo_on = cur_on;
    }
    // pull back from our tuned result by half to be on the safe side
    cur.o7_on_thresh = lo_on + (lo_on - start.o7_on_thresh) / 2.0;

    cur.computeScore(examples_sets_);
    if (start < cur)
    {
      PRINTF("o7_on tuning unsuccessful; leaving it alone\n");
      return start;
    }
    PRINTF("tuned o7_on from %g up to %g\n", start.o7_on_thresh, cur.o7_on_thresh);
    return cur;
  }

  void shrinkSteps() { pattern_divisor_ *= 2.0; }

private:
  // The factor that all Fourier power outputs will be multiplied by.
  const double scale_;
  // A vector of example-sets. Each example-set is a vector of samples of audio,
  // paired with how many events are expected to be in that audio.
  std::vector<std::vector<std::pair<AudioRecording, int>>> examples_sets_;
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
  return recordExampleCommon(desired_events, "'tchk'-ing",
                             "'tchk' like calling a cat", /*prolonged=*/false);
}

CatConfig trainCat(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   double scale, bool mic_near_mouth)
{
  TrainParamsFactory factory(audio_examples, scale, mic_near_mouth);
  TrainParams best = patternSearch(factory);

  best = factory.tuneOn5(best, best.o5_on_thresh, kMaxO5On);
  best = factory.tuneOn6(best, best.o6_on_thresh, kMaxO6On);
  best = factory.tuneOn7(best, best.o7_on_thresh, kMaxO7On);

  CatConfig ret;
  ret.scale = scale;
  ret.action_on = Action::NoAction;
  ret.action_off = Action::NoAction;
  ret.o5_on_thresh = best.o5_on_thresh;
  ret.o6_on_thresh = best.o6_on_thresh;
  ret.o7_on_thresh = best.o7_on_thresh;

  ret.enabled = (best.score[0] <= 1);
  return ret;
}

#include "train_blow.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <random>
#include <thread>
#include <vector>

#include "audio_recording.h"
#include "blow_detector.h"
#include "fft_result_distributor.h"
#include "interaction.h"

namespace {

constexpr double kMinO1On = 50;
constexpr double kMaxO1On = 10000;
constexpr double kMinO6On = 50;
constexpr double kMaxO6On = 500;
constexpr double kMinO6Off = 10;
constexpr double kMaxO6Off = 200;
constexpr double kMinO7On = 20;
constexpr double kMaxO7On = 200;
constexpr double kMinO7Off = 5;
constexpr double kMaxO7Off = 50;
constexpr int kMinLookbackBlocks = 1;
constexpr int kMaxLookbackBlocks = 10;

class TrainParams
{
public:
  TrainParams(double o1on, double o6on, double o6off,
              double o7on, double o7off, int lb, double scl)
  : o1_on_thresh(o1on), o6_on_thresh(o6on), o6_off_thresh(o6off),
    o7_on_thresh(o7on), o7_off_thresh(o7off), lookback_blocks(lb), scale(scl) {}

  bool operator==(TrainParams const& other) const
  {
    return o1_on_thresh == other.o1_on_thresh &&
           o6_on_thresh == other.o6_on_thresh &&
           o6_off_thresh == other.o6_off_thresh &&
           o7_on_thresh == other.o7_on_thresh &&
           o7_off_thresh == other.o7_off_thresh &&
           lookback_blocks == other.lookback_blocks;
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
        nullptr, o1_on_thresh, o6_on_thresh, o6_off_thresh,
        o7_on_thresh, o7_off_thresh, lookback_blocks, /*require_delay=*/false,
        &event_frames));

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
    PRINTF("blow_o1_on_thresh: %g blow_o6_on_thresh: %g blow_o6_off_thresh: %g "
           "blow_o7_on_thresh: %g blow_o7_off_thresh: %g lookback_blocks: %d\n",
           o1_on_thresh, o6_on_thresh, o6_off_thresh,
           o7_on_thresh, o7_off_thresh, lookback_blocks);
  }

  double o1_on_thresh;
  double o6_on_thresh;
  double o6_off_thresh;
  double o7_on_thresh;
  double o7_off_thresh;
  int lookback_blocks;
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
int randomLookbackBlocks()
{
  static RandomStuff* r = new RandomStuff(kMinLookbackBlocks, kMaxLookbackBlocks);
  return (int)r->random(); // close enough lol
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
      double o1_on_thresh, double o6_on_thresh,
      double o6_off_thresh, double o7_on_thresh, double o7_off_thresh,
      int lookback_blocks, double scale,
      std::vector<std::vector<std::pair<AudioRecording, int>>> const& example_sets)
  : pupa_(std::make_unique<TrainParams>(o1_on_thresh, o6_on_thresh, o6_off_thresh,
                                        o7_on_thresh, o7_off_thresh,
                                        lookback_blocks, scale)),
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

  bool emplaceIfValid(
      std::vector<TrainParamsCocoon>& ret, double o1_on_thresh,
      double o6_on_thresh, double o6_off_thresh, double o7_on_thresh,
      double o7_off_thresh, int lookback_blocks)
  {
    if (o6_off_thresh < o6_on_thresh && o7_off_thresh < o7_on_thresh)
    {
      ret.emplace_back(o1_on_thresh, o6_on_thresh, o6_off_thresh,
                       o7_on_thresh, o7_off_thresh, lookback_blocks,
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
      double o6_on_thresh = randomO6On();
      double o6_off_thresh = randomO6Off();
      double o7_on_thresh = randomO7On();
      double o7_off_thresh = randomO7Off();
      int lookback_blocks = randomLookbackBlocks();
      if (emplaceIfValid(ret, o1_on_thresh, o6_on_thresh, o6_off_thresh,
                         o7_on_thresh, o7_off_thresh, lookback_blocks))
      {
        break;
      }
    }
  }

#define HIDEOUS_FOR(x, mn, mx) for (double x = mn + 0.25*( mx - mn ); x <= mn + 0.75*( mx - mn ); x += 0.5*( mx - mn ))

  std::vector<TrainParamsCocoon> startingSet()
  {
    std::vector<TrainParamsCocoon> ret;
    HIDEOUS_FOR(o1_on_thresh, kMinO1On, kMaxO1On)
    HIDEOUS_FOR(o6_on_thresh, kMinO6On, kMaxO6On)
    HIDEOUS_FOR(o6_off_thresh, kMinO6Off, kMaxO6Off)
    HIDEOUS_FOR(o7_on_thresh, kMinO7On, kMaxO7On)
    HIDEOUS_FOR(o7_off_thresh, kMinO7Off, kMaxO7Off)
    for (int lookback_blocks = 3; lookback_blocks <= 6; lookback_blocks += 3)
    {
      emplaceIfValid(ret, o1_on_thresh, o6_on_thresh, o6_off_thresh,
                     o7_on_thresh, o7_off_thresh, lookback_blocks);
    }

    ret.emplace_back(0.5*(kMaxO1On-kMinO1On),
                     0.5*(kMaxO6On-kMinO6On),
                     0.5*(kMaxO6Off-kMinO6Off),
                     0.5*(kMaxO7On-kMinO7On),
                     0.5*(kMaxO7Off-kMinO7Off),
                     0.5*(kMaxLookbackBlocks-kMinLookbackBlocks),
                     scale_, examples_sets_);
    for (int i = 0; i < 25; i++)
      emplaceRandomParams(ret);
    return ret;
  }

#define KEEP_IN_BOUNDS(mn,x,mx) do { x = std::min(x, mx); x = std::max(x, mn); } while(false)

  std::vector<TrainParamsCocoon> patternAround(TrainParams x)
  {
    std::vector<TrainParamsCocoon> ret;

    double left_o1_on_thresh = x.o1_on_thresh - (kMaxO1On-kMinO1On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1On, left_o1_on_thresh, kMaxO1On);
    emplaceIfValid(ret, left_o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);
    double rite_o1_on_thresh = x.o1_on_thresh + (kMaxO1On-kMinO1On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO1On, rite_o1_on_thresh, kMaxO1On);
    emplaceIfValid(ret, rite_o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);

    double left_o6_on_thresh = x.o6_on_thresh - (kMaxO6On-kMinO6On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6On, left_o6_on_thresh, kMaxO6On);
    emplaceIfValid(ret, x.o1_on_thresh, left_o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);
    double rite_o6_on_thresh = x.o6_on_thresh + (kMaxO6On-kMinO6On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6On, rite_o6_on_thresh, kMaxO6On);
    emplaceIfValid(ret, x.o1_on_thresh, rite_o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);

    double left_o6_off_thresh = x.o6_off_thresh - (kMaxO6Off-kMinO6Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Off, left_o6_off_thresh, kMaxO6Off);
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   left_o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);
    double rite_o6_off_thresh = x.o6_off_thresh + (kMaxO6Off-kMinO6Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO6Off, rite_o6_off_thresh, kMaxO6Off);
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   rite_o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);

    double left_o7_on_thresh = x.o7_on_thresh - (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, left_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, left_o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);
    double rite_o7_on_thresh = x.o7_on_thresh + (kMaxO7On-kMinO7On)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7On, rite_o7_on_thresh, kMaxO7On);
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, rite_o7_on_thresh, x.o7_off_thresh, x.lookback_blocks);

    double left_o7_off_thresh = x.o7_off_thresh - (kMaxO7Off-kMinO7Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7Off, left_o7_off_thresh, kMaxO7Off);
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, left_o7_off_thresh, x.lookback_blocks);
    double rite_o7_off_thresh = x.o7_off_thresh + (kMaxO7Off-kMinO7Off)/pattern_divisor_;
    KEEP_IN_BOUNDS(kMinO7Off, rite_o7_off_thresh, kMaxO7Off);
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, rite_o7_off_thresh, x.lookback_blocks);

    int left_lookback_blocks = x.lookback_blocks - 1;
    if (left_lookback_blocks < kMinLookbackBlocks) left_lookback_blocks = kMinLookbackBlocks;
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, left_lookback_blocks);
    double rite_lookback_blocks = x.lookback_blocks + 1;
    if (rite_lookback_blocks > kMaxLookbackBlocks) rite_lookback_blocks = kMaxLookbackBlocks;
    emplaceIfValid(ret, x.o1_on_thresh, x.o6_on_thresh,
                   x.o6_off_thresh, x.o7_on_thresh, x.o7_off_thresh, rite_lookback_blocks);

    // and some random points within the pattern grid, the idea being that if
    // there are two params that only improve when changed together, pattern
    // search would miss it.
    for (int i=0; i<10; i++)
    {
      double o1on = randomBetween(left_o1_on_thresh, rite_o1_on_thresh);
      double o6on = randomBetween(left_o6_on_thresh, rite_o6_on_thresh);
      double o6off = randomBetween(left_o6_off_thresh, rite_o6_off_thresh);
      double o7on = randomBetween(left_o7_on_thresh, rite_o7_on_thresh);
      double o7off = randomBetween(left_o7_off_thresh, rite_o7_off_thresh);
      emplaceIfValid(ret, o1on, o6on, o6off, o7on, o7off, x.lookback_blocks);
    }
    // and also just some random ones for extra exploration
    for (int i = 0; i < 5; i++)
      emplaceRandomParams(ret);

    return ret;
  }

  TrainParams tuneLookback(TrainParams start)
  {
    int start_lb = start.lookback_blocks - 1;
    int real_start_lb = start.lookback_blocks;
    int cur_lb = start_lb;
    int final_lb = 4;
    TrainParams cur = start;
    while (cur_lb >= kMinLookbackBlocks)
    {
      cur.lookback_blocks = cur_lb;
      cur.computeScore(examples_sets_);
      if (start < cur)
      {
        final_lb = cur_lb + 2;
        if (final_lb > start_lb)
          final_lb = start_lb;
        break;
      }
      else if (cur < start)
      {
        start = cur;
        start_lb = cur_lb;
      }
      if (cur_lb == kMinLookbackBlocks)
      {
        final_lb = cur_lb + 1 < start_lb ? cur_lb + 1 : start_lb;
        break;
      }
      cur_lb--;
    }
    cur.lookback_blocks = final_lb;
    cur.computeScore(examples_sets_);
    PRINTF("tuned lookback_blocks from %d down to %d\n", real_start_lb, final_lb);
    return cur;
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

double pickBlowScalingFactor(std::vector<std::pair<AudioRecording, int>>
                             const& audio_examples)
{
  int most_examples_ind = 0;
  for (int i = 0; i < audio_examples.size(); i++)
    if (audio_examples[i].second > audio_examples[most_examples_ind].second)
      most_examples_ind = i;
  AudioRecording const& rec = audio_examples[most_examples_ind].first;

  std::vector<double> o[8];

  FourierLease lease = g_fourier->borrowWorker();

  std::vector<float> const& samples = rec.samples();
  for (int i = 0; i < samples.size(); i += kFourierBlocksize * g_num_channels)
  {
    for (int j=0; j <kFourierBlocksize; j++)
    {
      if (g_num_channels == 2)
        lease.in[j] = (samples[i + j*g_num_channels] + samples[i + j*g_num_channels + 1]) / 2.0;
      else
        lease.in[j] = samples[i + j];
    }
    lease.runFFT();
    for (int x = 0; x < kNumFourierBins; x++)
      lease.out[x][0]=lease.out[x][0]*lease.out[x][0] + lease.out[x][1]*lease.out[x][1];

    o[1].push_back(lease.out[1][0]);
    double t2 = 0; for (int j=0;j<32;j++) t2+=lease.out[32+j][0];
    o[6].push_back(t2);
    double s4 = 0; for (int j=0;j<64;j++) s4+=lease.out[64+j][0];
    o[7].push_back(s4);
  }
  std::sort(o[1].begin(), o[1].end());
  std::sort(o[6].begin(), o[6].end());
  std::sort(o[7].begin(), o[7].end());

  std::vector<double> medians;

  for (int i = 1; i < 8; i += (i==1?5:1))
  {
    double max_gap = 0;
    int ind_before_gap = 0;
    for (int j = (int)(0.1*o[i].size()); j < (int)(0.95*o[i].size()); j+=10)
    {
      if (o[i][j+10] - o[i][j] > max_gap)
      {
        max_gap = o[i][j+10] - o[i][j];
        ind_before_gap = j;
      }
    }
    medians.push_back(o[i][ind_before_gap+10+(o[i].size()-(ind_before_gap+10))/2]);
  }

  const std::array<double, 22> kScales = {0.001, 0.002, 0.005, 0.01, 0.02, 0.05,
    0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
  const std::array<double, 3> kCanonicalBlowO167 = {4602,673,264};
  double best_scale = 1;
  double best_squerr = 999999999999;
  for (double scale : kScales)
  {
    double squerr = 0;
    assert(medians.size() == kCanonicalBlowO167.size());
    for (int i = 0; i < medians.size(); i++)
    {
      squerr += (medians[i] * scale - kCanonicalBlowO167[i]) *
                (medians[i] * scale - kCanonicalBlowO167[i]);
    }
    if (squerr < best_squerr)
    {
      best_squerr = squerr;
      best_scale = scale;
    }
  }
  return best_scale;
}

AudioRecording recordExampleBlow(int desired_events, bool prolonged)
{
  return recordExampleCommon(desired_events, "blowing", "blow on the mic", prolonged);
}

BlowConfig trainBlow(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                     double scale, bool mic_near_mouth)
{
  TrainParamsFactory factory(audio_examples, scale, mic_near_mouth);
  TrainParams best = patternSearch(factory);

  tune(&best, &best.o6_off_thresh, /*tune_up=*/false,
       kMinO6Off, best.o6_off_thresh, 0.25, "o6_off", factory.examples_sets_);
  tune(&best, &best.o7_off_thresh, /*tune_up=*/false,
       kMinO7Off, best.o7_off_thresh, 0.25, "o7_off", factory.examples_sets_);
  tune(&best, &best.o1_on_thresh, /*tune_up=*/false,
       kMinO1On, best.o1_on_thresh, 0.75, "o1_on", factory.examples_sets_);
  tune(&best, &best.o6_on_thresh, /*tune_up=*/false,
       kMinO6On, best.o6_on_thresh, 0.75, "o6_on", factory.examples_sets_);
  tune(&best, &best.o7_on_thresh, /*tune_up=*/false,
       kMinO7On, best.o7_on_thresh, 0.75, "o7_on", factory.examples_sets_);
  best = factory.tuneLookback(best);

  BlowConfig ret;
  ret.scale = scale;
  ret.action_on = Action::NoAction;
  ret.action_off = Action::NoAction;
  ret.o1_on_thresh = best.o1_on_thresh;
  ret.o6_on_thresh = best.o6_on_thresh;
  ret.o6_off_thresh = best.o6_off_thresh;
  ret.o7_on_thresh = best.o7_on_thresh;
  ret.o7_off_thresh = best.o7_off_thresh;
  ret.lookback_blocks = best.lookback_blocks;

  ret.enabled = (best.score[0] <= 1);
  return ret;
}

#include "iterative_blow_trainer.h"

#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include "audio_recording.h"
#include "getch.h"
#include "blow_detector.h"

namespace {

class TrainParams
{
public:
  TrainParams(double lpp, double hpp, double lont, double lofft, double hont, double hofft, int bs,
              std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
  : lowpass_percent(lpp), highpass_percent(hpp), low_on_thresh(lont),
    low_off_thresh(lofft), high_on_thresh(hont), high_off_thresh(hofft),
    blocksize(bs), score(computeScore(example_sets)) {}

  bool operator==(TrainParams const& other) const
  {
    return lowpass_percent == other.lowpass_percent &&
           highpass_percent == other.highpass_percent &&
           low_on_thresh == other.low_on_thresh &&
           low_off_thresh == other.low_off_thresh &&
           high_on_thresh == other.high_on_thresh &&
           high_off_thresh == other.high_off_thresh &&
           blocksize == other.blocksize;
  }
  bool roughlyEquals(TrainParams const& other) const
  {
    return fabs(lowpass_percent - other.lowpass_percent) < 1.0 &&
           fabs(highpass_percent - other.highpass_percent) < 1.0 &&
           fabs(low_on_thresh - other.low_on_thresh) < 0.5 &&
           fabs(low_off_thresh - other.low_off_thresh) < 0.5 &&
           fabs(high_on_thresh - other.high_on_thresh) < 0.1 &&
           fabs(high_off_thresh - other.high_off_thresh) < 0.1 &&
           blocksize == other.blocksize;
  }
  friend bool operator<(TrainParams const& l, TrainParams const& r)
  {
    int l_sum = std::accumulate(l.score.begin(), l.score.end(), 0);
    int r_sum = std::accumulate(r.score.begin(), r.score.end(), 0);
    if (l_sum < r_sum)
      return true;
    else if (l_sum > r_sum)
      return false;
    for (int i=0; i<l.score.size(); i++)
    {
      if (l.score[i] < r.score[i])
        return true;
    }
    return false;
  }

  int detectEvents(std::vector<Sample> const& samples)
  {
    std::vector<int> event_frames;
    BlowDetector detector(nullptr, lowpass_percent, highpass_percent,
                          low_on_thresh, low_off_thresh, high_on_thresh,
                          high_off_thresh, blocksize, &event_frames);
    for (int sample_ind = 0;
         sample_ind + blocksize * kNumChannels < samples.size();
         sample_ind += blocksize * kNumChannels)
    {
      detector.processAudio(samples.data() + sample_ind, blocksize);
    }
    return event_frames.size();
  }

  // for each example-set, detectEvents on each example, and sum up that sets'
  // total violations. the score is a vector of those violations, one count per example-set.
  std::vector<int> computeScore(std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
  {
    std::vector<int> score;
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
    return score;
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
           "--low_off_thresh=%g --high_on_thresh=%g --high_off_thresh=%g --blocksize=%d\n",
           lowpass_percent, highpass_percent, low_on_thresh,
           low_off_thresh, high_on_thresh, high_off_thresh, blocksize);
  }

  double lowpass_percent;
  double highpass_percent;
  double low_on_thresh;
  double low_off_thresh;
  double high_on_thresh;
  double high_off_thresh;
  int blocksize;
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

class ExamplesSets
{
public:
  void addExampleSet(std::vector<std::pair<RecordedAudio, int>> ex)
  {
    examples_sets_.push_back(ex);
  }
  TrainParams randomParams()
  {
    double lowpass_percent = randomLowPassPercent();
    double highpass_percent = randomHighPassPercent(lowpass_percent);
    double low_off_thresh = randomLowOffThresh();
    double low_on_thresh = randomLowOnThresh(low_off_thresh);
    double high_off_thresh = randomHighOffThresh();
    double high_on_thresh = randomHighOnThresh(high_off_thresh);
    int blocksize = 256;

    return TrainParams(lowpass_percent, highpass_percent, low_on_thresh,
                       low_off_thresh, high_on_thresh, high_off_thresh,
                       blocksize, examples_sets_);
  }
  TrainParams betweenCandidates(TrainParams a, TrainParams b)
  {
    double lowpass_percent = (a.lowpass_percent + b.lowpass_percent) / 2.0;
    double highpass_percent = (a.highpass_percent + b.highpass_percent) / 2.0;
    double low_off_thresh = (a.low_off_thresh + b.low_off_thresh) / 2.0;
    double low_on_thresh = (a.low_on_thresh + b.low_on_thresh) / 2.0;
    double high_off_thresh = (a.high_off_thresh + b.high_off_thresh) / 2.0;
    double high_on_thresh = (a.high_on_thresh + b.high_on_thresh) / 2.0;
    int blocksize = b.blocksize;

    return TrainParams(lowpass_percent, highpass_percent, low_on_thresh,
                       low_off_thresh, high_on_thresh, high_off_thresh,
                       blocksize, examples_sets_);
  }
  TrainParams beyondCandidates(TrainParams from, TrainParams beyond)
  {
    double lowpass_percent_diff = beyond.lowpass_percent - from.lowpass_percent;
    double highpass_percent_diff = beyond.highpass_percent - from.highpass_percent;
    double low_off_thresh_diff = beyond.low_off_thresh - from.low_off_thresh;
    double low_on_thresh_diff = beyond.low_on_thresh - from.low_on_thresh;
    double high_off_thresh_diff = beyond.high_off_thresh - from.high_off_thresh;
    double high_on_thresh_diff = beyond.high_on_thresh - from.high_on_thresh;

    double lowpass_percent = beyond.lowpass_percent + lowpass_percent_diff/2.0;
    double highpass_percent = beyond.highpass_percent + highpass_percent_diff/2.0;
    double low_off_thresh = beyond.low_off_thresh + low_off_thresh_diff/2.0;
    double low_on_thresh = beyond.low_on_thresh + low_on_thresh_diff/2.0;
    double high_off_thresh = beyond.high_off_thresh + high_off_thresh_diff/2.0;
    double high_on_thresh = beyond.high_on_thresh + high_on_thresh_diff/2.0;

    lowpass_percent = std::min(lowpass_percent, kMaxLowPassPercent);
    lowpass_percent = std::max(lowpass_percent, kMinLowPassPercent);
    highpass_percent = std::min(highpass_percent, kMaxHighPassPercent);
    highpass_percent = std::max(highpass_percent, kMinHighPassPercent);
    low_off_thresh = std::min(low_off_thresh, kMaxLowOffThresh);
    low_off_thresh = std::max(low_off_thresh, kMinLowOffThresh);
    low_on_thresh = std::min(low_on_thresh, kMaxLowOnThresh);
    low_on_thresh = std::max(low_on_thresh, kMinLowOnThresh);
    high_off_thresh = std::min(high_off_thresh, kMaxHighOffThresh);
    high_off_thresh = std::max(high_off_thresh, kMinHighOffThresh);
    high_on_thresh = std::min(high_on_thresh, kMaxHighOnThresh);
    high_on_thresh = std::max(high_on_thresh, kMinHighOnThresh);

    return TrainParams(lowpass_percent, highpass_percent, low_on_thresh,
                       low_off_thresh, high_on_thresh, high_off_thresh,
                       beyond.blocksize, examples_sets_);
  }
private:
  // A vector of example-sets. Each example-set is a vector of samples of audio,
  // paired with how many events are expected to be in that audio.
  std::vector<std::vector<std::pair<RecordedAudio, int>>> examples_sets_;
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

constexpr bool DOING_DEVELOPMENT_TESTING = true;
} // namespace

void iterativeBlowTrainMain()
{
  std::vector<RecordedAudio> audio_examples;
  if (DOING_DEVELOPMENT_TESTING) // for easy development of the code
  {
    audio_examples.emplace_back("blows_0.pcm");
    audio_examples.emplace_back("blows_1.pcm");
    audio_examples.emplace_back("blows_2.pcm");
    audio_examples.emplace_back("blows_3.pcm");
  }
  else // for actual use
  {
    audio_examples.push_back(recordExample(0));
    audio_examples.push_back(recordExample(1));
    audio_examples.push_back(recordExample(2));
    audio_examples.push_back(recordExample(3));
  }
  std::vector<std::string> noise_names = {"falls_of_fall.pcm", "brandenburg.pcm"};
  std::vector<RecordedAudio> noises;
  for (auto name : noise_names)
    noises.emplace_back(name);

  ExamplesSets examples_sets;
  // (first, add examples without any noise)
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(audio_examples[0], 0);
    examples.emplace_back(audio_examples[1], 1);
    examples.emplace_back(audio_examples[2], 2);
    examples.emplace_back(audio_examples[3], 3);
    examples_sets.addExampleSet(examples);
  }
  for (auto const& noise : noises) // now a set of our examples for each noise
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(audio_examples[0], 0); examples.back().first.scale(0.7); examples.back().first += noise;
    examples.emplace_back(audio_examples[1], 1); examples.back().first.scale(0.7); examples.back().first += noise;
    examples.emplace_back(audio_examples[2], 2); examples.back().first.scale(0.7); examples.back().first += noise;
    examples.emplace_back(audio_examples[3], 3); examples.back().first.scale(0.7); examples.back().first += noise;
    examples_sets.addExampleSet(examples);
  }
  // finally, a quiet version, for a challenge/tie breaker
  {
    std::vector<std::pair<RecordedAudio, int>> examples;
    examples.emplace_back(audio_examples[0], 0); examples.back().first.scale(0.7);
    examples.emplace_back(audio_examples[1], 1); examples.back().first.scale(0.7);
    examples.emplace_back(audio_examples[2], 2); examples.back().first.scale(0.7);
    examples.emplace_back(audio_examples[3], 3); examples.back().first.scale(0.7);
    examples_sets.addExampleSet(examples);
  }

  printf("beginning optimization computations...\n");
  // The optimization algorithm: first, seed with 10 random points.
  std::set<TrainParams> candidates;
  for (int i=0; i<10; i++)
    candidates.insert(examples_sets.randomParams());
  TrainParams previous_best = *candidates.begin();
  int same_streak = 0;
  while (true)
  {
    // Keep the best three, discard the rest.
    auto it = candidates.begin();
    TrainParams best = *it;
    TrainParams second = *(++it);
    TrainParams third = *(++it);
    candidates.clear();

    printf("current best: ");
    best.printParams();
    printf("scores: best: %s, 2nd: %s, 3rd: %s\n",
           best.toString().c_str(), second.toString().c_str(), third.toString().c_str());
    if (best.roughlyEquals(previous_best))
      same_streak++;
    else
      same_streak = 0;
    previous_best = best;
    if (best.roughlyEquals(second) || same_streak > 4)
      break; // we've probably converged

    // The next iteration will consider, first of all, our current best three.
    candidates.insert(best);
    candidates.insert(second);
    candidates.insert(third);
    // Call our three best '1,2,3'. We will also consider these A,B,C,D:
    // A--1--B--2
    // C--1--D--3
    candidates.insert(examples_sets.betweenCandidates(best, second)); // B
    candidates.insert(examples_sets.betweenCandidates(best, third));  // D
    candidates.insert(examples_sets.beyondCandidates(second, best));  // A
    candidates.insert(examples_sets.beyondCandidates(third, best));   // C
    // Finally, pick a random line passing through best, and add a point on
    // whichever side looks better.
    TrainParams random_point = examples_sets.randomParams();
    candidates.insert(random_point);
    if (random_point < best)
      candidates.insert(examples_sets.betweenCandidates(best, random_point));
    else
      candidates.insert(examples_sets.beyondCandidates(random_point, best));
  }
  printf("converged; done.\n");
}

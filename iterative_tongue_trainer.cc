#include "iterative_tongue_trainer.h"

#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include "audio_recording.h"
#include "getch.h"
#include "tongue_detector.h"

namespace {

class TrainParams
{
public:
  TrainParams(double tl, double th, double teh, double tel, int rb, int bs,
    std::vector<std::vector<std::pair<RecordedAudio, int>>> const& example_sets)
    : tongue_low_hz(tl), tongue_high_hz(th), tongue_hzenergy_high(teh),
      tongue_hzenergy_low(tel), refrac_blocks(rb), blocksize(bs),
      score(computeScore(example_sets)) {}

  bool operator==(TrainParams const& other) const
  {
    return tongue_low_hz == other.tongue_low_hz &&
           tongue_high_hz == other.tongue_high_hz &&
           tongue_hzenergy_high == other.tongue_hzenergy_high &&
           tongue_hzenergy_low == other.tongue_hzenergy_low &&
           refrac_blocks == other.refrac_blocks &&
           blocksize == other.blocksize;
  }
  bool roughlyEquals(TrainParams const& other) const
  {
    return fabs(tongue_low_hz - other.tongue_low_hz) < 2.0 &&
           fabs(tongue_high_hz - other.tongue_high_hz) < 4.0 &&
           fabs(tongue_hzenergy_high - other.tongue_hzenergy_high) < 10.0 &&
           fabs(tongue_hzenergy_low - other.tongue_hzenergy_low) < 1.0 &&
           refrac_blocks == other.refrac_blocks &&
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
    TongueDetector detector(nullptr, tongue_low_hz, tongue_high_hz,
                            tongue_hzenergy_high, tongue_hzenergy_low,
                            refrac_blocks, blocksize, &event_frames);
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
    printf("--tongue_low_hz=%g --tongue_high_hz=%g --tongue_hzenergy_high=%g "
           "--tongue_hzenergy_low=%g --refrac_blocks=%d --blocksize=%d\n",
           tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
           tongue_hzenergy_low, refrac_blocks, blocksize);
  }


  double tongue_low_hz;
  double tongue_high_hz;
  double tongue_hzenergy_high;
  double tongue_hzenergy_low;
  int refrac_blocks;
  int blocksize;
  std::vector<int> score;
};

class RandomStuff
{
public:
  RandomStuff(double low, double high, std::random_device* rd)
    : mt_((*rd)()), dist_(low, high) {}
  double random() { return dist_(mt_); }
private:
  std::mt19937 mt_;
  std::uniform_real_distribution<double> dist_;
};
std::random_device* getRandomDev()
{
  std::random_device* dev = new std::random_device;
  return dev;
}

const double kMinLowHz = 500;
const double kMaxLowHz = 1500;
const double kMinHighHz = 700;
const double kMaxHighHz = 2500;
const double kMinLowEnergy = 100;
const double kMaxLowEnergy = 2100;
const double kMinHighEnergy = 500;
const double kMaxHighEnergy = 8000;
double randomLowHz()
{
  static RandomStuff* r = new RandomStuff(kMinHighHz, kMaxLowHz, getRandomDev());
  return r->random();
}
double randomHighHz(double low)
{
  static RandomStuff* r = new RandomStuff(kMinHighHz, kMaxHighHz, getRandomDev());
  double ret = low;
  while (ret - 100 < low)
    ret = r->random();
  return ret;
}
double randomLowEnergy()
{
  static RandomStuff* r = new RandomStuff(kMinLowEnergy, kMaxLowEnergy, getRandomDev());
  return r->random();
}
double randomHighEnergy(double low)
{
  static RandomStuff* r = new RandomStuff(kMinHighEnergy, kMaxHighEnergy, getRandomDev());
  double ret = low;
  while (ret - 100 < low)
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
    double tongue_low_hz = randomLowHz();
    double tongue_high_hz = randomHighHz(tongue_low_hz);
    double tongue_hzenergy_low = randomLowEnergy();
    double tongue_hzenergy_high = randomHighEnergy(tongue_hzenergy_low);
    int refrac_blocks = 12;
    int blocksize = 256;
    return TrainParams(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                       tongue_hzenergy_low, refrac_blocks, blocksize, examples_sets_);
  }

  TrainParams betweenCandidates(TrainParams a, TrainParams b)
  {
    double tongue_low_hz = (a.tongue_low_hz + b.tongue_low_hz) / 2.0;
    double tongue_high_hz = (a.tongue_high_hz + b.tongue_high_hz) / 2.0;
    double tongue_hzenergy_high = (a.tongue_hzenergy_high + b.tongue_hzenergy_high) / 2.0;
    double tongue_hzenergy_low = (a.tongue_hzenergy_low + b.tongue_hzenergy_low) / 2.0;
    int refrac_blocks = a.refrac_blocks;
    int blocksize = b.blocksize;
    return TrainParams(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                       tongue_hzenergy_low, refrac_blocks, blocksize, examples_sets_);
  }
  TrainParams betweenCandidates(TrainParams a, TrainParams b, TrainParams c)
  {
    double tongue_low_hz = (a.tongue_low_hz + b.tongue_low_hz + c.tongue_low_hz) / 3.0;
    double tongue_high_hz = (a.tongue_high_hz + b.tongue_high_hz + c.tongue_high_hz) / 3.0;
    double tongue_hzenergy_high = (a.tongue_hzenergy_high + b.tongue_hzenergy_high + c.tongue_hzenergy_high) / 3.0;
    double tongue_hzenergy_low = (a.tongue_hzenergy_low + b.tongue_hzenergy_low + c.tongue_hzenergy_low) / 3.0;
    int refrac_blocks = a.refrac_blocks;
    int blocksize = b.blocksize;
    return TrainParams(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                       tongue_hzenergy_low, refrac_blocks, blocksize, examples_sets_);
  }
  TrainParams beyondCandidates(TrainParams from, TrainParams beyond)
  {
    double lowfreq_diff = beyond.tongue_low_hz - from.tongue_low_hz;
    double highfreq_diff = beyond.tongue_high_hz - from.tongue_high_hz;
    double highen_diff = beyond.tongue_hzenergy_high - from.tongue_hzenergy_high;
    double lowen_diff = beyond.tongue_hzenergy_low - from.tongue_hzenergy_low;

    double tongue_low_hz = beyond.tongue_low_hz + lowfreq_diff/2.0;
    double tongue_high_hz = beyond.tongue_high_hz + highfreq_diff/2.0;
    double tongue_hzenergy_high = beyond.tongue_hzenergy_high + highen_diff/2.0;
    double tongue_hzenergy_low = beyond.tongue_hzenergy_low + lowen_diff/2.0;

    tongue_low_hz = std::min(tongue_low_hz, kMaxLowHz);
    tongue_low_hz = std::max(tongue_low_hz, kMinLowHz);
    tongue_high_hz = std::min(tongue_high_hz, kMaxHighHz);
    tongue_high_hz = std::max(tongue_high_hz, kMinHighHz);
    tongue_hzenergy_high = std::min(tongue_hzenergy_high, kMaxHighEnergy);
    tongue_hzenergy_high = std::max(tongue_hzenergy_high, kMinHighEnergy);
    tongue_hzenergy_low = std::min(tongue_hzenergy_low, kMaxLowEnergy);
    tongue_hzenergy_low = std::max(tongue_hzenergy_low, kMinLowEnergy);

    int refrac_blocks = beyond.refrac_blocks;
    int blocksize = beyond.blocksize;

    return TrainParams(tongue_low_hz, tongue_high_hz, tongue_hzenergy_high,
                       tongue_hzenergy_low, refrac_blocks, blocksize, examples_sets_);
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
           "don't do ANY tongue clicks; this is just to record your typical "
           "background sounds. Press any key when you are ready to start.\n",
           kSecondsToRecord);
  }
  else
  {
    printf("About to record! Will record for %d seconds. During that time, please "
           "do %d tongue clicks. Press any key when you are ready to start.\n",
           kSecondsToRecord, desired_events);
  }
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("Now recording..."); fflush(stdout);
  RecordedAudio recorder(kSecondsToRecord);
  printf("recording done.\n");
  return recorder;
}

constexpr bool DOING_DEVELOPMENT_TESTING = false;
} // namespace

void iterativeTongueTrainMain()
{
  std::vector<RecordedAudio> audio_examples;
  if (DOING_DEVELOPMENT_TESTING) // for easy development of the code
  {
    audio_examples.emplace_back("clicks_0a.pcm");
    audio_examples.emplace_back("clicks_1a.pcm");
    audio_examples.emplace_back("clicks_2a.pcm");
    audio_examples.emplace_back("clicks_3a.pcm");
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
    examples.emplace_back(audio_examples[0], 0); examples.back().first.scale(0.3);
    examples.emplace_back(audio_examples[1], 1); examples.back().first.scale(0.3);
    examples.emplace_back(audio_examples[2], 2); examples.back().first.scale(0.3);
    examples.emplace_back(audio_examples[3], 3); examples.back().first.scale(0.3);
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

    // The next iteration will consider:
    // our best three,
    candidates.insert(best);
    candidates.insert(second);
    candidates.insert(third);
    // the point in the middle of the three,
    candidates.insert(examples_sets.betweenCandidates(best, second, third));
    // the points in between each pair,
    candidates.insert(examples_sets.betweenCandidates(best, second));
    candidates.insert(examples_sets.betweenCandidates(best, third));
    candidates.insert(examples_sets.betweenCandidates(third, second));
    // the point "beyond" each pair in each direction (i.e. if you found C to be
    // between B and D, then add A and E that give you A---B---C---D---E).
    candidates.insert(examples_sets.beyondCandidates(best, second));
    candidates.insert(examples_sets.beyondCandidates(second, best));
    candidates.insert(examples_sets.beyondCandidates(best, third));
    candidates.insert(examples_sets.beyondCandidates(third, best));
    candidates.insert(examples_sets.beyondCandidates(third, second));
    candidates.insert(examples_sets.beyondCandidates(second, third));
    // Finally, add points in a random direction from each of the three.
    TrainParams random_point = examples_sets.randomParams();
    candidates.insert(random_point);
    candidates.insert(examples_sets.betweenCandidates(best, random_point));
    candidates.insert(examples_sets.betweenCandidates(second, random_point));
    candidates.insert(examples_sets.betweenCandidates(third, random_point));
  }
  printf("converged; done.\n");
}

// TODO update the other trainer to be like this one (or reuse somehow?)

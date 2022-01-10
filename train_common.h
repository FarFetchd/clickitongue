#ifdef CLICKITONGUE_TRAIN_COMMON_H_
#error "CLICKITONGUE_TRAIN_COMMON_H_ is actual code; must only include once."
#endif
#define CLICKITONGUE_TRAIN_COMMON_H_
// Actual code for direct (careful) inclusion within an anonymous namespace.
// Hacky, but does clean things up a bit, and I'm not sure there's a clean way
// to do it the "right" way with an interface base class.

void TrainParamsFactoryCtorCommon(
    std::vector<std::vector<std::pair<AudioRecording, int>>>* examples_sets,
    std::vector<std::pair<AudioRecording, int>> const& raw_examples,
    double scale, bool mic_near_mouth)
{
  std::vector<AudioRecording> noises;
  noises.emplace_back("data/noise1.pcm");
  noises.back().scale(1.0 / scale);
  noises.emplace_back("data/noise2.pcm");
  noises.back().scale(1.0 / scale);
  noises.emplace_back("data/noise3.pcm");
  noises.back().scale(1.0 / scale);

  // If we're training for mic-near-mouth, the base examples should include a
  // recording of light (but near the mic) mouth breathing.
  std::vector<std::pair<AudioRecording, int>> base_examples = raw_examples;
  if (mic_near_mouth)
  {
    base_examples.emplace_back(AudioRecording("data/breath.pcm"), 0);
    base_examples.back().first.scale(1.0 / scale);
  }

  // First, add the base examples, without any noise.
  examples_sets->push_back(base_examples);

  // Next, for each noise sample, our raw examples plus that noise.
  for (auto const& noise : noises)
  {
    std::vector<std::pair<AudioRecording, int>> examples = base_examples;
    for (auto& x : examples)
      x.first += noise;
    examples_sets->push_back(examples);
  }
  // A loud version of the base examples, to make one type less likely to cause
  // false positives for another.
  {
    std::vector<std::pair<AudioRecording, int>> examples = base_examples;
    for (auto& x : examples)
      x.first.scale(1.25);
    examples_sets->push_back(examples);
  }
  // Finally, a quiet version, for a challenge/tie breaker.
  {
    std::vector<std::pair<AudioRecording, int>> examples = base_examples;
    for (auto& x : examples)
      x.first.scale(0.75);
    examples_sets->push_back(examples);
  }
}

void addEqualReplaceBetter(std::vector<TrainParams>* best, TrainParams cur,
                           int max_length)
{
  if (best->empty())
    best->push_back(cur);
  else if (cur < best->front())
  {
    best->clear();
    best->push_back(cur);
  }
  else if (!(best->front() < cur) && best->size() < max_length)
    best->push_back(cur);
}

// https://en.wikipedia.org/wiki/Pattern_search_(optimization)
TrainParams patternSearch(TrainParamsFactory& factory)
{
  PRINTF("beginning optimization computations..."); fflush(stdout);
  std::vector<TrainParams> candidates;
  for (auto& cocoon : factory.startingSet())
    addEqualReplaceBetter(&candidates, cocoon.awaitHatch(), 8);

  int shrinks = 0;
  std::vector<TrainParams> old_candidates;
  std::vector<TrainParams> historical_bests;
  while (candidates != old_candidates && shrinks < 5)
  {
    old_candidates = candidates;

    // patternAround() kicks off a bunch of parallel computation, and the
    // awaitHatch() calls gather it all up.
    for (auto const& candidate : old_candidates)
      for (auto& cocoon : factory.patternAround(candidate))
        addEqualReplaceBetter(&candidates, cocoon.awaitHatch(), 3);

    for (auto const& old_best : historical_bests)
    {
      if (candidates.front() == old_best)
      {
        factory.shrinkSteps();
        shrinks++;
        break;
      }
    }

    PRINTF("\ncurrent best: ");
    candidates.front().printParams();
    PRINTF("best scores: %s\n", candidates.front().toString().c_str());

    historical_bests.push_back(candidates.front());
  }
  PRINTF("converged; done.\n");
  return candidates.front();
}

AudioRecording recordExampleCommon(int desired_events,
                                   std::string dont_do_any_of_this,
                                   std::string do_this_n_times,
                                   bool prolonged)
{
  if (desired_events == 0)
  {
    char msg[1024];
    sprintf(msg,
"This first recording is just to record your typical background noise.\n\n"
"Please don't do ANY %s.\n\nHit enter to start recording (for 4 seconds).",
            dont_do_any_of_this.c_str());
    promptInfo(msg);
  }
  else if (prolonged)
  {
    char msg[1024];
    sprintf(msg,
"Finally, please %s once, but prolonged to 2 or 3 seconds.\n\n"
"Hit enter to start recording (for 4 seconds).",
            do_this_n_times.c_str());
    promptInfo(msg);
  }
  else
  {
    char msg[1024];
    sprintf(msg,
"Next, please %s %d times.\n\nHit enter to start recording (for 4 seconds).",
            do_this_n_times.c_str(), desired_events);
    promptInfo(msg);
  }

  showRecordingBanner();
  AudioRecording recorder(4/*seconds*/);
  hideRecordingBanner();

  return recorder;
}

// member_of_obj must point to a member of 'obj' - the one to be tuned.
// pullback_fraction: pull back this fraction of the distance from our tuned
//                    result towards the value it started at.
//                    e.g. 0.75 to pull back by 3/4ths.
void tune(
    TrainParams* obj, double* member_of_obj, bool tune_up,
    double min_val, double max_val, double pullback_fraction, std::string var_name,
    std::vector<std::vector<std::pair<AudioRecording, int>>> const& examples_sets)
{
  double lo_val = min_val;
  double hi_val = max_val;
  TrainParams start = *obj;
  double start_val = *member_of_obj;
  double true_orig_start_val = start_val;
  int iterations = 0;
  while (hi_val - lo_val > 0.02 * (max_val - min_val))
  {
    if (++iterations > 40)
      break;
    double cur_val = (lo_val + hi_val) / 2.0;
    *member_of_obj = cur_val;
    obj->computeScore(examples_sets);
    if (tune_up)
    {
      if (start < *obj)
      {
        hi_val = (cur_val - min_val) / 2.0;
        lo_val = min_val;
      }
      else
      {
        if (*obj < start)
        {
          start = *obj;
          start_val = cur_val;
        }
        lo_val = cur_val;
      }
    }
    else // tuning down
    {
      if (start < *obj)
      {
        lo_val = (max_val - cur_val) / 2.0;
        hi_val = max_val;
      }
      else
      {
        if (*obj < start)
        {
          start = *obj;
          start_val = cur_val;
        }
        hi_val = cur_val;
      }
    }
  }
  // pull back from our tuned result by pullback_fraction to be on the safe side
  if (tune_up)
    *member_of_obj = start_val + (lo_val - start_val) * (1.0 - pullback_fraction);
  else
    *member_of_obj = hi_val + (start_val - hi_val) * pullback_fraction;

  obj->computeScore(examples_sets);
  if (start < *obj)
  {
    PRINTF("%s tuning unsuccessful; leaving it alone\n", var_name.c_str());
    *obj = start;
    return;
  }
  PRINTF("tuned %s from %g %s to %g\n", var_name.c_str(),
         true_orig_start_val, tune_up ? "up" : "down", *member_of_obj);
}

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

  // A loud version of the base examples, to make one type less likely to cause
  // false positives for another.
  {
    std::vector<std::pair<AudioRecording, int>> examples = base_examples;
    for (auto& x : examples)
      x.first.scale(1.25);
    examples_sets->push_back(examples);
  }
  // For each noise sample, our raw examples plus that noise.
  for (auto const& noise : noises)
  {
    std::vector<std::pair<AudioRecording, int>> examples = base_examples;
    for (auto& x : examples)
      x.first += noise;
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
  {
    best->push_back(cur);
    return;
  }
  bool inserted = false;
  for (int i=0; i<max_length && i<best->size(); i++)
  {
    if (cur < best->at(i))
    {
      best->insert(best->begin()+i, cur);
      inserted = true;
    }
    else if (cur == best->at(i))
      return; // already in there; shouldn't do anything!
  }
  if (!inserted && best->size() < max_length)
    best->push_back(cur);
  while (best->size() > max_length) // resize would need default ctor
    best->pop_back();
}

FILE* g_training_log;

std::vector<TrainParams> getInitialBest(TrainParamsFactory& factory)
{
  std::vector<TrainParams> candidates;
  std::vector<TrainParamsCocoon> cocoons = factory.startingSet();
  for (auto& cocoon : cocoons)
    addEqualReplaceBetter(&candidates, cocoon.awaitHatch(), 8);
  fprintf(g_training_log, "starting set: kept %d out of %d\n",
          (int)candidates.size(), (int)cocoons.size());
  return candidates;
}

// https://en.wikipedia.org/wiki/Pattern_search_(optimization)
TrainParams patternSearch(TrainParamsFactory& factory, const char* soundtype)
{
  fprintf(g_training_log, "beginning %s optimization computations...\n", soundtype);
  PRINTF("beginning %s optimization computations...", soundtype); fflush(stdout);

  std::vector<TrainParams> candidates = getInitialBest(factory);

  int shrinks = 0;
  std::vector<TrainParams> old_candidates;
  std::vector<TrainParams> historical_bests;
  while (candidates != old_candidates && shrinks < 5)
  {
    old_candidates = candidates;

    // patternAround() kicks off a bunch of parallel computation, and the
    // awaitHatch() calls gather it all up.
    for (auto const& candidate : old_candidates)
    {
      std::vector<TrainParamsCocoon> cocoons = factory.patternAround(candidate);
      fprintf(g_training_log, "considering %d points around candidate %s %s\n",
              (int)cocoons.size(),
              candidate.scoreToString().c_str(),
              candidate.paramsToString().c_str());
      for (auto& cocoon : cocoons)
        addEqualReplaceBetter(&candidates, cocoon.awaitHatch(), 3);
    }

    for (auto const& old_best : historical_bests)
    {
      if (candidates.front() == old_best)
      {
        factory.shrinkSteps();
        shrinks++;
        break;
      }
    }

    fprintf(g_training_log, "cur #1: scores: %s %s\n",
            candidates[0].scoreToString().c_str(),
            candidates[0].paramsToString().c_str());
    if (candidates.size() > 1)
    {
      fprintf(g_training_log, "cur #2: scores: %s %s\n",
            candidates[1].scoreToString().c_str(),
            candidates[1].paramsToString().c_str());
    }
    PRINTF("\ncurrent best scores: %s\n", candidates.front().scoreToString().c_str());

    historical_bests.push_back(candidates.front());
  }
  fprintf(g_training_log, "converged; %s optimization done.\n", soundtype);
  PRINTF("converged; %s optimization done.\n", soundtype);
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
"Please don't do ANY %s.\n\nHit enter to start recording (for 5 seconds).",
            dont_do_any_of_this.c_str());
    promptInfo(msg);
  }
  else if (prolonged)
  {
    char msg[1024];
    sprintf(msg,
"Finally, please %s once, but prolonged to 3 or 4 seconds.\n\n"
"Hit enter to start recording (for 5 seconds).",
            do_this_n_times.c_str());
    promptInfo(msg);
  }
  else
  {
    char msg[1024];
    sprintf(msg,
"Next, please %s %d times.\n\nHit enter to start recording (for 5 seconds).",
            do_this_n_times.c_str(), desired_events);
    promptInfo(msg);
  }

  showRecordingBanner();
  AudioRecording recorder(5/*seconds*/);
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
    if (!var_name.empty())
    {
      fprintf(g_training_log, "%s tuning unsuccessful; leaving it alone\n",
              var_name.c_str());
    }
    *obj = start;
    return;
  }
  if (!var_name.empty())
  {
    fprintf(g_training_log, "tuned %s from %g %s to %g\n", var_name.c_str(),
           true_orig_start_val, tune_up ? "up" : "down", *member_of_obj);
  }
}

// obj should be normal, not pointer
#define MIDDLETUNE(obj, VARNAME, var_string_name, min_val, max_val) do \
{                                                              \
  double start_val = obj.VARNAME;                              \
  TrainParams lower = obj;                                     \
  TrainParams upper = obj;                                     \
  tune(&lower, &lower.VARNAME, false,                          \
       min_val, lower.VARNAME, 0, "", factory.examples_sets_); \
  tune(&upper, &upper.VARNAME, true,                           \
       upper.VARNAME, max_val, 0, "", factory.examples_sets_); \
  obj.VARNAME = (lower.VARNAME + upper.VARNAME) / 2.0;         \
  obj.computeScore(factory.examples_sets_);                    \
                                                               \
  fprintf(g_training_log, "tuned %s from %g to %g\n",          \
          var_string_name, start_val, obj.VARNAME);            \
} while(false);

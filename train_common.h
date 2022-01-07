#ifdef CLICKITONGUE_TRAIN_COMMON_H_
#error "CLICKITONGUE_TRAIN_COMMON_H_ is actual code; must only include once."
#endif
#define CLICKITONGUE_TRAIN_COMMON_H_
// Actual code for direct (careful) inclusion within an anonymous namespace.
// Hacky, but does clean things up a bit, and I'm not sure there's a clean way
// to do it the "right" way with an interface base class.

void TrainParamsFactoryCtorCommon(
    std::vector<std::vector<std::pair<AudioRecording, int>>>* examples_sets,
    std::vector<std::pair<AudioRecording, int>> const& raw_examples, double scale)
{
  std::vector<AudioRecording> noises;
  noises.emplace_back("data/noise1.pcm");
  noises.back().scale(1.0 / scale);
  noises.emplace_back("data/noise2.pcm");
  noises.back().scale(1.0 / scale);
  noises.emplace_back("data/noise3.pcm");
  noises.back().scale(1.0 / scale);

  // (first, add the base examples, without any noise)
  examples_sets->push_back(raw_examples);

  for (auto const& noise : noises) // now a set of our examples for each noise
  {
    std::vector<std::pair<AudioRecording, int>> examples = raw_examples;
    for (auto& x : examples)
      x.first += noise;
    examples_sets->push_back(examples);
  }
  // finally, a quiet version, for a challenge/tie breaker
  {
    std::vector<std::pair<AudioRecording, int>> examples = raw_examples;
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

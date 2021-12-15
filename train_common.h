// Actual code for direct (careful) inclusion within an anonymous namespace,
// so no include guard.
// Hacky, but does clean things up a bit, and I'm not sure there's a clean way
// to do it the "right" way with an interface base class.

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
TrainParams patternSearch(TrainParamsFactory& factory, bool verbose)
{
  printf("beginning optimization computations...\n");
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
    for (auto const& candidate : candidates)
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

    if (verbose)
    {
      printf("current best: ");
      candidates.front().printParams();
      printf("best scores: %s\n", candidates.front().toString().c_str());
    }
    historical_bests.push_back(candidates.front());
  }
  if (verbose)
    printf("converged; done.\n");
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
"This first recording is just to record your environment's typical background\n"
"noise. Please don't do ANY %s.\n\nHit enter to start recording (for 4 seconds).",
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

#ifdef CLICKITONGUE_WINDOWS
  // TODO do something... show a window?
#else
  // TODO will this work on OSX? or need something different?
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("\n"
"##############################################################################\n"
"#                                                                            #\n"
"#              **********          ********  ********  ******                #\n"
"#             ************         **     ** **       **    **               #\n"
"#            **************        **     ** **       **                     #\n"
"#            **************        ********  ******   **                     #\n"
"#            **************        **   **   **       **                     #\n"
"#             ************         **    **  **       **    **               #\n"
"#              **********          **     ** ********  ******                #\n"
"#                                                                            #\n"
"##############################################################################\n"
"\n");
#endif

  AudioRecording recorder(4/*seconds*/);

#ifdef CLICKITONGUE_WINDOWS
  // TODO do something... hide a window?
#else
  promptInfo("");
#endif

  return recorder;
}

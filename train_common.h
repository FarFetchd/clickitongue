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
void patternSearch(TrainParamsFactory& factory)
{
  printf("beginning optimization computations...\n");
  std::vector<TrainParams> candidates;
  for (auto& cocoon : factory.startingSet())
    addEqualReplaceBetter(&candidates, cocoon.awaitHatch(), 8);

  int shrinks = 0;
  while (true)
  {
    std::vector<TrainParams> old_candidates = candidates;

    // patternAround() kicks off a bunch of parallel computation, and the
    // awaitHatch() calls gather it all up.
    for (auto const& candidate : candidates)
      for (auto& cocoon : factory.patternAround(candidate))
        addEqualReplaceBetter(&candidates, cocoon.awaitHatch(), 3);

    if (candidates.front() == old_candidates.front())
    {
      factory.shrinkSteps();
      shrinks++;
    }

    printf("current best: ");
    candidates.front().printParams();
    if (DOING_DEVELOPMENT_TESTING)
      printf("best scores: %s\n", candidates.front().toString().c_str());

    if (candidates == old_candidates)
      break; // we'll no longer be changing anything
    if (shrinks >= 5)
      break; // we're probably close to an optimum
  }
  printf("converged; done.\n");
}

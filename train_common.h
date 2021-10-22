// Actual code for direct (careful) inclusion within an anonymous namespace,
// so no include guard.
// Hacky, but hugely cleans things up, and I'm not sure there's a clean way to
// do it the "right" way with an interface base class.

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
    std::vector<TrainParams> best;
    for (int i=0; i<3 && i < candidates.size(); i++)
      best.push_back(candidates[i]);

    for (auto const& candidate : candidates)
    {
      // begin parallel computations
      std::vector<TrainParamsCocoon> step_cocoons = factory.patternAround(candidate);
      // gather results of parallel computations
      for (auto& cocoon : step_cocoons)
        addEqualReplaceBetter(&candidates, cocoon.awaitHatch(), 3);
    }

    std::vector<TrainParams> new_best;
    for (int i=0; i<3 && i < candidates.size(); i++)
      new_best.push_back(candidates[i]);

    if (new_best.front() == best.front())
    {
      factory.shrinkSteps();
      shrinks++;
    }

    printf("current best: ");
    new_best.front().printParams();
    if (DOING_DEVELOPMENT_TESTING)
      printf("best scores: %s\n", new_best.front().toString().c_str());

    if (new_best == best)
      break; // we'll no longer be changing anything
    if (shrinks >= 5)
      break; // we're probably close to an optimum
  }
  printf("converged; done.\n");
}

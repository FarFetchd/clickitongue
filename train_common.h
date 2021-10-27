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

// hacky getch
//================================================
#include <termios.h>
#include <cstdio>
static struct termios old_termios, current_termios;

void make_getchar_like_getch()
{
  tcgetattr(0, &old_termios); // grab old terminal i/o settings
  current_termios = old_termios; // make new settings same as old settings
  current_termios.c_lflag &= ~ICANON; // disable buffered i/o
  current_termios.c_lflag &= ~ECHO;
  tcsetattr(0, TCSANOW, &current_termios); // use these new terminal i/o settings now
}
// can now do:    char ch = getchar();
// once you're done, restore normality with:
void resetTermios()
{
  tcsetattr(0, TCSANOW, &old_termios);
}

RecordedAudio recordExampleCommon(int desired_events,
                                  std::string dont_do_any_of_this,
                                  std::string do_this_n_times)
{
  const int kSecondsToRecord = 4;
  if (desired_events == 0)
  {
    printf("About to record! Will record for %d seconds. During that time, please "
           "don't do ANY %s; this is just to record your typical "
           "background sounds. Press any key when you are ready to start.\n",
           kSecondsToRecord, dont_do_any_of_this.c_str());
  }
  else
  {
    printf("About to record! Will record for %d seconds. During that time, please "
           "%s %d times. Press any key when you are ready to start.\n",
           kSecondsToRecord, do_this_n_times.c_str(), desired_events);
  }
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("Now recording..."); fflush(stdout);
  RecordedAudio recorder(kSecondsToRecord);
  printf("recording done.\n");
  return recorder;
}

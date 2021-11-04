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
  while (candidates != old_candidates && shrinks < 5)
  {
    old_candidates = candidates;

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

    if (verbose)
    {
      printf("current best: ");
      candidates.front().printParams();
      printf("best scores: %s\n", candidates.front().toString().c_str());
    }
  }
  if (verbose)
    printf("converged; done.\n");
  return candidates.front();
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

AudioRecording recordExampleCommon(int desired_events,
                                  std::string dont_do_any_of_this,
                                  std::string do_this_n_times)
{
  const int kSecondsToRecord = 4;
  if (desired_events == 0)
  {
    char msg[1024];
    sprintf(msg, "About to record! Will record for %d seconds. During that time, "
            "please don't do ANY %s; this is just to record your typical "
            "background sounds.\n",
            kSecondsToRecord, dont_do_any_of_this.c_str());
    promptInfo(msg);
  }
  else
  {
    char msg[1024];
    sprintf(msg, "About to record! Will record for %d seconds. During that time, "
            "please %s %d times.\n",
            kSecondsToRecord, do_this_n_times.c_str(), desired_events);
    promptInfo(msg);
  }
#ifndef CLICKITONGUE_WINDOWS
  printf("Press any key when you are ready to start."); fflush(stdout);
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("Now recording..."); fflush(stdout);
#endif
  AudioRecording recorder(kSecondsToRecord);
  promptInfo("recording done.");
  return recorder;
}

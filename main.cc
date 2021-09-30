#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "constants.h"

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_output.h"
#include "ewma_trainer.h"

enum class ProgramMode { Train, Test, Use };
ProgramMode parseArgs(int argc, char** argv)
{
  if (argc < 3 ||
      (argc == 3 && strcmp(argv[1], "train")) ||
      argc == 4 ||
      argc == 5 ||
      (argc == 6 && strcmp(argv[1], "use") && strcmp(argv[1], "test")) ||
      argc > 6)
  {
    printf("usage: %s {train seconds | use refractory_ms thresh_low thresh_high "
           "ewma_alpha | test refractory_ms thresh_low thresh_high ewma_alpha}\n",
           argv[0]);
    exit(1);
  }
  if (!strcmp(argv[1], "train"))
    return ProgramMode::Train;
  else if (!strcmp(argv[1], "use"))
    return ProgramMode::Use;
  else //if (!strcmp(argv[1], "test"))
    return ProgramMode::Test;
}

void useMain(char** argv, Action action, BlockingQueue<Action>* action_queue)
{
  EwmaDetector ewma_clicker(action_queue,
                            /*alpha=*/atof(argv[3]),
                            /*refractory_ms=*/atoi(argv[2]),
                            /*ewma_thresh_high=*/atof(argv[4]),
                            /*ewma_thresh_low=*/atof(argv[3]),
                            action);
  AudioInput audio_input(ewmaUpdateCallback, &ewma_clicker);
  while (audio_input.active())
    Pa_Sleep(500);
}

// TODO on linux, make use of PaAlsa_EnableRealtimeScheduling

int main(int argc, char** argv)
{
  ProgramMode mode = parseArgs(argc, argv);

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);

  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  if (mode == ProgramMode::Use)
    useMain(argv, Action::ClickLeft, &action_queue);
  if (mode == ProgramMode::Test)
    useMain(argv, Action::SayClick, &action_queue);
  else if (mode == ProgramMode::Train)
    trainMain(atoi(argv[2]), &action_queue);

  action_dispatcher.shutdown();
  action_dispatch.join();
  return 0;
}

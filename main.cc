#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "constants.h"

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_output.h"
#include "ewma_trainer.h"
#include "freq_detector.h"
#include "iterative_ewma_trainer.h"
#include "iterative_freq_trainer.h"

enum class ProgramMode { Train, Test, Use };
ProgramMode parseArgs(int argc, char** argv)
{
  if (argc < 3 ||
      (argc == 3 && strcmp(argv[1], "train")) ||
      argc == 4 ||
      argc == 5 ||
      (argc == 6 && !strstr(argv[1], "use") && !strstr(argv[1], "test")) ||
      argc > 6)
  {
    printf("usage: %s {train seconds | use refractory_ms thresh_low thresh_high "
           "ewma_alpha | test refractory_ms thresh_low thresh_high ewma_alpha}\n",
           argv[0]);
    exit(1);
  }
  if (!strcmp(argv[1], "train"))
    return ProgramMode::Train;
  else if (strstr(argv[1], "use"))
    return ProgramMode::Use;
  else //if (strstr(argv[1], "test"))
    return ProgramMode::Test;
}

void useMain(char** argv, Action action, BlockingQueue<Action>* action_queue,
             bool freq)
{
  std::unique_ptr<Detector> clicker;
  if (freq)
    clicker = std::make_unique<FreqDetector>(action_queue, action);
  else
  {
    clicker = std::make_unique<EwmaDetector>(
        action_queue,
        /*alpha=*/atof(argv[3]),
        /*refractory_ms=*/atoi(argv[2]),
        /*ewma_thresh_high=*/atof(argv[4]),
        /*ewma_thresh_low=*/atof(argv[3]),
        action);
  }
  AudioInput audio_input(ewmaUpdateCallback, clicker.get(), /*frames_per_cb=*/128);
  while (audio_input.active())
    Pa_Sleep(500);
}

// TODO on linux, make use of PaAlsa_EnableRealtimeScheduling

int main(int argc, char** argv)
{
  if (argc > 1 && !strcmp(argv[1], "itertrain"))
  {
    iterativeTrainMain();
    return 0;
  }
  if (argc > 1 && !strcmp(argv[1], "iterfreqtrain"))
  {
    iterativeFreqTrainMain();
    return 0;
  }
  ProgramMode mode = parseArgs(argc, argv);
  bool freq = strstr(argv[1], "freq") != nullptr;

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);

  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  if (mode == ProgramMode::Use)
    useMain(argv, Action::ClickLeft, &action_queue, freq);
  if (mode == ProgramMode::Test)
    useMain(argv, Action::SayClick, &action_queue, freq);
  else if (mode == ProgramMode::Train)
    trainMain(atoi(argv[2]), &action_queue);

  action_dispatcher.shutdown();
  action_dispatch.join();
  return 0;
}

#include <cstdio>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_output.h"
#include "blow_detector.h"
#include "cmdline_options.h"
#include "constants.h"
#include "iterative_blow_trainer.h"

void crash(const char* s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

void useBlowMain(Action action, BlockingQueue<Action>* action_queue,
                 float lowpass_percent, float highpass_percent,
                 double low_on_thresh, double low_off_thresh,
                 double high_on_thresh, double high_off_thresh,
                 int fourier_blocksize_frames)
{
  BlowDetector clicker(action_queue, action, lowpass_percent, highpass_percent,
                       low_on_thresh, low_off_thresh, high_on_thresh, high_off_thresh);
  AudioInput audio_input(blowDetectorCallback, &clicker, fourier_blocksize_frames);
  while (audio_input.active())
    Pa_Sleep(500);
}

// TODO on linux, make use of PaAlsa_EnableRealtimeScheduling

int main(int argc, char** argv)
{
  auto opts = structopt::app("clickitongue").parse<ClickitongueCmdlineOpts>(argc,
                                                                            argv);
  if (!opts.mode.has_value() || (opts.mode.value() != "train" &&
                                 opts.mode.value() != "test" &&
                                 opts.mode.value() != "use"))
  {
    crash("Must specify --mode=train, test, or use.");
  }
  if (!opts.detector.has_value() || (opts.detector.value() != "blow"))
  {
    crash("Must specify --detector=blow.");
  }
  std::string mode = opts.mode.value();
  std::string detector = opts.detector.value();

  bool use_blow_detector = detector == "blow";

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  if (mode == "use" || mode == "test")
  {
    Action action = mode == "use" ? Action::ClickLeft : Action::SayClick;
    if (use_blow_detector)
    {
      if (!opts.lowpass_percent.has_value())
        crash("--detector=blow requires a value for --lowpass_percent.");
      if (!opts.highpass_percent.has_value())
        crash("--detector=blow requires a value for --highpass_percent.");
      if (!opts.low_on_thresh.has_value())
        crash("--detector=blow requires a value for --low_on_thresh.");
      if (!opts.low_off_thresh.has_value())
        crash("--detector=blow requires a value for --low_off_thresh.");
      if (!opts.high_on_thresh.has_value())
        crash("--detector=blow requires a value for --high_on_thresh.");
      if (!opts.high_off_thresh.has_value())
        crash("--detector=blow requires a value for --high_off_thresh.");
      if (!opts.fourier_blocksize_frames.has_value())
        crash("--detector=blow requires a value for --fourier_blocksize_frames.");

      useBlowMain(action, &action_queue, opts.lowpass_percent.value(),
                  opts.highpass_percent.value(), opts.low_on_thresh.value(),
                  opts.low_off_thresh.value(), opts.high_on_thresh.value(),
                  opts.high_off_thresh.value(), opts.fourier_blocksize_frames.value());
    }
  }
  else if (mode == "train")
  {
    if (use_blow_detector)
      iterativeBlowTrainMain();
  }

  action_dispatcher.shutdown();
  action_dispatch.join();
  return 0;
}

#include <cstdio>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_output.h"
#include "blow_detector.h"
#include "cmdline_options.h"
#include "constants.h"
#include "iterative_blow_trainer.h"
#include "tongue_detector.h"

void crash(const char* s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
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
  if (!opts.detector.has_value() || (opts.detector.value() != "blow" &&
                                     opts.detector.value() != "tongue"))
  {
    crash("Must specify --detector=blow or tongue.");
  }
  double fourier_blocksize = opts.fourier_blocksize_frames.value_or(-1);
  if (fourier_blocksize != 128 && fourier_blocksize != 256 &&
      fourier_blocksize != 512 && fourier_blocksize != 1024)
  {
    crash("Must specify --fourier_blocksize_frames=128 or 256 or 512 or 1024.");
  }

  std::string mode = opts.mode.value();
  std::string detector = opts.detector.value();

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  if (mode == "use" || mode == "test")
  {
    Action action = mode == "use" ? Action::ClickLeft : Action::SayClick;
    if (detector == "blow")
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

      BlowDetector clicker(&action_queue, action, opts.lowpass_percent.value(),
                           opts.highpass_percent.value(), opts.low_on_thresh.value(),
                           opts.low_off_thresh.value(), opts.high_on_thresh.value(),
                           opts.high_off_thresh.value());
      AudioInput audio_input(blowDetectorCallback, &clicker,
                             opts.fourier_blocksize_frames.value());
      while (audio_input.active())
        Pa_Sleep(500);
    }
    else if (detector == "tongue")
    {
      if (!opts.tongue_low_hz.has_value())
        crash("--detector=tongue requires a value for --tongue_low_hz.");
      if (!opts.tongue_high_hz.has_value())
        crash("--detector=tongue requires a value for --tongue_high_hz.");
      if (!opts.tongue_hzenergy_high.has_value())
        crash("--detector=tongue requires a value for --tongue_hzenergy_high.");
      if (!opts.tongue_hzenergy_low.has_value())
        crash("--detector=tongue requires a value for --tongue_hzenergy_low.");
      if (!opts.refrac_blocks.has_value())
        crash("--detector=tongue requires a value for --refrac_blocks.");

      TongueDetector clicker(
          &action_queue, action,
          opts.tongue_low_hz.value(),  opts.tongue_high_hz.value(),
          opts.tongue_hzenergy_high.value(), opts.tongue_hzenergy_low.value(),
          opts.refrac_blocks.value());
      AudioInput audio_input(tongueDetectorCallback, &clicker,
                             opts.fourier_blocksize_frames.value());
      while (audio_input.active())
        Pa_Sleep(500);
    }
  }
  else if (mode == "train")
  {
    if (detector == "blow")
      iterativeBlowTrainMain();
  }

  action_dispatcher.shutdown();
  action_dispatch.join();
  return 0;
}

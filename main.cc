#include <cstdio>
#include <thread>

#include "action_dispatcher.h"
#include "audio_input.h"
#include "audio_output.h"
#include "cmdline_options.h"
#include "constants.h"
#include "ewma_trainer.h"
#include "freq_detector.h"
#include "iterative_ewma_trainer.h"
#include "iterative_freq_trainer.h"

void crash(const char* s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}

void useEwmaMain(Action action, BlockingQueue<Action>* action_queue,
                 int refractory_ms, float ewma_thresh_low, float ewma_thresh_high,
                 float alpha)
{
  EwmaDetector clicker(action_queue, refractory_ms,
                       ewma_thresh_low, ewma_thresh_high, alpha, action);
  AudioInput audio_input(ewmaUpdateCallback, &clicker, 256 /*TODO TriM*/);
  while (audio_input.active())
    Pa_Sleep(500);
}

void useFreqMain(Action action, BlockingQueue<Action>* action_queue,
                 float lowpass_percent, float highpass_percent,
                 double low_on_thresh, double low_off_thresh,
                 double high_on_thresh, double high_off_thresh,
                 int fourier_blocksize_frames)
{
  FreqDetector clicker(action_queue, action, lowpass_percent, highpass_percent,
                       low_on_thresh, low_off_thresh, high_on_thresh, high_off_thresh);
  AudioInput audio_input(freqDetectorCallback, &clicker, fourier_blocksize_frames);
  while (audio_input.active())
    Pa_Sleep(500);
}

// TODO on linux, make use of PaAlsa_EnableRealtimeScheduling

int main(int argc, char** argv)
{
  auto opts = structopt::app("clickitongue").parse<ClickitongueCmdlineOpts>(argc,
                                                                            argv);
  if (!opts.mode.has_value() || (opts.mode.value() != "train" &&
                                 opts.mode.value() != "itertrain" &&
                                 opts.mode.value() != "test" &&
                                 opts.mode.value() != "use"))
  {
    crash("Must specify --mode=train, itertrain, test, or use.");
  }
  if (!opts.detector.has_value() || (opts.detector.value() != "ewma" &&
                                     opts.detector.value() != "freq"))
  {
    crash("Must specify --detector=ewma or freq.");
  }
  std::string mode = opts.mode.value();
  std::string detector = opts.detector.value();

  bool use_freq_detector = detector == "freq";

  BlockingQueue<Action> action_queue;
  ActionDispatcher action_dispatcher(&action_queue);
  std::thread action_dispatch(actionDispatch, &action_dispatcher);

  if (mode == "use" || mode == "test")
  {
    Action action = mode == "use" ? Action::ClickLeft : Action::SayClick;
    if (use_freq_detector)
    {
      if (!opts.lowpass_percent.has_value())
        crash("--detector=freq requires a value for --lowpass_percent.");
      if (!opts.highpass_percent.has_value())
        crash("--detector=freq requires a value for --highpass_percent.");
      if (!opts.low_on_thresh.has_value())
        crash("--detector=freq requires a value for --low_on_thresh.");
      if (!opts.low_off_thresh.has_value())
        crash("--detector=freq requires a value for --low_off_thresh.");
      if (!opts.high_on_thresh.has_value())
        crash("--detector=freq requires a value for --high_on_thresh.");
      if (!opts.high_off_thresh.has_value())
        crash("--detector=freq requires a value for --high_off_thresh.");
      if (!opts.fourier_blocksize_frames.has_value())
        crash("--detector=freq requires a value for --fourier_blocksize_frames.");

      useFreqMain(action, &action_queue, opts.lowpass_percent.value(),
                  opts.highpass_percent.value(), opts.low_on_thresh.value(),
                  opts.low_off_thresh.value(), opts.high_on_thresh.value(),
                  opts.high_off_thresh.value(), opts.fourier_blocksize_frames.value());
    }
    else
    {
      if (!opts.refractory_ms.has_value())
        crash("--detector=ewma requires a value for --refractory_ms.");
      if (!opts.ewma_thresh_low.has_value())
        crash("--detector=ewma requires a value for --ewma_thresh_low.");
      if (!opts.ewma_thresh_high.has_value())
        crash("--detector=ewma requires a value for --ewma_thresh_high.");
      if (!opts.ewma_alpha.has_value())
        crash("--detector=ewma requires a value for --ewma_alpha.");

      useEwmaMain(action, &action_queue, opts.refractory_ms.value(),
                  opts.ewma_thresh_low.value(), opts.ewma_thresh_high.value(),
                  opts.ewma_alpha.value());
    }
  }
  else if (mode == "train")
    trainMain(opts.duration_seconds.value(), &action_queue);
  else if (mode == "itertrain")
  {
    if (use_freq_detector)
      iterativeFreqTrainMain();
    else
      iterativeEwmaTrainMain();
  }

  action_dispatcher.shutdown();
  action_dispatch.join();
  return 0;
}

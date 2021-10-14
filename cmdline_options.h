#ifndef CLICKITONGUE_CMDLINE_OPTIONS_H_
#define CLICKITONGUE_CMDLINE_OPTIONS_H_

#include "structopt/app.hpp"

struct ClickitongueCmdlineOpts
{
  std::optional<std::string> mode;
  std::optional<std::string> detector;
  std::optional<int> fourier_blocksize_frames = 128;
  std::optional<int> duration_seconds = 5;

  // blow
  std::optional<double> lowpass_percent;
  std::optional<double> highpass_percent;
  std::optional<double> low_on_thresh;
  std::optional<double> low_off_thresh;
  std::optional<double> high_on_thresh;
  std::optional<double> high_off_thresh;

  // tongue
  std::optional<double> tongue_low_hz;
  std::optional<double> tongue_high_hz;
  std::optional<double> tongue_hzenergy_high;
  std::optional<double> tongue_hzenergy_low;
  std::optional<int> refrac_blocks = 1;

  // record, play
  std::optional<std::string> filename;
};
STRUCTOPT(ClickitongueCmdlineOpts, mode, detector, fourier_blocksize_frames, duration_seconds, lowpass_percent, highpass_percent, low_on_thresh, low_off_thresh, high_on_thresh, high_off_thresh, tongue_low_hz, tongue_high_hz, tongue_hzenergy_high, tongue_hzenergy_low, refrac_blocks, filename);

#endif // CLICKITONGUE_CMDLINE_OPTIONS_H_

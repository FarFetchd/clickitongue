#ifndef CLICKITONGUE_CMDLINE_OPTIONS_H_
#define CLICKITONGUE_CMDLINE_OPTIONS_H_

#include "structopt/app.hpp"

struct ClickitongueCmdlineOpts
{
  std::optional<std::string> mode;
  std::optional<std::string> detector;
  std::optional<int> fourier_blocksize_frames = 128;
  std::optional<int> duration_seconds = 5;

  // ewma
  std::optional<int> refractory_ms;
  std::optional<double> ewma_thresh_low;
  std::optional<double> ewma_thresh_high;
  std::optional<double> ewma_alpha;

  // freq
  std::optional<double> lowpass_percent;
  std::optional<double> highpass_percent;
  std::optional<double> low_on_thresh;
  std::optional<double> low_off_thresh;
  std::optional<double> high_on_thresh;
  std::optional<double> high_off_thresh;
};
STRUCTOPT(ClickitongueCmdlineOpts, mode, detector, fourier_blocksize_frames, duration_seconds, refractory_ms, ewma_thresh_low, ewma_thresh_high, ewma_alpha, lowpass_percent, highpass_percent, low_on_thresh, low_off_thresh, high_on_thresh, high_off_thresh);

#endif // CLICKITONGUE_CMDLINE_OPTIONS_H_

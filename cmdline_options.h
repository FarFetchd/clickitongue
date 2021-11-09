#ifndef CLICKITONGUE_CMDLINE_OPTIONS_H_
#define CLICKITONGUE_CMDLINE_OPTIONS_H_

#include "structopt.hpp"

struct ClickitongueCmdlineOpts
{
  std::optional<std::string> mode;
  std::optional<std::string> detector;
  std::optional<int> duration_seconds = 5;

  // blow
  std::optional<double> lowpass_percent;
  std::optional<double> highpass_percent;
  std::optional<double> low_on_thresh;
  std::optional<double> low_off_thresh;
  std::optional<double> high_on_thresh;
  std::optional<double> high_off_thresh;
  std::optional<double> high_spike_frac;
  std::optional<double> high_spike_level;

  // tongue
  std::optional<double> tongue_low_hz;
  std::optional<double> tongue_high_hz;
  std::optional<double> tongue_hzenergy_high;
  std::optional<double> tongue_hzenergy_low;
  std::optional<double> tongue_min_spikes_freq_frac;
  std::optional<double> tongue_high_spike_frac;
  std::optional<double> tongue_high_spike_level;

  // record, play
  std::optional<std::string> filename;
};
STRUCTOPT(ClickitongueCmdlineOpts, mode, detector, duration_seconds,
          lowpass_percent, highpass_percent, low_on_thresh, low_off_thresh,
          high_on_thresh, high_off_thresh, high_spike_frac, high_spike_level,
          tongue_low_hz, tongue_high_hz, tongue_hzenergy_high, tongue_hzenergy_low,
          tongue_min_spikes_freq_frac, tongue_high_spike_frac,
          tongue_high_spike_level, filename);

#endif // CLICKITONGUE_CMDLINE_OPTIONS_H_

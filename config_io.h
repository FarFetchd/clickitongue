#ifndef CLICKITONGUE_CONFIG_IO_H_
#define CLICKITONGUE_CONFIG_IO_H_

#include "flatconfig.hpp"

#include "constants.h"

struct BlowConfig
{
  BlowConfig() : enabled(false) {}
  BlowConfig(farfetchd::ConfigReader const& cfg);

  bool enabled;
  Action action_on;
  Action action_off;
  double lowpass_percent;
  double highpass_percent;
  double low_on_thresh;
  double low_off_thresh;
  double high_on_thresh;
  double high_off_thresh;
  double high_spike_frac;
  double high_spike_level;
};

struct TongueConfig
{
  TongueConfig() : enabled(false) {}
  TongueConfig(farfetchd::ConfigReader const& cfg);

  bool enabled;
  Action action;
  double tongue_low_hz;
  double tongue_high_hz;
  double tongue_hzenergy_high;
  double tongue_hzenergy_low;
  int refrac_blocks;
};

struct Config
{
  Config() {}
  Config(farfetchd::ConfigReader const& cfg) : blow(cfg), tongue(cfg) {}
  BlowConfig blow;
  TongueConfig tongue;
};

std::optional<Config> readConfig();
bool writeConfig(Config config);

#endif // CLICKITONGUE_CONFIG_IO_H_
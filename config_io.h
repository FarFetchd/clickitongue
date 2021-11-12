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

struct PinkConfig
{
  PinkConfig() : enabled(false) {}
  PinkConfig(farfetchd::ConfigReader const& cfg);

  bool enabled;
  Action action_on;
  Action action_off;
  double o5_on_thresh;
  double o5_off_thresh;
  double o6_on_thresh;
  double o6_off_thresh;
  double o7_on_thresh;
  double o7_off_thresh;
  double ewma_alpha;
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
  double tongue_min_spikes_freq_frac;
  double tongue_high_spike_frac;
  double tongue_high_spike_level;
};

struct Config
{
  Config() {}
  Config(farfetchd::ConfigReader const& cfg) : blow(cfg), tongue(cfg) {}
  std::string toString() const;

  BlowConfig blow;
  PinkConfig pink;
  TongueConfig tongue;
};

std::optional<Config> readConfig(std::string config_name);
bool writeConfig(Config config, std::string config_name);

std::string getAndEnsureConfigDir();

#endif // CLICKITONGUE_CONFIG_IO_H_

#ifndef CLICKITONGUE_CONFIG_IO_H_
#define CLICKITONGUE_CONFIG_IO_H_

#include <optional>
#include <string>
#include <vector>

#include "flatconfig.hpp"

#include "constants.h"

struct BlowConfig
{
  BlowConfig() : enabled(false) {}
  BlowConfig(farfetchd::ConfigReader const& cfg);

  bool enabled;
  Action action_on;
  Action action_off;
  double o1_on_thresh;
  double o7_on_thresh;
  double o7_off_thresh;
  int lookback_blocks;
  double scale;
};

struct CatConfig
{
  CatConfig() : enabled(false) {}
  CatConfig(farfetchd::ConfigReader const& cfg);

  bool enabled;
  Action action_on;
  Action action_off;
  double o7_on_thresh;
  double o1_limit;
  bool use_limit;
  double scale;
};

struct HumConfig
{
  HumConfig() : enabled(false) {}
  HumConfig(farfetchd::ConfigReader const& cfg);

  bool enabled;
  Action action_on;
  Action action_off;
  double o1_on_thresh;
  double o1_off_thresh;
  double o6_limit;
  double ewma_alpha;
  double scale;
};

struct Config
{
  Config() {}
  Config(farfetchd::ConfigReader const& cfg) : blow(cfg), cat(cfg), hum(cfg) {}
  std::string toString() const;

  BlowConfig blow;
  CatConfig cat;
  HumConfig hum;
};

// Returns nullopt if it fails to read file.
std::optional<Config> readConfig(std::string config_name);
// Returns true if all is well, false if it fails to write the file or if it
// fails in its attempt to immediately read+parse what was just written.
// attempted_filepath will be filled with the destination filepath.
bool writeConfig(Config config, std::string config_name,
                 std::string* attempted_filepath);

std::string getAndEnsureConfigDir();

std::optional<int> loadDeviceConfig(std::vector<std::string> dev_names);
int writeDeviceConfig(std::vector<std::string> dev_names, int chosen);

#endif // CLICKITONGUE_CONFIG_IO_H_

#include "config_io.h"

#include <sys/stat.h>

#include <cstring>
#include <sstream>

#include "interaction.h"

Action parseAction(std::string str)
{
  if (str == "ClickLeft") return Action::ClickLeft;
  if (str == "ClickRight") return Action::ClickRight;
  if (str == "LeftDown") return Action::LeftDown;
  if (str == "LeftUp") return Action::LeftUp;
  if (str == "RightDown") return Action::RightDown;
  if (str == "RightUp") return Action::RightUp;
  if (str == "ScrollDown") return Action::ScrollDown;
  if (str == "ScrollUp") return Action::ScrollUp;
  if (str == "RecordCurFrame") return Action::RecordCurFrame;
  return Action::NoAction;
}

std::string actionString(Action action)
{
  switch (action)
  {
    case Action::ClickLeft: return "ClickLeft";
    case Action::ClickRight: return "ClickRight";
    case Action::LeftDown: return "LeftDown";
    case Action::LeftUp: return "LeftUp";
    case Action::RightDown: return "RightDown";
    case Action::RightUp: return "RightUp";
    case Action::ScrollDown: return "ScrollDown";
    case Action::ScrollUp: return "ScrollUp";
    case Action::RecordCurFrame: return "RecordCurFrame";
    case Action::NoAction: return "NoAction";
    default: return "!!!!!actionString() was given an unknown action!!!!!";
  }
}

std::string Config::toString() const
{
  std::stringstream sts;
  if (blow.enabled)
  {
    sts << "blow_action_on: " << actionString(blow.action_on) << "\n"
        << "blow_action_off: " << actionString(blow.action_off) << "\n"
        << "blow_lowpass_percent: " << blow.lowpass_percent << "\n"
        << "blow_highpass_percent: " << blow.highpass_percent << "\n"
        << "blow_low_on_thresh: " << blow.low_on_thresh << "\n"
        << "blow_low_off_thresh: " << blow.low_off_thresh << "\n"
        << "blow_high_on_thresh: " << blow.high_on_thresh << "\n"
        << "blow_high_off_thresh: " << blow.high_off_thresh << "\n"
        << "blow_high_spike_frac: " << blow.high_spike_frac << "\n"
        << "blow_high_spike_level: " << blow.high_spike_level << "\n";
  }
  if (pink.enabled)
  {
    sts << "pink_action_on: " << actionString(pink.action_on) << "\n"
        << "pink_action_off: " << actionString(pink.action_off) << "\n"
        << "pink_o5_on_thresh: " << pink.o5_on_thresh << "\n"
        << "pink_o5_off_thresh: " << pink.o5_off_thresh << "\n"
        << "pink_o6_on_thresh: " << pink.o6_on_thresh << "\n"
        << "pink_o6_off_thresh: " << pink.o6_off_thresh << "\n"
        << "pink_o7_on_thresh: " << pink.o7_on_thresh << "\n"
        << "pink_o7_off_thresh: " << pink.o7_off_thresh << "\n"
        << "pink_ewma_alpha: " << pink.ewma_alpha << "\n";
  }
  if (tongue.enabled)
  {
    sts << "tongue_action: " << actionString(tongue.action) << "\n"
        << "tongue_low_hz: " << tongue.tongue_low_hz << "\n"
        << "tongue_high_hz: " << tongue.tongue_high_hz << "\n"
        << "tongue_hzenergy_high: " << tongue.tongue_hzenergy_high << "\n"
        << "tongue_hzenergy_low: " << tongue.tongue_hzenergy_low << "\n"

        << "tongue_min_spikes_freq_frac: " << tongue.tongue_min_spikes_freq_frac << "\n"
        << "tongue_high_spike_frac: " << tongue.tongue_high_spike_frac << "\n"
        << "tongue_high_spike_level: " << tongue.tongue_high_spike_level << "\n";
  }
  return sts.str();
}

BlowConfig::BlowConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("blow_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("blow_action_off").value_or("x"));
  lowpass_percent = cfg.getDouble("blow_lowpass_percent").value_or(-1);
  highpass_percent = cfg.getDouble("blow_highpass_percent").value_or(-1);
  low_on_thresh = cfg.getDouble("blow_low_on_thresh").value_or(-1);
  low_off_thresh = cfg.getDouble("blow_low_off_thresh").value_or(-1);
  high_on_thresh = cfg.getDouble("blow_high_on_thresh").value_or(-1);
  high_off_thresh = cfg.getDouble("blow_high_off_thresh").value_or(-1);
  high_spike_frac = cfg.getDouble("blow_high_spike_frac").value_or(-1);
  high_spike_level = cfg.getDouble("blow_high_spike_level").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
              lowpass_percent >= 0 && highpass_percent >= 0 &&
              low_on_thresh >= 0 && low_off_thresh >= 0 &&
              high_on_thresh >= 0 && high_off_thresh >= 0 &&
              high_spike_frac >= 0 && high_spike_level >= 0);
}

PinkConfig::PinkConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("pink_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("pink_action_off").value_or("x"));
  o5_on_thresh = cfg.getDouble("pink_o5_on_thresh").value_or(-1);
  o5_off_thresh = cfg.getDouble("pink_o5_off_thresh").value_or(-1);
  o6_on_thresh = cfg.getDouble("pink_o6_on_thresh").value_or(-1);
  o6_off_thresh = cfg.getDouble("pink_o6_off_thresh").value_or(-1);
  o7_on_thresh = cfg.getDouble("pink_o7_on_thresh").value_or(-1);
  o7_off_thresh = cfg.getDouble("pink_o7_off_thresh").value_or(-1);
  ewma_alpha = cfg.getDouble("pink_ewma_alpha").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o5_on_thresh >= 0 && o5_off_thresh >= 0 && o6_on_thresh >= 0 &&
             o6_off_thresh >= 0 && o7_on_thresh >= 0 && o7_off_thresh >= 0 &&
             ewma_alpha >= 0);
}

TongueConfig::TongueConfig(farfetchd::ConfigReader const& cfg)
{
  action = parseAction(cfg.getString("tongue_action").value_or("x"));
  tongue_low_hz = cfg.getDouble("tongue_low_hz").value_or(-1);
  tongue_high_hz = cfg.getDouble("tongue_high_hz").value_or(-1);
  tongue_hzenergy_high = cfg.getDouble("tongue_hzenergy_high").value_or(-1);
  tongue_hzenergy_low = cfg.getDouble("tongue_hzenergy_low").value_or(-1);

  tongue_min_spikes_freq_frac = cfg.getDouble("tongue_min_spikes_freq_frac")
                                   .value_or(-1);
  tongue_high_spike_frac = cfg.getDouble("tongue_high_spike_frac").value_or(-1);
  tongue_high_spike_level = cfg.getDouble("tongue_high_spike_level").value_or(-1);

  enabled = (action != Action::NoAction && tongue_low_hz >= 0 &&
             tongue_high_hz >= 0 && tongue_hzenergy_high >= 0 &&
             tongue_hzenergy_low >= 0 && tongue_min_spikes_freq_frac >= 0 &&
             tongue_high_spike_frac >= 0 && tongue_high_spike_level >= 0);
}

#ifdef CLICKITONGUE_LINUX
std::string getHomeDir()
{
  char home_dir[1024];
  memset(home_dir, 0, 1024);
  strncpy(home_dir, getenv("HOME"), 1023);
  return std::string(home_dir);
}
#endif // CLICKITONGUE_LINUX

std::string getAndEnsureConfigDir()
{
#ifdef CLICKITONGUE_LINUX
  std::string config_dir = getHomeDir() + "/.config";
  mkdir(config_dir.c_str(), S_IRWXU);
  std::string clicki_config_dir = config_dir + "/clickitongue";
  mkdir(clicki_config_dir.c_str(), S_IRWXU);
  return clicki_config_dir + "/";
#endif // CLICKITONGUE_LINUX
#ifdef CLICKITONGUE_WINDOWS
  return "";
#endif // CLICKITONGUE_WINDOWS
#ifdef CLICKITONGUE_OSX
#error "OSX not yet supported"
    // TODO mkdir and set config_path
#endif // CLICKITONGUE_OSX
}

std::optional<Config> readConfig(std::string config_name)
{
  farfetchd::ConfigReader reader;
  if (!reader.parseFile(getAndEnsureConfigDir() + config_name + ".clickitongue"))
    return std::nullopt;

  return Config(reader);
}

bool writeConfig(Config config, std::string config_name)
{
  bool successful = true;
  std::string config_path;
  try
  {
    std::ofstream out(getAndEnsureConfigDir() + config_name + ".clickitongue");
    out << config.toString();
  }
  catch (std::exception const& e)
  {
    successful = false;
  }

  std::optional<Config> test_read = readConfig(config_name);
  if (!test_read.has_value())
    successful = false;
  if (!successful)
  {
    std::string m = "Attempt to write config file " + config_path + " failed.";
    promptInfo(m.c_str());
  }
  return successful;
}

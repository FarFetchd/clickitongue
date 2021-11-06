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
  if (tongue.enabled)
  {
    sts << "tongue_action: " << actionString(tongue.action) << "\n"
        << "tongue_low_hz: " << tongue.tongue_low_hz << "\n"
        << "tongue_high_hz: " << tongue.tongue_high_hz << "\n"
        << "tongue_hzenergy_high: " << tongue.tongue_hzenergy_high << "\n"
        << "tongue_hzenergy_low: " << tongue.tongue_hzenergy_low << "\n"
        << "tongue_refrac_blocks: " << tongue.refrac_blocks << "\n";
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

TongueConfig::TongueConfig(farfetchd::ConfigReader const& cfg)
{
  action = parseAction(cfg.getString("tongue_action").value_or("x"));
  tongue_low_hz = cfg.getDouble("tongue_low_hz").value_or(-1);
  tongue_high_hz = cfg.getDouble("tongue_high_hz").value_or(-1);
  tongue_hzenergy_high = cfg.getDouble("tongue_hzenergy_high").value_or(-1);
  tongue_hzenergy_low = cfg.getDouble("tongue_hzenergy_low").value_or(-1);
  refrac_blocks = cfg.getInt("tongue_refrac_blocks").value_or(-1);

  enabled = (action != Action::NoAction && tongue_low_hz >= 0 &&
              tongue_high_hz >= 0 && tongue_hzenergy_high >= 0 &&
              tongue_hzenergy_low >= 0 && refrac_blocks >= 0);
}

#ifdef CLICKITONGUE_LINUX
std::string getHomeDir()
{
  char home_dir[1024];
  memset(home_dir, 0, 1024);
  strncpy(home_dir, getenv("HOME"), 1023);
  return std::string(home_dir);
}
#define CONFIG_DIR_PATH getHomeDir() + "/.config/clickitongue/"
#endif // CLICKITONGUE_LINUX
#ifdef CLICKITONGUE_WINDOWS
#define CONFIG_DIR_PATH std::string("")
#endif // CLICKITONGUE_WINDOWS
#ifdef CLICKITONGUE_OSX
#error "OSX not yet supported"
    // TODO mkdir and set config_path
#endif // CLICKITONGUE_OSX

std::optional<Config> readConfig(std::string config_name)
{
  farfetchd::ConfigReader reader;
  if (!reader.parseFile(CONFIG_DIR_PATH + config_name + ".clickitongue"))
    return std::nullopt;

  return Config(reader);
}

bool writeConfig(Config config, std::string config_name)
{
  bool successful = true;
  std::string config_path;
  try
  {
#ifdef CLICKITONGUE_LINUX
    std::string config_dir = getHomeDir() + "/.config";
    std::string clicki_config_dir = config_dir + "/clickitongue";
    config_path = clicki_config_dir + "/" + config_name + ".clickitongue";
    mkdir(config_dir.c_str(), S_IRWXU);
    mkdir(clicki_config_dir.c_str(), S_IRWXU);
#endif // CLICKITONGUE_LINUX
#ifdef CLICKITONGUE_WINDOWS
    config_path = config_name + ".clickitongue";
#endif // CLICKITONGUE_WINDOWS
#ifdef CLICKITONGUE_OSX
#error "OSX not yet supported"
    // TODO mkdir and set config_path
#endif // CLICKITONGUE_OSX

    std::ofstream out(config_path);
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

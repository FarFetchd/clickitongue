#include "config_io.h"

#include <sys/stat.h>

#include <cstring>

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

std::string getConfigDir()
{
  char home_dir[1024];
  memset(home_dir, 0, 1024);
  strncpy(home_dir, getenv("HOME"), 1023); //TODO handle other OSes
  std::string config_path(home_dir);
  return config_path + "/.config/clickitongue";
}

std::optional<Config> readConfig()
{
  farfetchd::ConfigReader reader;
  if (!reader.parseFile(getConfigDir() + "/default.clickitongue"))
    return std::nullopt;

  return Config(reader);
}

void writeConfig(Config config)
{
  std::string config_dir = getConfigDir();
  mkdir(config_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // TODO other OSes... although msys might work for this, right?
  std::ofstream out(config_dir + "/default.clickitongue");

  if (config.blow.enabled)
  {
    out << "blow_action_on" << actionString(config.blow.action_on) << "\n"
        << "blow_action_off" << actionString(config.blow.action_off) << "\n"
        << "blow_lowpass_percent" << config.blow.lowpass_percent << "\n"
        << "blow_highpass_percent" << config.blow.highpass_percent << "\n"
        << "blow_low_on_thresh" << config.blow.low_on_thresh << "\n"
        << "blow_low_off_thresh" << config.blow.low_off_thresh << "\n"
        << "blow_high_on_thresh" << config.blow.high_on_thresh << "\n"
        << "blow_high_off_thresh" << config.blow.high_off_thresh << "\n"
        << "blow_high_spike_frac" << config.blow.high_spike_frac << "\n"
        << "blow_high_spike_level" << config.blow.high_spike_level << std::endl;
  }
  if (config.tongue.enabled)
  {
    out << "tongue_action" << actionString(config.tongue.action) << "\n"
        << "tongue_low_hz" << config.tongue.tongue_low_hz << "\n"
        << "tongue_high_hz" << config.tongue.tongue_high_hz << "\n"
        << "tongue_hzenergy_high" << config.tongue.tongue_hzenergy_high << "\n"
        << "tongue_hzenergy_low" << config.tongue.tongue_hzenergy_low << "\n"
        << "tongue_refrac_blocks" << config.tongue.refrac_blocks << std::endl;
  }
}

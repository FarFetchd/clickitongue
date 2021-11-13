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
        << "blow_o5_on_thresh: " << blow.o5_on_thresh << "\n"
        << "blow_o5_off_thresh: " << blow.o5_off_thresh << "\n"
        << "blow_o6_on_thresh: " << blow.o6_on_thresh << "\n"
        << "blow_o6_off_thresh: " << blow.o6_off_thresh << "\n"
        << "blow_o7_on_thresh: " << blow.o7_on_thresh << "\n"
        << "blow_o7_off_thresh: " << blow.o7_off_thresh << "\n"
        << "blow_ewma_alpha: " << blow.ewma_alpha << "\n";
  }
  if (hum.enabled)
  {
    sts << "hum_action_on: " << actionString(hum.action_on) << "\n"
        << "hum_action_off: " << actionString(hum.action_off) << "\n"
        << "hum_o1_on_thresh: " << hum.o1_on_thresh << "\n"
        << "hum_o1_off_thresh: " << hum.o1_off_thresh << "\n"
        << "hum_o6_limit: " << hum.o6_limit << "\n"
        << "hum_ewma_alpha: " << hum.ewma_alpha << "\n";
  }
  return sts.str();
}

BlowConfig::BlowConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("blow_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("blow_action_off").value_or("x"));
  o5_on_thresh = cfg.getDouble("blow_o5_on_thresh").value_or(-1);
  o5_off_thresh = cfg.getDouble("blow_o5_off_thresh").value_or(-1);
  o6_on_thresh = cfg.getDouble("blow_o6_on_thresh").value_or(-1);
  o6_off_thresh = cfg.getDouble("blow_o6_off_thresh").value_or(-1);
  o7_on_thresh = cfg.getDouble("blow_o7_on_thresh").value_or(-1);
  o7_off_thresh = cfg.getDouble("blow_o7_off_thresh").value_or(-1);
  ewma_alpha = cfg.getDouble("blow_ewma_alpha").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o5_on_thresh >= 0 && o5_off_thresh >= 0 && o6_on_thresh >= 0 &&
             o6_off_thresh >= 0 && o7_on_thresh >= 0 && o7_off_thresh >= 0 &&
             ewma_alpha >= 0);
}

HumConfig::HumConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("hum_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("hum_action_off").value_or("x"));
  o1_on_thresh = cfg.getDouble("hum_o1_on_thresh").value_or(-1);
  o1_off_thresh = cfg.getDouble("hum_o1_off_thresh").value_or(-1);
  o6_limit = cfg.getDouble("hum_o6_limit").value_or(-1);
  ewma_alpha = cfg.getDouble("hum_ewma_alpha").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o1_on_thresh >= 0 && o1_off_thresh >= 0 && o6_limit >= 0 &&
             ewma_alpha >= 0);
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

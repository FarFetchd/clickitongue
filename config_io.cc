#include "config_io.h"

#include <sys/stat.h>

#include <cstring>
#include <sstream>

Action parseAction(std::string str)
{
  if (str.find("LeftDown") != std::string::npos) return Action::LeftDown;
  if (str.find("LeftUp") != std::string::npos) return Action::LeftUp;
  if (str.find("RightDown") != std::string::npos) return Action::RightDown;
  if (str.find("RightUp") != std::string::npos) return Action::RightUp;
  if (str.find("ScrollDown") != std::string::npos) return Action::ScrollDown;
  if (str.find("ScrollUp") != std::string::npos) return Action::ScrollUp;
  if (str.find("RecordCurFrame") != std::string::npos) return Action::RecordCurFrame;

  return Action::NoAction;
}

std::string actionString(Action action)
{
  switch (action)
  {
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
        << "blow_o1_on_thresh: " << blow.o1_on_thresh << "\n"
        << "blow_o6_on_thresh: " << blow.o6_on_thresh << "\n"
        << "blow_o6_off_thresh: " << blow.o6_off_thresh << "\n"
        << "blow_o7_on_thresh: " << blow.o7_on_thresh << "\n"
        << "blow_o7_off_thresh: " << blow.o7_off_thresh << "\n"
        << "blow_ewma_alpha: " << blow.ewma_alpha << "\n"
        << "blow_scale: " << blow.scale << "\n";
  }
  if (cat.enabled)
  {
    sts << "cat_action_on: " << actionString(cat.action_on) << "\n"
        << "cat_action_off: " << actionString(cat.action_off) << "\n"
        << "cat_o1_limit: " << cat.o1_limit << "\n"
        << "cat_o6_on_thresh: " << cat.o6_on_thresh << "\n"
        << "cat_o6_off_thresh: " << cat.o6_off_thresh << "\n"
        << "cat_o7_on_thresh: " << cat.o7_on_thresh << "\n"
        << "cat_o7_off_thresh: " << cat.o7_off_thresh << "\n"
        << "cat_ewma_alpha: " << cat.ewma_alpha << "\n"
        << "cat_scale: " << cat.scale << "\n";
  }
  if (hum.enabled)
  {
    sts << "hum_action_on: " << actionString(hum.action_on) << "\n"
        << "hum_action_off: " << actionString(hum.action_off) << "\n"
        << "hum_o1_on_thresh: " << hum.o1_on_thresh << "\n"
        << "hum_o1_off_thresh: " << hum.o1_off_thresh << "\n"
        << "hum_o2_on_thresh: " << hum.o2_on_thresh << "\n"
        << "hum_o3_limit: " << hum.o3_limit << "\n"
        << "hum_o6_limit: " << hum.o6_limit << "\n"
        << "hum_ewma_alpha: " << hum.ewma_alpha << "\n"
        << "hum_scale: " << hum.scale << "\n";
  }
  if (sip.enabled)
  {
    sts << "sip_action_on: " << actionString(sip.action_on) << "\n"
        << "sip_action_off: " << actionString(sip.action_off) << "\n"
        << "sip_o7_on_thresh: " << sip.o7_on_thresh << "\n"
        << "sip_o7_off_thresh: " << sip.o7_off_thresh << "\n"
        << "sip_o1_limit: " << sip.o1_limit << "\n"
        << "sip_ewma_alpha: " << sip.ewma_alpha << "\n"
        << "sip_scale: " << sip.scale << "\n";
  }
  return sts.str();
}

BlowConfig::BlowConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("blow_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("blow_action_off").value_or("x"));
  o1_on_thresh = cfg.getDouble("blow_o1_on_thresh").value_or(-1);
  o6_on_thresh = cfg.getDouble("blow_o6_on_thresh").value_or(-1);
  o6_off_thresh = cfg.getDouble("blow_o6_off_thresh").value_or(-1);
  o7_on_thresh = cfg.getDouble("blow_o7_on_thresh").value_or(-1);
  o7_off_thresh = cfg.getDouble("blow_o7_off_thresh").value_or(-1);
  ewma_alpha = cfg.getDouble("blow_ewma_alpha").value_or(-1);
  scale = cfg.getDouble("blow_scale").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o1_on_thresh >= 0 && o6_on_thresh >= 0 && o6_off_thresh >= 0 &&
             o7_on_thresh >= 0 && o7_off_thresh >= 0 &&
             ewma_alpha >= 0 && scale >= 0);
}

CatConfig::CatConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("cat_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("cat_action_off").value_or("x"));
  o1_limit = cfg.getDouble("cat_o1_limit").value_or(-1);
  o6_on_thresh = cfg.getDouble("cat_o6_on_thresh").value_or(-1);
  o6_off_thresh = cfg.getDouble("cat_o6_off_thresh").value_or(-1);
  o7_on_thresh = cfg.getDouble("cat_o7_on_thresh").value_or(-1);
  o7_off_thresh = cfg.getDouble("cat_o7_off_thresh").value_or(-1);
  ewma_alpha = cfg.getDouble("cat_ewma_alpha").value_or(-1);
  scale = cfg.getDouble("cat_scale").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o1_limit >= 0 && o6_on_thresh >= 0 && o6_off_thresh >= 0 &&
             o7_on_thresh >= 0 && o7_off_thresh >= 0 &&
             ewma_alpha >= 0 && scale >= 0);
}

HumConfig::HumConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("hum_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("hum_action_off").value_or("x"));
  o1_on_thresh = cfg.getDouble("hum_o1_on_thresh").value_or(-1);
  o1_off_thresh = cfg.getDouble("hum_o1_off_thresh").value_or(-1);
  o2_on_thresh = cfg.getDouble("hum_o2_on_thresh").value_or(-1);
  o3_limit = cfg.getDouble("hum_o3_limit").value_or(-1);
  o6_limit = cfg.getDouble("hum_o6_limit").value_or(-1);
  ewma_alpha = cfg.getDouble("hum_ewma_alpha").value_or(-1);
  scale = cfg.getDouble("hum_scale").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o1_on_thresh >= 0 && o1_off_thresh >= 0 && o2_on_thresh >= 0 &&
             o3_limit >= 0 && o6_limit >= 0 && ewma_alpha >= 0 && scale >= 0);
}

SipConfig::SipConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("sip_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("sip_action_off").value_or("x"));
  o7_on_thresh = cfg.getDouble("sip_o7_on_thresh").value_or(-1);
  o7_off_thresh = cfg.getDouble("sip_o7_off_thresh").value_or(-1);
  o1_limit = cfg.getDouble("sip_o1_limit").value_or(-1);
  ewma_alpha = cfg.getDouble("sip_ewma_alpha").value_or(-1);
  scale = cfg.getDouble("sip_scale").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o7_on_thresh >= 0 && o7_off_thresh >= 0 && o1_limit >= 0 &&
             ewma_alpha >= 0 && scale >= 0);
}

std::string getHomeDir()
{
  char home_dir[1024];
  memset(home_dir, 0, 1024);
  strncpy(home_dir, getenv("HOME"), 1023);
  return std::string(home_dir);
}

std::string getAndEnsureConfigDir()
{
#ifdef CLICKITONGUE_WINDOWS
  return "";
#else
  std::string config_dir = getHomeDir() + "/.config";
  mkdir(config_dir.c_str(), S_IRWXU);
  std::string clicki_config_dir = config_dir + "/clickitongue";
  mkdir(clicki_config_dir.c_str(), S_IRWXU);
  return clicki_config_dir + "/";
#endif
}

std::optional<Config> readConfig(std::string config_name)
{
  farfetchd::ConfigReader reader;
  if (!reader.parseFile(getAndEnsureConfigDir() + config_name + ".clickitongue"))
    return std::nullopt;

  return Config(reader);
}

bool writeConfig(Config config, std::string config_name,
                 std::string* attempted_filepath)
{
  bool successful = true;
  *attempted_filepath = getAndEnsureConfigDir() + config_name + ".clickitongue";
  try
  {
    std::ofstream out(*attempted_filepath);
    out << config.toString();
  }
  catch (std::exception const& e)
  {
    successful = false;
  }

  std::optional<Config> test_read = readConfig(config_name);
  if (!test_read.has_value())
    successful = false;
  return successful;
}

std::optional<int> loadDeviceConfig(std::vector<std::string> dev_names)
{
  farfetchd::ConfigReader reader;
  if (!reader.parseFile(getAndEnsureConfigDir() + "audio_input_device.config"))
    return std::nullopt;
  int ind;
  if (reader.getInt("dev_ind").has_value())
    ind = reader.getInt("dev_ind").value();
  else
    return std::nullopt;
  std::string name;
  if (reader.getString("dev_name").has_value())
    name = reader.getString("dev_name").value();
  else
    return std::nullopt;

  if (ind < dev_names.size() && dev_names[ind] == name)
    return ind;
  return std::nullopt;
}

int writeDeviceConfig(std::vector<std::string> dev_names, int chosen)
{
  std::string fname = getAndEnsureConfigDir() + "audio_input_device.config";
  try
  {
    std::ofstream out(fname);
    out << "dev_ind: " << chosen << std::endl
        << "dev_name: " << dev_names[chosen] << std::endl;
  }
  catch (std::exception const& e) {}
  return chosen;
}

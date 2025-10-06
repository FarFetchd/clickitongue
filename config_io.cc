#include "config_io.h"

#include <sys/stat.h>

#include <cstring>
#include <sstream>

#include "interaction.h"

Action parseAction(std::string str)
{
  if (str.find("LeftDown") != std::string::npos) return Action::LeftDown;
  if (str.find("LeftUp") != std::string::npos) return Action::LeftUp;
  if (str.find("RightDown") != std::string::npos) return Action::RightDown;
  if (str.find("RightUp") != std::string::npos) return Action::RightUp;
  if (str.find("ScrollDown") != std::string::npos) return Action::ScrollDown;
  if (str.find("ScrollUp") != std::string::npos) return Action::ScrollUp;
  if (str.find("RecordCurFrame") != std::string::npos) return Action::RecordCurFrame;
  if (str.find("CopyPaste") != std::string::npos) return Action::CopyPaste;
  if (str.find("JustCopy") != std::string::npos) return Action::JustCopy;
  if (str.find("JustPaste") != std::string::npos) return Action::JustPaste;
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
    case Action::CopyPaste: return "CopyPaste";
    case Action::JustCopy: return "JustCopy";
    case Action::JustPaste: return "JustPaste";
    case Action::NoAction: return "NoAction";
    default: return "!!!!!actionString() was given an unknown action!!!!!";
  }
}

void crash(const char* s);
char g_athene_url[512];
char g_whisper_url[512];
Config::Config(farfetchd::ConfigReader const& cfg) : blow(cfg), cat(cfg), hum(cfg)
{
  athene_url = cfg.getString("athene_url").value_or("none");
  if (athene_url.size() > 511)
    crash("athene_url longer than 511 chars");
  strcpy(g_athene_url, athene_url.c_str());
  whisper_url = cfg.getString("whisper_url").value_or("none");
  if (whisper_url.size() > 511)
    crash("whisper_url longer than 511 chars");
  strcpy(g_whisper_url, whisper_url.c_str());
}

std::string Config::toString() const
{
  std::stringstream sts;
  if (blow.enabled)
  {
    sts << "blow_action_on: " << actionString(blow.action_on) << "\n"
        << "blow_action_off: " << actionString(blow.action_off) << "\n"
        << "blow_o1_on_thresh: " << blow.o1_on_thresh << "\n"
        << "blow_o7_on_thresh: " << blow.o7_on_thresh << "\n"
        << "blow_o7_off_thresh: " << blow.o7_off_thresh << "\n"
        << "blow_lookback_blocks: " << blow.lookback_blocks << "\n"
        << "blow_scale: " << blow.scale << "\n";
  }
  if (cat.enabled)
  {
    sts << "cat_action_on: " << actionString(cat.action_on) << "\n"
        << "cat_action_off: " << actionString(cat.action_off) << "\n"
        << "cat_o7_on_thresh: " << cat.o7_on_thresh << "\n"
        << "cat_o1_limit: " << cat.o1_limit << "\n"
        << "cat_use_limit: " << (cat.use_limit ? "true" : "false") << "\n"
        << "cat_scale: " << cat.scale << "\n";
  }
  if (hum.enabled)
  {
    sts << "hum_action_on: " << actionString(hum.action_on) << "\n"
        << "hum_action_off: " << actionString(hum.action_off) << "\n"
        << "hum_o1_on_thresh: " << hum.o1_on_thresh << "\n"
        << "hum_o1_off_thresh: " << hum.o1_off_thresh << "\n"
        << "hum_o6_limit: " << hum.o6_limit << "\n"
        << "hum_ewma_alpha: " << hum.ewma_alpha << "\n"
        << "hum_scale: " << hum.scale << "\n";
  }
  sts << "whisper_url: " << whisper_url << "\n";
  sts << "athene_url: " << athene_url << "\n";
  return sts.str();
}

BlowConfig::BlowConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("blow_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("blow_action_off").value_or("x"));
  o1_on_thresh = cfg.getDouble("blow_o1_on_thresh").value_or(-1);
  o7_on_thresh = cfg.getDouble("blow_o7_on_thresh").value_or(-1);
  o7_off_thresh = cfg.getDouble("blow_o7_off_thresh").value_or(-1);
  lookback_blocks = cfg.getInt("blow_lookback_blocks").value_or(-1);
  scale = cfg.getDouble("blow_scale").value_or(-1);

  enabled = (action_on != Action::NoAction && action_off != Action::NoAction &&
             o1_on_thresh >= 0 && o7_on_thresh >= 0 && o7_off_thresh >= 0 &&
             lookback_blocks >= 0 && scale >= 0);
}

CatConfig::CatConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("cat_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("cat_action_off").value_or("x"));
  o7_on_thresh = cfg.getDouble("cat_o7_on_thresh").value_or(-1);
  o1_limit = cfg.getDouble("cat_o1_limit").value_or(-1);
  use_limit = (cfg.getString("cat_use_limit").value_or("false") == "true");
  scale = cfg.getDouble("cat_scale").value_or(-1);

  enabled = (action_on != Action::NoAction && o7_on_thresh >= 0 && o1_limit >= 0 && scale >= 0);
}

HumConfig::HumConfig(farfetchd::ConfigReader const& cfg)
{
  action_on = parseAction(cfg.getString("hum_action_on").value_or("x"));
  action_off = parseAction(cfg.getString("hum_action_off").value_or("x"));
  o1_on_thresh = cfg.getDouble("hum_o1_on_thresh").value_or(-1);
  o1_off_thresh = cfg.getDouble("hum_o1_off_thresh").value_or(-1);
  o6_limit = cfg.getDouble("hum_o6_limit").value_or(-1);
  ewma_alpha = cfg.getDouble("hum_ewma_alpha").value_or(-1);
  scale = cfg.getDouble("hum_scale").value_or(-1);

  enabled = (action_on != Action::NoAction &&
             o1_on_thresh >= 0 && o1_off_thresh >= 0 && o6_limit >= 0 &&
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

std::string getConfigDir()
{
#ifdef CLICKITONGUE_WINDOWS
  return "";
#else
  std::string config_dir = getHomeDir() + "/.config";
  std::string clicki_config_dir = config_dir + "/clickitongue";
  return clicki_config_dir + "/";
#endif
}

void writePythonToConfigDir()
{
const char kOurPython[] =
R"(
import os
import sys
import time
import requests
import subprocess

def remove_markdown_backticks(s):
    lines = s.splitlines()
    if lines and lines[0].strip().startswith('```'):
        lines = lines[1:]
    if lines and lines[-1].strip().endswith('```'):
        lines = lines[:-1]
    return '\n'.join(lines)

def clean_output_from_response(resp):
    the_out = resp.json()['content']
    if '```' in the_out:
        the_out = remove_markdown_backticks(the_out)
    return the_out.strip()



def atheneDictate(whisper_url, athene_url):
    result = subprocess.run(['pgrep', '-x', 'Xorg'], check=False, stdout=subprocess.PIPE)
    is_x11 = (result.returncode == 0)

    if is_x11:
        whisper_cmd = 'xsel --clipboard -o >/tmp/scifiscribe_whisper_prompt.txt 2>/dev/null'
    else:
        whisper_cmd = 'wl-paste >/tmp/scifiscribe_whisper_prompt.txt'
    whisper_cmd += f' ; curl {whisper_url} -H "Content-Type: multipart/form-data" -F file=@/tmp/scifiscribe_to_whisper.wav -F temperature="0.0" -F temperature_inc="0.2" -F response_format="text" >/tmp/scifiscribe_whisper_out.txt 2>/dev/null'
    os.system(whisper_cmd)

    with open('/tmp/scifiscribe_whisper_prompt.txt', 'r', encoding='utf-8') as file:
        context = file.read().strip()
    with open('/tmp/scifiscribe_whisper_out.txt', 'r', encoding='utf-8') as file:
        whisper_out = file.read().strip()

    print(f'preceding context:\n{context}\n\n')
    print(f'whisper output:\n{whisper_out}\n\n')

    our_request_to_llm = f'You are formatting the output of a speech recognition system. I will give you the raw speech recognition output, plus the text immediately preceding where the user wants the output inserted. Print a version of the raw output formatted to naturally follow after the given preceding text. It is likely (but not guaranteed) that the text being worked on is source code. If the preceding text is empty, assume not code. I will be directly pasting your output into the text editor. Now, here is the preceding context (demarcated by end_of_context):\n{context}\n\nend_of_context\nAnd here is the speech recognition raw output:\n{whisper_out}'
    full_prompt = f'<|im_start|>system\nYou are Qwen, created by Alibaba Cloud. You are a helpful assistant.<|im_end|>\n<|im_start|>user\n{our_request_to_llm}<|im_end|>\n<|im_start|>assistant\n'
    req_json = {
        'prompt': full_prompt,
        'n_predict': -1,
        'temperature': 0
    }
    response = requests.post(athene_url, json=req_json)
    final_output = clean_output_from_response(response)
    print(f'about to paste LLM output:\n{final_output}\n\n')
    if is_x11:
        subprocess.run(['xsel', '--clipboard', '-i'], input=final_output.encode('utf-8'))
    else:
        subprocess.run(['wl-copy'], input=final_output.encode('utf-8'))


def describeToWhisperAndBack(whisper_url, athene_url):
    result = subprocess.run(['pgrep', '-x', 'Xorg'], check=False, stdout=subprocess.PIPE)
    is_x11 = (result.returncode == 0)

    if is_x11:
        whisper_cmd = 'xsel --clipboard -o >/tmp/scifiscribe_whisper_prompt.txt 2>/dev/null'
    else:
        whisper_cmd = 'wl-paste >/tmp/scifiscribe_whisper_prompt.txt'
    whisper_cmd += f' ; curl {whisper_url} -H "Content-Type: multipart/form-data" -F file=@/tmp/scifiscribe_to_whisper.wav -F temperature="0.0" -F temperature_inc="0.2" -F response_format="text" >/tmp/scifiscribe_whisper_out.txt 2>/dev/null'
    os.system(whisper_cmd)

    with open('/tmp/scifiscribe_whisper_prompt.txt', 'r', encoding='utf-8') as file:
        context = file.read().strip()
    with open('/tmp/scifiscribe_whisper_out.txt', 'r', encoding='utf-8') as file:
        whisper_out = file.read().strip()

    print(f'whisper output:\n{whisper_out}\n\n')

    if context:
        our_request_to_llm = f'You are modifying some existing code, as specified by a description coming from speech recognition. Keep in mind that if the user referred to a variable or function name, the speech recognition is likely to have presented it as multiple English words, like "some function" rather than some_function(). Now, here is the existing code:\n```\n{context}\n```\n...and here is the user\'s description:\n{whisper_out}\n(end of description)\nYour output will be directly pasted into the editor, so please write the code and nothing else: no descriptions or explanations.'
    else:
        our_request_to_llm = f'Please write code according to the following description. Your output will be directly pasted into the editor, so please write the code and nothing else: no descriptions or explanations. Here is the description: {whisper_out}'
    full_prompt = f'<|im_start|>system\nYou are Qwen, created by Alibaba Cloud. You are a helpful assistant.<|im_end|>\n<|im_start|>user\n{our_request_to_llm}<|im_end|>\n<|im_start|>assistant\n'
    req_json = {
        'prompt': full_prompt,
        'n_predict': -1,
        'temperature': 0
    }
    response = requests.post(athene_url, json=req_json)
    final_output = clean_output_from_response(response)
    print(f'about to paste LLM output:\n{final_output}\n\n')
    if is_x11:
        subprocess.run(['xsel', '--clipboard', '-i'], input=final_output.encode('utf-8'))
    else:
        subprocess.run(['wl-copy'], input=final_output.encode('utf-8'))



if sys.argv[1] == 'dictateAndClipboardToWhisperAndBack':
    atheneDictate(sys.argv[2], sys.argv[3])
else:
    describeToWhisperAndBack(sys.argv[2], sys.argv[3])
)";
  std::string full_path = getAndEnsureConfigDir() + "clickitongue_voice_to_llm.py";
  try
  {
    std::ofstream out(full_path);
    out << kOurPython;
  }
  catch (std::exception const& e)
  {
    crash("failed to write python script to config dir");
  }
}

std::optional<Config> readConfig(std::string config_name)
{
  farfetchd::ConfigReader reader;
  std::string config_full_path = getAndEnsureConfigDir() + config_name + ".clickitongue";
  if (!reader.parseFile(config_full_path))
    return std::nullopt;
  PRINTF("loaded config from: %s\n", config_full_path.c_str());

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

  writePythonToConfigDir();

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

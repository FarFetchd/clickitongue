#include "main_train.h"

#include "audio_input.h"
#include "audio_recording.h"
#include "config_io.h"
#include "constants.h"
#include "interaction.h"
#include "train_blow.h"
#include "train_hum.h"

// returns try_blows
bool trainIntro(std::string* intro_message)
{
  chooseInputDevice();

  bool try_blows = promptYesNo(
"It looks like this is your first time running Clickitongue.\n"
"We'll now train Clickitongue on the acoustics and typical background noise\n"
"of your particular enivornment.\n\n"
"First: can you keep your mic positioned ~1cm from your mouth for long-term usage? ");

  if (try_blows)
  {
    *intro_message +=
"Great! You'll be able to do both left- and right-clicks, using blowing and\n"
"humming. Ensure your mic is in position ~1cm from your mouth - your index\n"
"finger should fit perfectly between mouth and mic. If your mic has a foamy/fuzzy\n"
"windscreen, remove it for best results.\n\n";
  }
  else
  {
    *intro_message +=
"Ok, we will fall back to left-clicks only, controlled by humming.\n\n";
  }
  return try_blows;
}

void trainingBody(bool try_blows, std::string* intro_message,
                  std::vector<std::pair<AudioRecording, int>>* blow_examples,
                  std::vector<std::pair<AudioRecording, int>>* hum_examples);

void firstTimeTrain()
{
  std::string intro_message;
  bool try_blows = trainIntro(&intro_message);
  std::vector<std::pair<AudioRecording, int>> blow_examples;
  std::vector<std::pair<AudioRecording, int>> hum_examples;
  trainingBody(try_blows, &intro_message, &blow_examples, &hum_examples);
}

void displayAndReset(std::string* msg)
{
  promptInfo(msg->c_str());
  *msg = "";

// TODO OSX
#ifdef CLICKITONGUE_LINUX
  printf("Press any key to continue to the training prompt.\n\n");
  make_getchar_like_getch(); getchar(); resetTermios();
#endif
}

bool afterTraining(Config config);
void normalOperation(Config config);

void trainingBody(bool try_blows, std::string* intro_message,
                  std::vector<std::pair<AudioRecording, int>>* blow_examples,
                  std::vector<std::pair<AudioRecording, int>>* hum_examples)
{
  if (try_blows && blow_examples->empty())
  {
    *intro_message +=
"We will first train Clickitongue on your blowing. These should be extremely\n"
"gentle puffs of air, blown directly onto the mic. Don't try too hard to make\n"
"these training examples strong and clear, or else Clickitongue will expect\n"
"that much strength every time. These puffs should be more focused than simply\n"
"exhaling through an open mouth, but weaker than just about any other blow.\n"
"Think of trying to propel an eyelash over a hand's width.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of blows.\n\n";
    displayAndReset(intro_message);
    for (int i = 0; i < 6; i++)
      blow_examples->emplace_back(recordExampleBlow(i), i);
    blow_examples->emplace_back(recordExampleBlow(1, /*prolonged=*/true), 1);
  }
  {
    *intro_message +=
"We will now train Clickitongue on your humming. These hums should be simple,\n"
"relatively quiet closed-mouth hums, like saying 'hm' in reaction to something\n"
"just a tiny bit interesting.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of hums.\n\n";
    displayAndReset(intro_message);
    for (int i = 0; i < 6; i++)
      hum_examples->emplace_back(recordExampleHum(i), i);
    hum_examples->emplace_back(recordExampleHum(1, /*prolonged=*/true), 1);
  }

  // all examples of one are negative examples for the other
  std::vector<std::pair<AudioRecording, int>> hum_examples_plus_neg;
  std::vector<std::pair<AudioRecording, int>> blow_examples_plus_neg;
  for (auto const& ex_pair : *blow_examples)
    blow_examples_plus_neg.emplace_back(ex_pair.first, ex_pair.second);
  for (auto const& ex_pair : *hum_examples)
    hum_examples_plus_neg.emplace_back(ex_pair.first, ex_pair.second);
  for (auto const& ex_pair : *blow_examples)
    hum_examples_plus_neg.emplace_back(ex_pair.first, 0);
  for (auto const& ex_pair : *hum_examples)
    blow_examples_plus_neg.emplace_back(ex_pair.first, 0);

  double scale = 1.0;
  scale = pickHumScalingFactor(hum_examples_plus_neg);
  printf("using scale %g\n", scale);

  Config config;
  if (try_blows)
    config.blow = trainBlow(blow_examples_plus_neg, scale);
  config.hum = trainHum(hum_examples_plus_neg, scale);

  if (!config.hum.enabled || (try_blows && !config.blow.enabled))
  {
    std::string msg;
    if (!config.hum.enabled)
    {
      msg += "Clickitongue was not able to learn to detect your humming.\n\n";
      hum_examples->clear();
    }
    if (try_blows && !config.blow.enabled)
    {
      msg += "Clickitongue was not able to learn to detect your blowing.\n\n";
      blow_examples->clear();
    }
    msg += "Would you like to redo the training?\n\n";
    if (config.hum.enabled || config.blow.enabled)
      msg += "(If you leave it as is, you will only be able to left click.)";
    if (promptYesNo(msg.c_str()))
    {
      trainingBody(try_blows, intro_message, blow_examples, hum_examples);
      return;
    }
  }
  if (afterTraining(config))
    normalOperation(config);
}

// returns true if we can proceed to normalOperation().
bool afterTraining(Config config)
{
  std::string success_msg;
  // TODO whistle
  if (config.blow.enabled && config.hum.enabled)
  {
    config.blow.action_on = Action::LeftDown;
    config.blow.action_off = Action::LeftUp;
    config.hum.action_on = Action::RightDown;
    config.hum.action_off = Action::RightUp;
    success_msg =
"Clickitongue is now configured. Blow on the mic to left click, hum for right.\n\n";
  }
  else if (config.blow.enabled)
  {
    config.blow.action_on = Action::LeftDown;
    config.blow.action_off = Action::LeftUp;
    success_msg =
"Clickitongue is now configured. Blow on the mic to left click.\n\n";
  }
  else if (config.hum.enabled)
  {
    config.hum.action_on = Action::LeftDown;
    config.hum.action_off = Action::LeftUp;
    success_msg =
"Clickitongue is now configured. Hum to left click.\n\n";
  }
  else
  {
    promptInfo(
"Clickitongue was not able to find parameters that distinguish your humming\n"
"from background noise with acceptable accuracy. Remove sources of\n"
"background noise, move the mic closer to your mouth, and try again.");
    return false;
  }
#ifdef CLICKITONGUE_WINDOWS
  success_msg +=
"If you ever want to redo this training, delete the file default.clickitongue "
"that you should find right next to clickitongue.exe, and then run "
"clickitongue.exe again.\n\nNow entering normal operation.";
#else
  success_msg +=
"If you ever want to redo this training, run clickitongue with the flag --retrain."
"\n\nNow entering normal operation.";
#endif
  promptInfo(success_msg.c_str());

#ifdef CLICKITONGUE_WINDOWS
  promptInfo(
"A note on admin privileges:\n\nWindows does not allow lesser privileged "
"processes to send clicks to processes running as admin. If you ever find that "
"just one particular window refuses to listen to Clickitongue's clicks, try "
"running Clickitongue as admin. (But you might never encounter this problem.)");
#endif

  std::string attempted_filepath;
  if (!writeConfig(config, kDefaultConfig, &attempted_filepath))
  {
    std::string msg = "Failed to write config file " + attempted_filepath +
    ". You'll have to redo this training the next time you run Clickitongue.";
    promptInfo(msg.c_str());
  }
  return true;
}

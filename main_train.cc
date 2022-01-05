#include "main_train.h"

#include <cassert>

#include "audio_input.h"
#include "audio_recording.h"
#include "config_io.h"
#include "constants.h"
#include "interaction.h"
#include "train_blow.h"
#include "train_cat.h"
#include "train_hissing_sip.h"
#include "train_hum.h"

using TaggedExamples = std::vector<std::pair<AudioRecording, int>>;

bool introAndAskIfMicNearMouth()
{
  chooseInputDevice();

  bool try_blows = promptYesNo(
"It looks like this is your first time running Clickitongue.\n\n"
"Clickitongue needs to learn your environment's acoustics and background noise.\n\n"
"First: can you keep your mic positioned 1-2cm from your mouth for long-term usage? ");

  if (try_blows)
  {
    promptInfo(
"Great! That enables more comfortable input methods. You'll now train\n"
"Clickitongue to recognize each of the following sounds:\n\n"
" * Gently blowing directly on the mic\n\n"
" * 'tchk' sucking/clicking noises, like calling a cat\n\n"
" * Humming\n\n"
" * A hissing inhalation, like reacting to touching something hot\n\n"
"Ensure your mic is in position 1-2cm from your mouth. If your mic has a foamy or\n"
"fuzzy windscreen, remove it for best results.\n\n");
  }
  else
  {
    promptInfo(
"That's ok, Clickitongue should still work. You'll now train Clickitongue to\n"
"recognize humming, and 'tchk' sucking/clicking noises (like calling a cat).\n\n");
  }
#ifndef CLICKITONGUE_WINDOWS
  PRINTF("Press any key to continue to the training prompt.\n\n");
  make_getchar_like_getch(); getchar(); resetTermios();
#endif
  return try_blows;
}

// pass null to skip trying to train a type
void collectAnyMissingExamples(
    TaggedExamples* blow_examples, TaggedExamples* cat_examples,
    TaggedExamples* hum_examples, TaggedExamples* sip_examples)
{
  if (blow_examples && blow_examples->empty())
  {
    promptInfo(
"We will first train Clickitongue on your blowing. These should be extremely\n"
"gentle puffs of air, blown directly onto the mic. Don't try too hard to make\n"
"these training examples strong and clear, or else Clickitongue will expect\n"
"that much strength every time. These puffs should be more focused than simply\n"
"exhaling through an open mouth, but weaker than just about any other blow.\n"
"Think of trying to propel an eyelash over a hand's width.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of blows.\n\n");
#ifndef CLICKITONGUE_WINDOWS
    PRINTF("Press any key to continue to the training prompt.\n\n");
    make_getchar_like_getch(); getchar(); resetTermios();
#endif
    for (int i = 0; i < 6; i++)
      blow_examples->emplace_back(recordExampleBlow(i), i);
    blow_examples->emplace_back(recordExampleBlow(1, /*prolonged=*/true), 1);
  }
  if (cat_examples && cat_examples->empty())
  {
    promptInfo(
"We will now train Clickitongue on your 'tchk'-ing - the sound you make when\n"
"you want to get a cat's attention.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of 'tchk's.\n\n"
"When the training asks you to do multiple, don't do them too quickly:\n"
"no faster than four per second.\n\n");
#ifndef CLICKITONGUE_WINDOWS
    PRINTF("Press any key to continue to the training prompt.\n\n");
    make_getchar_like_getch(); getchar(); resetTermios();
#endif
    for (int i = 0; i < 6; i++)
      cat_examples->emplace_back(recordExampleCat(i), i);
  }
  if (hum_examples && hum_examples->empty())
  {
    promptInfo(
"We will now train Clickitongue on your humming. These hums should be simple,\n"
"relatively quiet closed-mouth hums, like saying 'hm' in reaction to something\n"
"just a tiny bit interesting.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of hums.\n\n");
#ifndef CLICKITONGUE_WINDOWS
    PRINTF("Press any key to continue to the training prompt.\n\n");
    make_getchar_like_getch(); getchar(); resetTermios();
#endif
    for (int i = 0; i < 6; i++)
      hum_examples->emplace_back(recordExampleHum(i), i);
    hum_examples->emplace_back(recordExampleHum(1, /*prolonged=*/true), 1);
  }
  if (sip_examples && sip_examples->empty())
  {
    promptInfo(
"We will now train Clickitongue on hissing inhales. The goal is to make a 'ssss'\n"
"sound by inhaling through your mouth with your teeth closed. No special\n"
"technique is needed; your lips and tongue can be neutral. Just keep the teeth\n"
"together.\n\n"
"The training will record several 4-second snippets, during each of which you\n"
"will be asked to do a specific number of hissing inhales.\n\n");
#ifndef CLICKITONGUE_WINDOWS
    PRINTF("Press any key to continue to the training prompt.\n\n");
    make_getchar_like_getch(); getchar(); resetTermios();
#endif
    for (int i = 0; i < 6; i++)
      sip_examples->emplace_back(recordExampleSip(i), i);
    sip_examples->emplace_back(recordExampleSip(1, /*prolonged=*/true), 1);
  }
}

TaggedExamples combinePositiveAndNegative(TaggedExamples* positive,
                                          std::vector<TaggedExamples*> negatives)
{
  if (!positive)
    return {};
  assert(!positive->empty());
  TaggedExamples ret;
  for (auto const& ex_pair : *positive)
    ret.emplace_back(ex_pair.first, ex_pair.second);
  for (TaggedExamples* negative_set : negatives)
    if (negative_set)
      for (auto const& ex_pair : *negative_set)
        ret.emplace_back(ex_pair.first, 0);
  return ret;
}

bool afterTraining(Config* config, int success_count);
void normalOperation(Config config, bool first_time);

// pass null to skip trying to train a type
void trainingBody(TaggedExamples* blow_examples, TaggedExamples* cat_examples,
                  TaggedExamples* hum_examples, TaggedExamples* sip_examples)
{
  collectAnyMissingExamples(blow_examples, cat_examples,
                            hum_examples, sip_examples);

  // all examples of one are negative examples for the other
  TaggedExamples blow_examples_plus_neg = combinePositiveAndNegative(
      blow_examples, {cat_examples, hum_examples, sip_examples});
  TaggedExamples cat_examples_plus_neg = combinePositiveAndNegative(
      cat_examples, {blow_examples, hum_examples, sip_examples});
  TaggedExamples hum_examples_plus_neg = combinePositiveAndNegative(
      hum_examples, {blow_examples, cat_examples, sip_examples});
  TaggedExamples sip_examples_plus_neg = combinePositiveAndNegative(
      sip_examples, {blow_examples, cat_examples, hum_examples});

  assert(hum_examples && !hum_examples->empty());
  double scale = pickHumScalingFactor(*hum_examples);
  PRINTF("using scale %g\n", scale);

  Config config;
  if (!blow_examples_plus_neg.empty())
    config.blow = trainBlow(blow_examples_plus_neg, scale);
  if (!cat_examples_plus_neg.empty())
    config.cat = trainCat(cat_examples_plus_neg, scale);
  if (!hum_examples_plus_neg.empty())
    config.hum = trainHum(hum_examples_plus_neg, scale);
  if (!sip_examples_plus_neg.empty())
    config.sip = trainSip(sip_examples_plus_neg, scale);

  std::string failure_list;
  if (blow_examples && !config.blow.enabled)
  {
    failure_list += "blowing, ";
    blow_examples->clear();
  }
  if (cat_examples && !config.cat.enabled)
  {
    failure_list += "cat 'tchk'-ing, ";
    cat_examples->clear();
  }
  if (hum_examples && !config.hum.enabled)
  {
    failure_list += "humming, ";
    hum_examples->clear();
  }
  if (sip_examples && !config.sip.enabled)
  {
    failure_list += "hiss-inhaling, ";
    sip_examples->clear();
  }
  if (!failure_list.empty())
    failure_list.erase(failure_list.end() - 2, failure_list.end());
  int success_count = (config.blow.enabled? 1:0) + (config.cat.enabled? 1:0) +
                      (config.hum.enabled? 1:0) + (config.sip.enabled? 1:0);
  int sounds_attempted = (blow_examples? 1:0) + (cat_examples? 1:0) +
                         (hum_examples? 1:0) + (sip_examples? 1:0);
  if (success_count < sounds_attempted)
  {
    std::string msg =
"Clickitongue was not able to learn to detect the following sound types:\n\n" + failure_list + ".\n\n";
    if (success_count == 1)
    {
      msg += "If you leave it as-is, you will only be able to left-click.\n\n";
    }
    msg += "Would you like to retry the problematic types?";
    if (success_count >= 2)
    {
      msg += "\n\n("+std::to_string(success_count)+
" types were good, so you can left- and right- click even without retrying)";
    }
    if (promptYesNo(msg.c_str()))
    {
      trainingBody(blow_examples, cat_examples, hum_examples, sip_examples);
      return;
    }
  }

  if (afterTraining(&config, success_count))
    normalOperation(config, /*first_time=*/true);
}

void firstTimeTrain()
{
  TaggedExamples blow_examples;
  TaggedExamples cat_examples;
  TaggedExamples hum_examples;
  TaggedExamples sip_examples;
  if (introAndAskIfMicNearMouth())
    trainingBody(&blow_examples, &cat_examples, &hum_examples, &sip_examples);
  else
    trainingBody(nullptr, &cat_examples, &hum_examples, nullptr);
}

#define ASSIGN_LEFT_OR_RIGHT_CLICK(x) if (config->x.enabled) \
{ \
  config->x.action_on = left_done ? Action::RightDown : Action::LeftDown; \
  config->x.action_off = left_done ? Action::RightUp : Action::LeftUp; \
  left_done = true; \
}

// returns true if we can proceed to normalOperation().
bool afterTraining(Config* config, int success_count)
{
  if (success_count == 0)
  {
    promptInfo(
"Clickitongue was not able to find parameters that distinguish your humming\n"
"from background noise with acceptable accuracy. Remove sources of\n"
"background noise, move the mic closer to your mouth, and try again.");
    return false;
  }
  else if (success_count <= 2)
  {
    bool left_done = false;
    // (not just alphabetical, this is the preference i think makes sense)
    ASSIGN_LEFT_OR_RIGHT_CLICK(blow);
    ASSIGN_LEFT_OR_RIGHT_CLICK(cat);
    ASSIGN_LEFT_OR_RIGHT_CLICK(hum);
    ASSIGN_LEFT_OR_RIGHT_CLICK(sip);
  }
  else if (success_count == 4 || (success_count == 3 && config->blow.enabled))
  {
    config->blow.action_on = Action::LeftDown;
    config->blow.action_off = Action::LeftUp;
    while (true)
    {
      if (config->cat.enabled && promptYesNo("Use cat 'tchk's for right click?"))
      {
        config->cat.action_on = Action::RightDown;
        config->cat.action_off = Action::RightUp;
        break;
      }
      if (config->hum.enabled && promptYesNo("Use humming for right click?"))
      {
        config->hum.action_on = Action::RightDown;
        config->hum.action_off = Action::RightUp;
        break;
      }
      if (config->sip.enabled && promptYesNo("Use hissing inhales for right click?"))
      {
        config->sip.action_on = Action::RightDown;
        config->sip.action_off = Action::RightUp;
        break;
      }
    }
  }
  else if (success_count == 3 && !config->blow.enabled)
  {
    bool cat_left = promptYesNo(
"Would you like to use cat 'tchk's for left click? It's more comfortable than\n"
"humming, but you won't be able to long click (select, drag+drop).");

    std::string right_prompt =
"Would you like to use hissing inhales for right click? If not, we'll use\n";
    right_prompt += (cat_left ? "humming" : "cat 'tchk's");
    right_prompt += " for right click.";
    bool sip_right = promptYesNo(right_prompt.c_str());

    if (cat_left)
    {
      config->cat.action_on = Action::LeftDown;
      config->cat.action_off = Action::LeftUp;
      if (!sip_right)
      {
        config->hum.action_on = Action::RightDown;
        config->hum.action_off = Action::RightUp;
      }
    }
    else
    {
      config->hum.action_on = Action::LeftDown;
      config->hum.action_off = Action::LeftUp;
      if (!sip_right)
      {
        config->cat.action_on = Action::RightDown;
        config->cat.action_off = Action::RightUp;
      }
    }
    if (sip_right)
    {
      config->sip.action_on = Action::RightDown;
      config->sip.action_off = Action::RightUp;
    }
  }
  assert(success_count <= 4);

#ifdef CLICKITONGUE_WINDOWS
  promptInfo(
"A note on admin privileges:\n\nWindows does not allow lesser privileged "
"processes to send clicks to processes running as admin. If you ever find that "
"just one particular window refuses to listen to Clickitongue's clicks, try "
"running Clickitongue as admin. (But you might never encounter this problem.)");
#endif

  std::string attempted_filepath;
  if (!writeConfig(*config, kDefaultConfig, &attempted_filepath))
  {
    std::string msg = "Failed to write config file " + attempted_filepath +
    ". You'll have to redo this training the next time you run Clickitongue.";
    promptInfo(msg.c_str());
  }
  return true;
}

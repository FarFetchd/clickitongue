#include "main_train.h"

#include <cassert>

#include "audio_input.h"
#include "audio_recording.h"
#include "config_io.h"
#include "constants.h"
#include "interaction.h"
#include "train_blow.h"
#include "train_cat.h"
#include "train_hum.h"

using TaggedExamples = std::vector<std::pair<AudioRecording, int>>;

bool introAndAskIfMicNearMouth()
{
  chooseInputDevice();

  bool mic_near_mouth = promptYesNo(
"It looks like this is your first time running Clickitongue.\n\n"
"Clickitongue needs to learn your environment's acoustics and background noise.\n\n"
"First: can you keep your mic positioned about 2cm from your mouth for long-term\n"
"usage? ");

  if (mic_near_mouth)
  {
    promptInfo(
"Great! That enables more comfortable input methods. You'll now train\n"
"Clickitongue to recognize each of the following sounds:\n\n"
" * Gently blowing directly on the mic\n\n"
" * Trying to get a cat's attention\n\n"
" * Humming\n\n"
"Ensure your mic is in position about 2cm from your mouth. If your mic has a\n"
"foamy or fuzzy windscreen, remove it for best results.\n\n");
  }
  else
  {
    promptInfo(
"That's ok, Clickitongue should still work. You'll now train Clickitongue to\n"
"recognize getting a cat's attention, and humming.\n\n");
  }
#ifndef CLICKITONGUE_WINDOWS
  PRINTF("Press any key to continue to the training prompt.\n\n");
  make_getchar_like_getch(); getchar(); resetTermios();
#endif
  return mic_near_mouth;
}

// pass null to skip trying to train a type
void collectAnyMissingExamples(
    TaggedExamples* blow_examples, TaggedExamples* cat_examples,
    TaggedExamples* hum_examples)
{
  if (blow_examples && blow_examples->empty())
  {
    promptInfo(
"We will first train Clickitongue on your blowing. These should be extremely\n"
"gentle puffs of air, blown directly onto the mic. Don't try too hard to make\n"
"these training examples strong and clear, or else Clickitongue will expect\n"
"that much strength every time. These puffs should be more focused than simply\n"
"exhaling through an open mouth, but weaker than just about any other blow.\n"
"Think of trying to move an eyelash by about a hand's width.\n\n"
"The training will record several 5-second snippets, during each of which you\n"
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
"We will now train Clickitongue on the sound you make when you want to get a\n"
"cat's attention. This can be 'tchk' sucking/clicking noises like a rodent makes,\n"
"kisses, or short constrained 'ts' sounds. Any of these three can work, so try\n"
"whichever you like. Note that 'pss pss' does NOT work.\n\n"
"The training will record several 5-second snippets, during each of which you\n"
"will be asked to do a specific number of sounds.\n\n"
"When the training asks you to do multiple, don't do them super quickly:\n"
"no faster than four or five per second.\n\n");
#ifndef CLICKITONGUE_WINDOWS
    PRINTF("Press any key to continue to the training prompt.\n\n");
    make_getchar_like_getch(); getchar(); resetTermios();
#endif
    for (int i = 0; i < 6; i++)
      cat_examples->emplace_back(recordExampleCat(i), i);
    cat_examples->emplace_back(recordExampleCat(8), 8);
  }
  if (hum_examples && hum_examples->empty())
  {
    promptInfo(
"We will now train Clickitongue on your humming. These hums should be simple,\n"
"relatively quiet closed-mouth hums, like saying 'hm' in reaction to something\n"
"just a tiny bit interesting.\n\n"
"The training will record several 5-second snippets, during each of which you\n"
"will be asked to do a specific number of hums.\n\n");
#ifndef CLICKITONGUE_WINDOWS
    PRINTF("Press any key to continue to the training prompt.\n\n");
    make_getchar_like_getch(); getchar(); resetTermios();
#endif
    for (int i = 0; i < 6; i++)
      hum_examples->emplace_back(recordExampleHum(i), i);
    hum_examples->emplace_back(recordExampleHum(1, /*prolonged=*/true), 1);
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
                  TaggedExamples* hum_examples, bool mic_near_mouth)
{
  collectAnyMissingExamples(blow_examples, cat_examples, hum_examples);

  // all examples of one are negative examples for the other...
  // ...except cat aren't shown to blow, because they look too much like blow,
  // and we're already handling the confusion by having cat inhibit blow.
  TaggedExamples blow_examples_plus_neg = combinePositiveAndNegative(
      blow_examples, {hum_examples});
  TaggedExamples cat_examples_plus_neg = combinePositiveAndNegative(
      cat_examples, {blow_examples, hum_examples});
  TaggedExamples hum_examples_plus_neg = combinePositiveAndNegative(
      hum_examples, {blow_examples, cat_examples});

  assert(hum_examples && !hum_examples->empty());
  double scale = pickHumScalingFactor(*hum_examples);
  PRINTF("using scale %g\n", scale);

  Config config;
  if (!blow_examples_plus_neg.empty())
    config.blow = trainBlow(blow_examples_plus_neg, scale, mic_near_mouth);
  if (!cat_examples_plus_neg.empty())
    config.cat = trainCat(cat_examples_plus_neg, scale, mic_near_mouth);
  if (!hum_examples_plus_neg.empty())
    config.hum = trainHum(hum_examples_plus_neg, scale, mic_near_mouth);

  std::string failure_list;
  if (blow_examples && !config.blow.enabled)
  {
    failure_list += "blowing, ";
    blow_examples->clear();
  }
  if (cat_examples && !config.cat.enabled)
  {
    failure_list += "cat-attention-getting, ";
    cat_examples->clear();
  }
  if (hum_examples && !config.hum.enabled)
  {
    failure_list += "humming, ";
    hum_examples->clear();
  }
  if (!failure_list.empty())
    failure_list.erase(failure_list.end() - 2, failure_list.end());
  int success_count = (config.blow.enabled? 1:0) + (config.cat.enabled? 1:0) +
                      (config.hum.enabled? 1:0);
  int sounds_attempted = (blow_examples? 1:0) + (cat_examples? 1:0) +
                         (hum_examples? 1:0);
  bool blow_and_cat_good = config.blow.enabled && config.cat.enabled;
  if (success_count < sounds_attempted && !blow_and_cat_good)
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
      trainingBody(blow_examples, cat_examples, hum_examples, mic_near_mouth);
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
  bool mic_near_mouth = introAndAskIfMicNearMouth();
  if (mic_near_mouth)
    trainingBody(&blow_examples, &cat_examples, &hum_examples, mic_near_mouth);
  else
    trainingBody(nullptr, &cat_examples, &hum_examples, mic_near_mouth);
}

#define ASSIGN_LEFT_OR_RIGHT_CLICK(x) if (config->x.enabled && remaining_to_assign > 0) \
{ \
  config->x.action_on = left_done ? Action::RightDown : Action::LeftDown; \
  config->x.action_off = left_done ? Action::RightUp : Action::LeftUp; \
  left_done = true; \
  remaining_to_assign--; \
}

// returns true if we can proceed to normalOperation().
bool afterTraining(Config* config, int success_count)
{
  if (success_count == 0)
  {
    promptInfo(
"Clickitongue was not able to find parameters to distinguish any type of mouth\n"
"sound from background noise with acceptable accuracy.\n\n"
"First, make absolutely sure your computer is actually recording from your mic:\n"
"try recording in the Audacity audio editor (which uses the same audio library\n"
"as Clickitongue) and seeing if playback sounds as you would expect. If the mic\n"
"is right, then try moving the mic more directly in front of your mouth, or\n"
"reducing noise in your environment.");
    return false;
  }
  bool left_done = false;
  int remaining_to_assign = 2;
  // (not just alphabetical, this is the preference i think makes sense)
  ASSIGN_LEFT_OR_RIGHT_CLICK(blow);
  ASSIGN_LEFT_OR_RIGHT_CLICK(cat);
  ASSIGN_LEFT_OR_RIGHT_CLICK(hum);
  assert(success_count <= 3);

  std::string attempted_filepath;
  if (!writeConfig(*config, kDefaultConfig, &attempted_filepath))
  {
    std::string msg = "Failed to write config file " + attempted_filepath +
    ". You'll have to redo this training the next time you run Clickitongue.";
    promptInfo(msg.c_str());
  }
  return true;
}

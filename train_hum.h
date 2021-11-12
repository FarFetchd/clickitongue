#ifndef CLICKITONGUE_TRAIN_HUM_H_
#define CLICKITONGUE_TRAIN_HUM_H_

#include "audio_recording.h"
#include "config_io.h"

AudioRecording recordExampleHum(int desired_events);

// the int of the pair is the number of hum events that are supposed to be in
// that particular AudioRecording.
HumConfig trainHum(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   Action hum_on_action, Action hum_off_action, bool verbose = false);

#endif // CLICKITONGUE_TRAIN_HUM_H_

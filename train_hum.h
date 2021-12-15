#ifndef CLICKITONGUE_TRAIN_HUM_H_
#define CLICKITONGUE_TRAIN_HUM_H_

#include "audio_recording.h"
#include "config_io.h"

// Picks a scaling factor that brings the AudioRecordings most closely in line
// with canonical expected values, so that the train param limits will make sense.
//
// the int of the pair is the number of hum events that are supposed to be in
// that particular AudioRecording.
double pickHumScalingFactor(std::vector<std::pair<AudioRecording, int>>
                            const& audio_examples);

AudioRecording recordExampleHum(int desired_events, bool prolonged = false);

// the int of the pair is the number of hum events that are supposed to be in
// that particular AudioRecording.
HumConfig trainHum(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   Action hum_on_action, Action hum_off_action, double scale,
                   bool verbose = false);

#endif // CLICKITONGUE_TRAIN_HUM_H_

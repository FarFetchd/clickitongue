#ifndef CLICKITONGUE_TRAIN_BLOW_H_
#define CLICKITONGUE_TRAIN_BLOW_H_

#include "audio_recording.h"
#include "config_io.h"

// Picks a scaling factor that brings the AudioRecordings most closely in line
// with canonical expected values, so that the train param limits will make sense.
//
// the int of the pair is the number of blow events that are supposed to be in
// that particular AudioRecording.
double pickBlowScalingFactor(std::vector<std::pair<AudioRecording, int>>
                             const& audio_examples);

AudioRecording recordExampleBlow(int desired_events, bool prolonged = false);

// the int of the pair is the number of blow events that are supposed to be in
// that particular AudioRecording.
BlowConfig trainBlow(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                     double scale, bool verbose = false);

#endif // CLICKITONGUE_TRAIN_BLOW_H_

#ifndef CLICKITONGUE_TRAIN_CAT_H_
#define CLICKITONGUE_TRAIN_CAT_H_

#include "audio_recording.h"
#include "config_io.h"

// Picks a scaling factor that brings the AudioRecordings most closely in line
// with canonical expected values, so that the train param limits will make sense.
//
// the int of the pair is the number of cat events that are supposed to be in
// that particular AudioRecording.
double pickCatScalingFactor(std::vector<std::pair<AudioRecording, int>>
                            const& audio_examples);

AudioRecording recordExampleCat(int desired_events);

// the int of the pair is the number of blow events that are supposed to be in
// that particular AudioRecording.
CatConfig trainCat(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   double scale, bool mic_near_mouth);

#endif // CLICKITONGUE_TRAIN_CAT_H_

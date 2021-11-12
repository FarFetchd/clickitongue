#ifndef CLICKITONGUE_TRAIN_PINK_H_
#define CLICKITONGUE_TRAIN_PINK_H_

#include "audio_recording.h"
#include "config_io.h"

AudioRecording recordExamplePink(int desired_events);

// the int of the pair is the number of blow events that are supposed to be in
// that particular AudioRecording.
PinkConfig trainPink(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                     bool verbose = false);

#endif // CLICKITONGUE_TRAIN_PINK_H_

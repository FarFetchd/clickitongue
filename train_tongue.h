#ifndef CLICKITONGUE_TRAIN_TONGUE_H_
#define CLICKITONGUE_TRAIN_TONGUE_H_

#include "audio_recording.h"
#include "config_io.h"

AudioRecording recordExampleTongue(int desired_events);

// the int of the pair is the number of tongue click events that are supposed to
// be in that particular AudioRecording.
TongueConfig trainTongue(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                         Action action, bool verbose = false);

#endif // CLICKITONGUE_TRAIN_TONGUE_H_

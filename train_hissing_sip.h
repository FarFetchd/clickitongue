#ifndef CLICKITONGUE_TRAIN_SIP_H_
#define CLICKITONGUE_TRAIN_SIP_H_

#include "audio_recording.h"
#include "config_io.h"

AudioRecording recordExampleSip(int desired_events, bool prolonged = false);

// the int of the pair is the number of sip events that are supposed to be in
// that particular AudioRecording.
SipConfig trainSip(std::vector<std::pair<AudioRecording, int>> const& audio_examples,
                   double scale);

#endif // CLICKITONGUE_TRAIN_SIP_H_

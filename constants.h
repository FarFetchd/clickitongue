#ifndef CLICKITONGUE_CONSTANTS_H_
#define CLICKITONGUE_CONSTANTS_H_

// Important terminology: each single float value is a sample. A chunk of
// kNumChannels samples (usually 2) is a frame. If you see a pointer to a buffer
// of "samples" together with that buffer's length called num_frames, then that
// buffer holds kNumChannels*num_frames floats (i.e. samples).
//
// Unfortunately, the standard term "sample rate" (usually 44100Hz) is a
// misnomer; it should really be called frame rate, because there are 44100
// *frames* per second recorded, or 88200 samples.
// So we'll call it "frames per second".
constexpr int kFramesPerSec = 44100; // aka "sample rate"
constexpr int kNumChannels = 2;
constexpr float kSilentSample = 0.0f;

using Sample = float;

#endif // CLICKITONGUE_CONSTANTS_H_

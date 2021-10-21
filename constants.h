#ifndef CLICKITONGUE_CONSTANTS_H_
#define CLICKITONGUE_CONSTANTS_H_

// Important terminology: each single float value is a sample. A chunk of
// kNumChannels samples (usually 2) is a frame. If you see a pointer to a buffer
// of "samples" together with that buffer's length called num_frames, then that
// buffer holds kNumChannels*num_frames floats (i.e. samples).
//
// Unfortunately, the standard terms "sample" / "sample rate" (usually 44100Hz
// for audio) in signal processing are not concerned with multiple channels.
// A "sample" in that context is a "frame" in the audio context. We'll stick
// with the audio terms, and call sample rate "frames per second" (we are
// recording 88200 samples per second).
constexpr int kFramesPerSec = 44100; // aka "sample rate".

// The end frequency of the n/2 + 1 Fourier transform output frequency bins.
constexpr double kNyquist = 22050.0f;
constexpr int kNumChannels = 2;
constexpr float kSilentSample = 0.0f;

constexpr int kFourierBlocksize = 256; // must be 128, 256, 512, or 1024

using Sample = float;

#endif // CLICKITONGUE_CONSTANTS_H_

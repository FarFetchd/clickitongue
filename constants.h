#ifndef CLICKITONGUE_CONSTANTS_H_
#define CLICKITONGUE_CONSTANTS_H_

#ifdef CLICKITONGUE_OSX
// ==========================================================================
// ***** CHANGE ME!!! *****
// If you are an OSX user and want a different speed double-click.
//
// This is how long you have (in milliseconds) to issue the second half of a
// double click. Lower values will require you to do your double-clicks faster.
// If you set it too high, you might find unwanted double-clicks happening when
// you meant to do two separate single clicks.
const uint64_t kOSXDoubleClickMs = 333;
// ==========================================================================
#endif

// current version: 1.1.0
#define CLICKITONGUE_VERSION "v1.1.0"

#include <string>

// Important terminology: each single float value is a sample. A chunk of
// g_num_channels samples (usually 2) is a frame. If you see a pointer to a buffer
// of "samples" together with that buffer's length called num_frames, then that
// buffer holds g_num_channels*num_frames floats (i.e. samples).
//
// Unfortunately, the standard terms "sample" / "sample rate" (usually 44100Hz
// for audio) in signal processing are not concerned with multiple channels.
// A "sample" in that context is a "frame" in the audio context. We'll stick
// with the audio terms, and call sample rate "frames per second" (we are
// recording 88200 samples per second).
constexpr int kFramesPerSec = 44100; // aka "sample rate".

// The end frequency of the n/2 + 1 Fourier transform output frequency bins.
constexpr double kNyquist = 22050.0f;

constexpr float kSilentSample = 0.0f;

// Batch size of frames (i.e. pairs of samples if g_num_channels is 2) to feed
// into each Fourier transform. This is sort of the "master granularity" of
// all of our DSP logic; we do decision-making exactly every [this many frames].
constexpr int kFourierBlocksize = 256; // must be 128, 256, 512, or 1024

constexpr int kNumFourierBins = kFourierBlocksize/2 + 1;

// In Hz, the difference between two adjacent bin centers.
constexpr double kBinWidth = kNyquist / (kFourierBlocksize / 2.0 + 1.0);

using Sample = float;

enum class Action
{
  LeftDown, LeftUp, RightDown, RightUp, ScrollDown, ScrollUp,
  RecordCurFrame, // for training only; requires having called setCurFrameDest() and setCurFrameSource().
  NoAction
};

constexpr char kDefaultConfig[] = "default";

extern int g_num_channels;

#endif // CLICKITONGUE_CONSTANTS_H_

Clickitongue carries out mouse clicks when triggered by mouth sounds picked
up by your computer's microphone, to give sore wrists a rest.

Clickitongue runs on Linux (both X and Wayland), Windows, and OSX.

# Installing on Linux

To build clickitongue, you'll need to be able to link the shared libraries
[-lportaudio](http://www.portaudio.com/) and [-lfftw3](https://www.fftw.org/).
On Ubuntu, you can simply `sudo apt install portaudio19-dev libfftw3-dev`.

Once all that is taken care of, run `./build.sh` to compile clickitongue.

I've tested that
`sudo apt install portaudio19-dev libfftw3-dev && ./build.sh`
gets you a working clickitongue even on a totally fresh live-boot of Ubuntu 20.04.

On Linux, clickitongue must be run as root: `sudo ./clickitongue`.

# Installing on Windows

Download the Windows release .zip, unzip it, and run clickitongue.exe.

# Installing on OSX

`brew install portaudio fftw`, and then in the clickitongue directory
run `./build.sh osx.ccbuildfile`. After that, `./clickitongue` to run.

If you want a stricter/looser double-click timing, change the value of
kOSXDoubleClickMs at the top of constants.h before compiling. (Defaults to 1/3rd
of a second).

# Usage

The first time you run Clickitongue, it will have you train it to detect your
particular blowing/cat-attention-getting/humming sounds, in your particular
acoustic environment. This should take about two to five minutes total. If the
training does not leave Clickitongue confident in its ability to detect your
sounds, it will give you a chance to redo part or all of the training.

After that first run, whenever you start Clickitongue it will remember the
configuration it learned the first time, and begin clicking for you immediately.

If you ever want to redo the training procedure, or select a different audio
input device: On Linux or OSX run Clickitongue with the --retrain or
--forget_input_dev flag. On Windows, use the buttons in the GUI.

If none of the sound types work, or if just blowing doesn't work despite having
the mic setup described in the next section, Clickitongue might not have
selected the right audio input device. (Or your OS might be doing something
weird; e.g. my Kubuntu needed "audio profile" changed to "Analog Stereo Duplex"
in order to hear the mic of a headset). A good sanity check is to see what sort
of audio [Audacity](https://github.com/audacity/audacity) is able to record from
you, since Audacity uses the same audio abstraction library (PortAudio) as
Clickitongue.

# Mic Advice

Depending on your mic setup, Clickitongue will use two of three sounds: softly
blowing on the mic, making sounds like trying to get a cat's attention (this can
be 'tchk' sounds, kisses, or compressed 'ts' sounds), or humming.

Based on users' experiences so far, soft blowing is effortlessly smooth even for
hours of use at a time, cat-attention-getting works extremely well but is
slightly cumbersome for long-term frequent use, and humming is quite annoying
indeed for long-term frequent use. Therefore, Clickitongue assigns blowing to
the left-click and cat-attention-getting sounds to the much rarer right-click
when possible; cat to left and humming to right otherwise.

So, you ideally want to use blowing. The soft blows can only be picked up if the
mic is directly in front of your mouth, and very close - about 2cm.
In that position, your soft blows will sound like a hurricane to the mic, easily
noticed by Clickitongue.

As you might imagine, headset mic designers will specifically try to prevent
moderate exhales from sounding like a hurricane. Spongy/fuzzy windscreens are
one technique, which you can of course just remove.

Another technique, much worse for Clickitongue, is to put the mic on a stiff arm
limited to rotating in a fixed arc, far enough to the side of the mouth that
moderate exhalations won't be registered. You can only do Clickitongue's blows
by "aiming" your mouth at such a mic, which gets unacceptably uncomfortable
within like a minute of use.

So, get a headset with a long flexible arm, and a removable windscreen. For an
example, I've been using the
[Nubwo N7](https://www.amazon.com/NUBWO-headsets-Headset-Headphones-Canceling/dp/B07KXMMXKP)
with Clickitongue, and it works great. It's really comfy, too! (This is just a
personal recommendation, not a paid ad, and that is not an affiliate link.)

Clickitongue does still work even without its preferred mic-near-mouth setup, if
you're willing to settle for cat-attention-getting and humming (which at the
very least can be a "free trial" to see if it's worth buying a headset for).
In fact, if you aren't trying to use blowing, Clickitongue actually doesn't
require a headset at all: for instance, a random decade old webcam plugged into
a desktop, and the built-in mics of an X1 Carbon ThinkPad and a recent Macbook,
all work. In general, any audio setup that allows someone on the other end of a
video call to hear what you're saying ought to work for cat-attention-getting
and humming.

# Compiling on Windows

(You can ignore this if you're not interested in working on Clickitongue's
source code!) Install MSYS2, and from an MSYS2 terminal run
`./build.sh windows.ccbuildfile` in the main clickitongue dir.

# Acknowledgments

Clickitongue uses the PortAudio audio abstraction library (http://portaudio.com/)
and the Fastest Fourier Transform in the West (https://www.fftw.org/) Fast
Fourier Transform library. FFTW is quite impressively polished; it radiates an
aura of "this thing is basically perfect". PortAudio is kind of amazing in how
easy it makes it to write portable audio code. I would have guessed every
additional platform would be a grueling effort, but nope, completely effortless.
(Other than getting it to compile on Windows...) Speaking of compiling on
Windows, Clickitongue uses MSYS2 (https://www.msys2.org/), which was nicely
transparent to my UNIX-y code while offering every Windows feature I cared to
use. Finally, C++ structopt (https://github.com/p-ranav/structopt), a great
little header-only library that gives you command line arguments in the
cleanest, simplest way I can imagine.

Many thanks to everyone who worked on these projects! They are all a pleasure to
use.

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

If you want to click any windows belonging to run-as-admin programs, you'll
have to run clickitongue.exe as admin. (But that should be rare).

# Installing on OSX

`brew install portaudio fftw`, and then in the clickitongue directory
run `./build.sh osx.ccbuildfile`. After that, `./clickitongue` to run.

If you want a stricter/looser double-click timing, change the value of
kOSXDoubleClickMs at the top of constants.h before compiling. (Defaults to 1/3rd
of a second).

# Usage

The first time you run Clickitongue, it will have you train it to detect your
particular blowing and humming sounds, in your particular acoustic environment.

If you have a microphone that you can position about 1cm in front of your mouth,
you'll be able to use both blowing and humming, allowing both left and right
clicks. If you can't keep the mic that close, humming will still work.
Don't worry about mic quality - even the built-in mic of an X1 Carbon ThinkPad
works for humming, and any mic positioned (very) near your mouth should work for
blowing. Remove any spongy/fuzzy windscreens for best blowing results!

If you ever want to redo the training procedure, or change the selected audio
input device: On Linux or OSX run Clickitongue with the --retrain or
--forget_input_dev flag. On Windows, use the buttons in the main GUI.

For long-term use, you'll probably settle into using the gentlest blows
possible, to avoid fatigue. However, when attempting fast double-clicks,
these gentle blows tend to smear into each other, making it hard for
Clickitongue to differentiate them. So, when double-clicking, try making a
more well-defined plosive sound, like "puh-puh".

# Mic Advice

Humming works, but for really smooth non-annoying long-term use you want your
left-clicks to be controlled by blowing. Clickitongue wants yours blows to be
directly hitting the mic, so that playing back the recording would sound like a
massive hurricane.

As you might imagine, good headset mic designers will specifically try to
prevent their mic from recording this particular kind of audio. Spongy/fuzzy
windscreens are one technique, which you can of course just remove.

Another technique, much worse for Clickitongue, is to put the mic on a stiff arm
limited to rotating in a fixed arc, far enough from the mouth so that moderate
exhalations (like what Clickitongue wants) won't be registered. This type of
headset is generally unusable for long-term comfortable Clickitongue blowing.
Get one with a long flexible arm, instead. For an example, I've been using the
Nubwo N7 with Clickitongue, and it works quite nicely. (Comfy, too!)

# Manual Tuning

The following is highly user-unfriendly, and should ideally never be needed
for smooth usage of Clickitongue. However, for determined users who find the
automatic training almost but not quite good enough, here is how to manually
tune Clickitongue. First exit Clickitongue, then edit the default.clickitongue
config file (find it alongside clickitongue.exe for Windows, in ~/.config/
for Linux and OSX).

For blowing, there are "on" and "off" thresholds for each of three octaves.
When tweaking a threshold, do so for all three octaves, proportionately - e.g.
increase all three by 5% each.

* If Clickitongue is making you blow too hard to start a click, reduce the "on"
  thresholds, and/or increase the EWMA alpha.
* If Clickitongue is leaving the mouse clicked too long after you stop blowing,
  increase the "off" thresholds, and/or increase the EWMA alpha.
* If Clickitongue is issuing spurious clicks, increase the "on" thresholds,
  and/or decrease the EWMA alpha.
* If Clickitongue is spuriously releasing long clicks (flakey drag+drop /
  selection), decrease the "off" thresholds, and/or decrease the EWMA alpha.

For humming, there are on and off thresholds like blowing, and also two
"limits". These limits suppress the off->on transition when either is exceeded,
but do not affect the on->off transition. Increasing them may make it easier to
trigger hum detection. However, their purpose is to prevent even extremely light
mouth exhalations from being confused with humming, so increasing them too much
is likely to cause problems.

# Compiling on Windows

(You can ignore this if you're not interested in working on Clickitongue's
source code!) Install MSYS2, and from an MSYS2 terminal run
`./build.sh windows.ccbuildfile` in the main clickitongue dir.

# Language Support

Currently only English is available. If you are fluent in another language and
would like to make Clickitongue more widely available, translations will be
very gratefully accepted! (The code currently just uses a bunch of hard-coded
English strings, so don't dive right in. Instead, open an issue to let me know
you'd like to contribute a translation, and I'll add some sort of framework to
support multiple languages.)

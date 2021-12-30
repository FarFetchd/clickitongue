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

If you have a microphone that you can position about 1cm in front of your mouth,
you'll be able to use both blowing and humming, allowing both left and right
clicks. If you can't keep the mic that close, humming will still work.
Don't worry about mic quality - even the built-in mic of an X1 Carbon ThinkPad
works for humming, and any mic positioned (very) near your mouth should work for
blowing. Remove any spongy/fuzzy windscreens for best blowing results!

The first time you run Clickitongue, it will have you train it to detect your
particular blowing and humming sounds, in your particular acoustic environment.

If you ever want to redo the training procedure: On Linux or OSX run
Clickitongue with the --retrain flag. On Windows delete the
default.clickitongue file that should live in the same directory as
clickitongue.exe.

If you want to change audio input devices: On Linux or OSX run Clickitongue with
the --forget_input_dev flag. On Windows delete the audio_input_device.config
file that should live in the same directory as clickitongue.exe.

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
config file described earlier.

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

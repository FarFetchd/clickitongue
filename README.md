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

# Usage

If you have a microphone that you can position about 1cm in front of your mouth,
you'll be able to use both blowing and humming, allowing both left and right
clicks. If you can't keep the mic that close, humming will still work.
Don't worry about mic quality - even the built-in mic of an X1 Carbon ThinkPad
works for humming, and any mic positioned near your mouth should work for
blowing. Remove any fuzzy/spongy windscreens for best blowing results!

The first time you run Clickitongue, it will have you train it to detect your
particular blowing and humming sounds, in your particular acoustic environment.

If you ever want to redo the training procedure: On Linux or OSX run
Clickitongue with the --retrain flag. On Windows delete the
default.clickitongue file that should live in the same directory as
clickitongue.exe.

If you want to change audio input devices: On Linux or OSX run Clickitongue with
the --forget_input_dev flag. On Windows delete the audio_input_device.config
file that should live in the same directory as clickitongue.exe.

# Compiling on Windows

(You can ignore this if you're not interested in working on Clickitongue's
source code!) Install MSYS2, and from an MSYS2 terminal run
`./build.sh windows.ccbuildfile` in the main clickitongue dir.

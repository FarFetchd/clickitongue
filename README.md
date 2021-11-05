Clickitongue carries out mouse clicks when triggered by mouth sounds picked
up by your computer's microphone, to give sore wrists a rest.

Clickitongue is still in development, but is already basically usable. It
currently runs on Linux (both X and Wayland) and Windows only. Support for OSX
is planned.

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
have to run clickitongue.exe as admin.

# Usage

If you have a microphone that you can position like 2cm in front of your mouth,
you'll be able to use blowing in addition to tongue clicks, allowing both left
and right clicks. If you can't keep the mic nearby, tongue clicks will still
work. Don't worry about mic quality - even the built-in mic of an X1 Carbon
ThinkPad works for tongue clicks, and any mic positioned near your mouth should
work for blowing. Remove any fuzzy/spongy windscreens for best blowing results!

The first time you run Clickitongue, it will have you train it to detect your
particular tongue click and blowing sounds, in your particular acoustic
environment. For blow training, short soft bursts as if you were going to say
"pu" are best. The tongue clicks it expects are like 0:31-0:34 of
[this video.](https://youtu.be/L7sWPZArUN0?t=31)

After that first time training, you should be able to simply run Clickitongue
and immediately start clicking.

# Compiling on Windows

(You can ignore this if you're not interested in working on Clickitongue's
source code!) Install MSYS2, and from an MSYS2 terminal run
`./build.sh windows.ccbuildfile` in the main clickitongue dir.

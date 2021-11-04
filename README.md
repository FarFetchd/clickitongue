Clickitongue carries out mouse clicks when triggered by mouth sounds picked
up by your computer's microphone, to give sore wrists a rest.

Clickitongue is still in development, but is already basically usable. It
currently runs on (non-Wayland) Linux and Windows only. Support for Wayland and
OSX is planned.

# COMPILATION / NEEDED LIBRARIES

To build clickitongue, you'll need to be able to link the following shared
libraries: [-lportaudio](http://www.portaudio.com/),
[-lxdo](https://github.com/jordansissel/xdotool), and
[-lfftw3](https://www.fftw.org/). On Ubuntu,
`sudo apt install libxdo-dev libfftw3-dev` should take care of the second two.
For PortAudio,

```
$ git clone https://github.com/PortAudio/portaudio
$ cd portaudio
$ ./configure --disable-static
$ make
$ make install
```

Once all that is taken care of, run `./build.sh` to compile clickitongue.

(That's for Linux. For Windows, install MSYS2, and from an MSYS2 terminal run
`./build.sh windows.ccbuildfile` in the main clickitongue dir.
The resulting .exe needs to have all the .dlls from the windows_libs/ dir
living in the same dir with it - but can be run normally; doesn't need
MSYS2 installed).

# USAGE

If you have a microphone that you can position like 2cm in front of your mouth,
you'll be able to use blowing in addition to tongue clicks, allowing both left
and right clicks. Remove any fuzzy/spongy windscreens for best blowing results!
If you can't keep the mic nearby, tongue clicks will still work. Don't worry
about mic quality - even the built-in mic of an X1 Carbon ThinkPad works for
tongue clicks, and any mic positioned near your mouth should work for blowing.

The first time you run Clickitongue, it will have you train it to detect your
particular tongue click and blowing sounds, in your particular acoustic
environment. For blow training, short soft bursts as if you were going to say
"pu" are best. The tongue clicks it expects are like 0:31-0:34 of
[this video.](https://youtu.be/L7sWPZArUN0?t=31)

After that first time training, you should be able to simply run Clickitongue
and immediately start clicking.

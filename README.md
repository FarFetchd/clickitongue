Clickitongue carries out mouse clicks when triggered by mouth sounds picked
up by your computer's microphone, to give sore wrists a rest.

Clickitongue is still very much in early development, but is already basically
usable. It currently runs on (non-Wayland) Linux only. Support for Wayland,
Windows, and OSX is planned.

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

# USAGE

First, decide whether you'll be using tongue clicks, or blowing. If you have a
microphone that you can position like half an inch in front of your mouth,
blowing will be better (remove any fuzzy/spongy windscreens for best results).
If not, tongue clicks should work. Don't worry about mic quality - even the
built-in mic of an X1 Carbon ThinkPad works for tongue clicks, and any mic
positioned near your mouth should work for blowing.

Train Clickitongue on some samples of your personal setup: run either
`./clickitongue --mode=train --detector=tongue` or
`./clickitongue --mode=train --detector=blow` and follow the instructions. For
blow training, short bursts as if you were going to say "pu" are best.
Once trained, those same bursts will work for clicks. You can also do a
prolonged blow to hold the mouse button down. (Tongue clicks do not have a
hold-down mode).

The training will go through an optimization algorithm, printing out command
line parameter configurations at each step. Once the optimization has converged,
copy the last batch of command line parameters it printed, and use them in
`./clickitongue --mode=use --detector=tongue REPLACE_ME_WITH_THE_PARAMS` or
`./clickitongue --mode=use --detector=blow REPLACE_ME_WITH_THE_PARAMS`. This
command will run forever, doing mouse clicks when it detects the right sound.

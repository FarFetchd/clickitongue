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

# TODOs

* make this into an actual readme, move the TODOs elsewhere
* for fancy blowing, have dedicated click, and LMB down. click is "a block passes threshold T1 in one window and does NOT pass T2 in the next." LMB down is T1 followed by T2. dedicated click should have a refrac block or two. at the same time, listen for something else ('hot' breathing? tongue clicking? both?) to do right clicks.
* if we want tongue clicks to be right click alongside a puff/blow left click, then we'll need to add logic to tongue click
to expect a LACK of energy outside the expected range.
* get blow trainer to handle both on and off events for long blows. migrate it to the nifty new logic tongue uses. (not sure which is easier to do first.... probably migrate first).


# SIM NOTES


working pretty well for laptop builtin mic:
./clickitongue --mode=use --detector=tongue --tongue_low_hz=750 --tongue_high_hz=950 --tongue_hzenergy_high=1500 --tongue_hzenergy_low=500 --refrac_blocks=8 --fourier_blocksize_frames=256

best of this batch have 0 violations
--tongue_low_hz=1000 --tongue_high_hz=1500 --tongue_hzenergy_high=4100 --tongue_hzenergy_low=1100 --refrac_blocks=12 --blocksize=256
--tongue_low_hz=1000 --tongue_high_hz=1500 --tongue_hzenergy_high=4100 --tongue_hzenergy_low=2100 --refrac_blocks=12 --blocksize=256

from the nifty new random optimization:
./clickitongue --mode=test --detector=tongue --tongue_low_hz=957.202 --tongue_high_hz=2168.13 --tongue_hzenergy_high=6980.68 --tongue_hzenergy_low=1193.79 --refrac_blocks=12 --blocksize=256
wow!
./clickitongue --mode=test --detector=tongue --tongue_low_hz=998.848 --tongue_high_hz=1786.5 --tongue_hzenergy_high=4985.03 --tongue_hzenergy_low=1232.3 --refrac_blocks=12 --blocksize=256


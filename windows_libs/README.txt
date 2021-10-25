PROVENANCE
===================================

libportaudio.a was built as follows:

1) follow setup instructions on https://www.msys2.org/
   (NOTE: windows.ccbuildfile assumes you install to the default path /c/msys64/ !!!)
2) git clone https://github.com/PortAudio/portaudio
3) in portaudio, `./configure --host=i686-mingw64msvc`
4) `make`. All of the tests+examples will "fail" to compile... but you'll find them,
   working, in bin/.libs. More importantly, lib/.libs will contain libportaudio.a.
   That's the one I included here in the repo.



The FFTW dll is the 64-bit dll provided at http://www.fftw.org/install/windows.html.

The msys dlls are copied from the msys64/usr/bin of an ordinary MSYS2 installation.


USAGE
===================================
libportaudio.a gets statically linked in, as you would expect.

The dlls live with the source code so that MSYS2's spooky special version of g++ can
be pointed at them. It is apparently able to see inside them and figure out how to
generate the appropriate "use this dll stuff" mechanisms within the .exe. (This used
to need a .lib or .dll.a file, but I guess things have gotten smarter, phew).

In addition to the compilation needing to see those dlls, the end user trying to
run clickitongue.exe will need all of those dlls in the same directory as the .exe.

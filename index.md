## Clickitongue

Clickitongue is a tool that removes the need to physically click mouse buttons, to help people with repetitive strain injuries. **Its goal is for an experienced mouse user to feel no more encumbered than they would clicking normally with their fingers.** Clickitongue learns to recognize specific sounds, and issues clicks (left and right) in response. It can tolerate noisy environments - people talking, air conditioners running nearby - without issuing spurious clicks.

Clickitongue is available to everyone free of charge under the GPLv3. It runs on Linux (both X and Wayland), Windows, and OSX.

## Goals and Motivation

Clickitongue is unlike other assistive technologies, in that it specifically targets repetitive strain injury rather than full disability. Tools for people without any use of their fingers have to replace the entirety of computer input techniques. Such replacements are necessarily slower and more annoying to use than mouse+keyboard, since nothing compares to the dexterity of fingers. This degraded experience makes them all but unusable for all but the worst RSI: an expert mouse+keyboard user using a tool that restricts them to half capacity, when their full capacity is right at their fingertips, will eventually give in and switch back.

Clickitongue keeps users out of that trap by staying simple and smooth. Mouse clicking, a major contributor to RSI, is a very simple action: depressing/releasing one of two buttons. By controlling just these simple actions with the most comfortable input method available - blowing whisper-quiet puffs of air at a mic - Clickitongue makes it possible for people who *could* but really *shouldn't* be clicking to choose to give their wrists a rest, and stick to that choice. Besides the ease of long-term use, getting started is easy; there is nothing for a new user to learn.

## Usage

On the first run, Clickitongue will have you select your audio input device, and then guide you through providing samples to its detector optimization process. This process takes about two to five minutes total. Afterwards, and on all future runs of the program, Clickitongue will proceed directly to letting you click.

Just about any microphone, including webcam or built-in laptop mics, should get you basic functionality. However, for really smooth operation the mic needs to be positioned near your mouth. [More details here.](https://github.com/FarFetchd/clickitongue#mic-advice)

## Installation

On Windows, [download the latest Windows release](https://github.com/FarFetchd/clickitongue/releases/latest), unzip it, and run the .exe.

On Linux or OSX, `git clone https://github.com/FarFetchd/clickitongue`, and follow [the installation instructions for Linux](https://github.com/FarFetchd/clickitongue#installing-on-linux) or [for OSX](https://github.com/FarFetchd/clickitongue#installing-on-osx).

## Contact

You [can open an issue on Github](https://github.com/FarFetchd/clickitongue/issues/new), send a message to (this tool's name) at gmail, or contact [https://twitter.com/clickitongue](https://twitter.com/clickitongue).

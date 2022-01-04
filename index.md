## Clickitongue

Clickitongue is a tool to help repetitive-strain injured wrists, by removing the need to physically click mouse buttons. It learns to recognize the sound of blowing on and/or humming into a mic, and issues left and right clicks in response. Clickitongue can tolerate noisy environments - people talking, air conditioners running nearby - without issuing spurious clicks.

Clickitongue runs on Linux (both X and Wayland), Windows, and OSX.

## Installation

On Windows, [download the latest Windows release](https://github.com/FarFetchd/clickitongue/releases/latest), unzip it, and run the .exe.

On Linux or OSX, `git clone https://github.com/FarFetchd/clickitongue`, and follow [the installation instructions for Linux](https://github.com/FarFetchd/clickitongue#installing-on-linux) or [for OSX](https://github.com/FarFetchd/clickitongue#installing-on-osx).

## Usage

On the first run, Clickitongue will have you select your audio input device, and then guide you through its training process. The training is quick; it should take about two minutes total. If the training does not leave Clickitongue confident in its ability to detect your sounds, it will give you a chance to redo part or all of the training. Once it is confident, it will proceed to normal operation. Clickitongue saves the results of the training, so all subsequent runs of Clickitongue will immediately let you start clicking, with no further configuration.

Just about any microphone should get you basic functionality, but for really smooth operation the mic needs to be positioned very near your mouth. [More details here.](https://github.com/FarFetchd/clickitongue#mic-advice)

For more detailed instructions and usage tips, [read Clickitongue's README](https://github.com/FarFetchd/clickitongue#usage).

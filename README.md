# Pi Meter

## ALSA Plugin to display a VU meter on various Raspberry Pi add-ons

### Installing

Currently, you will need to compile the pimeter lib from source, as follows:

#### pre-requisite

```
sudo apt-get install build-essential autoconf automake libtool libasound2-dev libfftw3-dev
```

#### clone repository

if you haven't done so already, git clone this repository and cd into it:

```
git clone https://github.com/pimoroni/pimeter
cd pimeter
```

#### compiling from source


from the top of the repository, run the following to generate the MakeFile:

```
aclocal && libtoolize
autoconf && automake --add-missing
```

then, to compile and install:

```
./configure && make
sudo make install
```

### Options

Pi Meter supports various options in your `/etc/asound.conf`, these include:

#### output_device

Specify which device to display the VU meter on.

Supported devices:

* default (18-segment VU driven by SN3218)
* blinkt
* speaker-phat

#### brightness

Specify the pixel brightness from 0 to 255

#### Example

```
pcm_scope.pimeter {
        type pimeter
        decay_ms 500
        peak_ms 400
        brightness 128
        output_device blinkt
}
```

a full example of a suitable `asound.conf` file can be found in the `dependencies` directory.

# Pi Meter

## ALSA Plugin to display a VU meter on various Raspberry Pi add-ons

### Options

Pi Meter supports various options in your `/etc/asound.conf`, these include:

#### output_device

Specify which device to display the VU meter on.

Supported devices:

* blinkt - Simple amplitude meter through Green->Yellow->Red
* speaker-phat - Simple amplitude meter
* scroll-phat - displays 11-band FFT-based EQ

#### brightness

Specify the pixel brightness from 0 to 255

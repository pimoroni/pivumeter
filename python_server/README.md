# Pi VU Meter: Python Front-end

The `pivumeter` Python library creates a socket-based server to which the Pi VU Meter ALSA plugin can
connect and stream left/right VU amplitude and FFT transform frequency bins.

Currently to run this code you will need to clone the devel branch of Pi VU Meter, install it, and
tweak your `/etc/asound.conf` to change the `output-device` to `socket`.

Finally, run the desired example from the Python Server examples folder and play some audio!

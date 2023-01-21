# Pi VU Meter: Python Front-end

The `pivumeter` Python library creates a socket-based server to which the Pi VU Meter ALSA plugin can
connect and stream left/right VU amplitude and FFT transform frequency bins.

## Installing

Currently to run this code you will need to clone the `devel` branch of Pi VU Meter and compile it from source.

You may use our automated setup to do so, specifying the output device to `socket`
```
setup.sh socket
```
You can also compile and set up thing yourself - just follow [this guide](../README.md#installing) but change the `output-device` to `socket`.

After that you need to install python pivumeter library.
Just go to the `pivumeter/python_server/library` folder and run
```
sudo python setup.py install
```

Finally, run the desired example from the Python Server examples folder and play some audio!

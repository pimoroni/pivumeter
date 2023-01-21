#!/usr/bin/env python
"""
A basic implementaton of a Pi VU Meter python server, for use with the "socket" mode.

To create a server for a specific HAT or pHAT you must derive from the pivumeter.OutputDevice class.

This class includes three methods you should override:

    setup(self): Perform any initial setup for your output device here
    display_vu(self, left, right): Receives the left/right VU meter data unscaled
    cleanup(self): Perform any cleanup for your output device here

The left and right channel values passed into `display_vu` are unscaled. It is your responsibility to
scale them to a sensible range for display on your HAT or pHAT.
"""
import pivumeter
import signal
import phatbeat

BRIGHTNESS = 255

class OutputBlinkt(pivumeter.OutputDevice):
    def setup(self):
        self.base_colours = [(0,0,0) for x in range(phatbeat.CHANNEL_PIXELS)]
        self.generate_base_colours(BRIGHTNESS)

    def generate_base_colours(self, brightness = 255.0):
        for x in range(phatbeat.CHANNEL_PIXELS):
            self.base_colours[x] = (float(brightness)/phatbeat.CHANNEL_PIXELS-1) * x, float(brightness) - ((255/phatbeat.CHANNEL_PIXELS-1) * x), 0

    def display_fft(self, bins):
        pass

    def display_vu(self, left, right):
        left /= 2000.0
        right /= 2000.0

        left = max(min(left, 8), 0)
        right = max(min(right, 8), 0)

        self.set_level(0, left)
        self.set_level(1, right)

        phatbeat.show()

    def set_level(self, channel, level):
        for x in range(phatbeat.CHANNEL_PIXELS):
            val = max(min(level, 1), 0)

            r, g, b = [int(c * val) for c in self.base_colours[x]]

            phatbeat.set_pixel(x, r, g, b, channel=channel)

            level -= 1

    def cleanup(self):
        self.display_vu(0, 0)

pivumeter.run(OutputBlinkt)
signal.pause()

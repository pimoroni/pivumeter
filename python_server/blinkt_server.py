#!/usr/bin/env python
"""
A basic implementaton of a Pi VU Meter python server, for use with the "socket" mode.

To create a server for a specific HAT or pHAT you must derive from the socket_server.OutputDevice class.

This class includes three methods you should override:

    setup(self): Perform any initial setup for your output device here
    display_vu(self, left, right): Receives the left/right VU meter data unscaled
    cleanup(self): Perform any cleanup for your output device here

The left and right channel values passed into `display_vu` are unscaled. It is your responsibility to
scale them to a sensible range for display on your HAT or pHAT.
"""
import socket_server
import signal
import blinkt
import speakerphat

BRIGHTNESS = 255

class OutputBlinkt(socket_server.OutputDevice):
    def setup(self):
        self.base_colours = [(0,0,0) for x in range(blinkt.NUM_PIXELS)]
        self.generate_base_colours(BRIGHTNESS)

    def generate_base_colours(self, brightness = 255.0):
        for x in range(blinkt.NUM_PIXELS):
            self.base_colours[x] = (float(brightness)/blinkt.NUM_PIXELS-1) * x, float(brightness) - ((255/blinkt.NUM_PIXELS-1) * x), 0

    def display_fft(self, bins):
        pass

    def display_vu(self, left, right):
        left /= 2000.0
        right /= 2000.0

        level = max(left, right)
        level = max(min(level, 8), 0)

        for x in range(blinkt.NUM_PIXELS):
            val = 0

            if level > 1:
                val = 1
            elif level > 0:
                val = level

            r, g, b = [int(c * val) for c in self.base_colours[x]]

            blinkt.set_pixel(x, r, g, b)
            speakerphat.set_led(x, int(val * 255.0))
            level -= 1 

        blinkt.show()
        speakerphat.show()

    def cleanup(self):
        self.display_vu(0, 0)

socket_server.run(OutputBlinkt)
signal.pause()

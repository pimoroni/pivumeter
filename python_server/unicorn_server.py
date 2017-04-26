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
import unicornhat


class OutputScrollPhatHD(socket_server.OutputDevice):
    def setup(self):
        self.brightness = 128.0
        self.base_colours = [(0,0,0) for x in range(8)]
        for x in range(8):
            r = int((self.brightness/8-1) * x)
            g = (self.brightness - r)
            b = 0
            self.base_colours[x] = r, g, b

    def display_fft(self, bins):
        for x in range(8):
            bar = (bins[x] / 65535.0) * 8
            for y in range(8):
                val = 0

                if bar > 1:
                    val = 1
                elif bar > 0:
                    val = bar

                r, g, b = [int(c * val) for c in self.base_colours[y]]

                unicornhat.set_pixel(x, y, r, g, b)
                
                bar -= 1

        unicornhat.show() 

    def display_vu(self, left, right):
        pass

    def cleanup(self):
        pass

socket_server.run(OutputScrollPhatHD)
signal.pause()

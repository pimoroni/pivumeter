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
import scrollphathd


class OutputScrollPhatHD(socket_server.OutputDevice):
    def setup(self):
        pass

    def display_fft(self, bins):
        scrollphathd.set_graph(bins, low=0, high=65535, brightness=0.5, x=0, y=0)
        scrollphathd.show() 

    def display_vu(self, left, right):
        pass

    def cleanup(self):
        pass

socket_server.run(OutputScrollPhatHD)
signal.pause()

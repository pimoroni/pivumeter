#!/usr/bin/env python

import pivumeter
import signal

BRIGHTNESS = 255

try:
    import sn3218
except ImportError:
    exit("This library requires the sn3218 module\nInstall with: sudo pip install sn3218")

class OutputSpeakerpHAT(pivumeter.OutputDevice):
    NUM_PIXELS = 10

    def set_led(self, index, value):
        self.led_values[self.stupid_led_mappings[index]] = value

    def show(self):
        sn3218.output(self.led_values)

    def setup(self):
        self.stupid_led_mappings = [0, 1, 2, 4, 6, 8, 10, 12, 14, 16]

        self.led_values = [0 for x in range(18)]
        
        enable_leds = 0

        for x in self.stupid_led_mappings:
            enable_leds |= 1 << x

        sn3218.enable_leds(enable_leds)
        sn3218.enable()

    def display_fft(self, bins):
        pass

    def display_vu(self, left, right):
        left /= 2000.0
        right /= 2000.0

        level = max(left, right)
        level = max(min(level, 8), 0)
        
        for x in range(self.NUM_PIXELS):
            val = max(min(level, 1), 0)

            self.set_led(x, int(val * BRIGHTNESS))

            level -= 1

        self.show()

    def cleanup(self):
        self.display_vu(0, 0)

pivumeter.run(OutputSpeakerpHAT)
signal.pause()

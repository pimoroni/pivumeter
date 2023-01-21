#!/usr/bin/env python
import pivumeter
from signal import pause
from gpiozero import LEDBoard
from time import sleep
from random import shuffle

MAX = 65535.0

class Xmas(pivumeter.OutputDevice):
    def setup(self):
        self.busy = False
        self.tree = LEDBoard(*(2, 19,16,25,27,11, 17,4,8,5,26,13,9, 18,20,24,22,15,6,12, 23,10,14,7,21), pwm = True)
        self.leds = [0] * 5
        self.blink()

    def normalize(self, number):
        return max(float(format(number/MAX, '.1f')), 0.05)

    def display(self):
        self.busy = True
        values = [self.leds[0]] + [self.leds[1]] * 5 + [self.leds[2]] * 7 + [self.leds[3]] * 7 + [self.leds[4]] * 5
        self.tree.value = values
        sleep(0.025)
        self.busy = False

    def display_fft(self, bins):
        if self.busy: return
        bins = bins[:(len(self.leds) - 1)];
        self.leds = [self.leds[0]] + map(lambda x: self.normalize(x), bins);

    def display_vu(self, left, right):
        if self.busy: return
        vu = left + right
        self.leds[0] = self.normalize(vu)
        if vu == 0:
            if self.blinking != True:
                self.blink()
        else:
            self.blinking = False
            self.display()

    def cleanup(self):
        for led in reversed(self.tree.leds):
            led.off()
            sleep(0.1)

    def blink(self):
        self.blinking = True
        leds = list(self.tree.leds[1:])
        shuffle(leds)
        self.tree[0].on()
        for led in leds:
            led.pulse()
            sleep(0.05)

pivumeter.run(Xmas)
pause()

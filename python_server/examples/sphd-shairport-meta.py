#!/usr/bin/env python

import pivumeter
import signal
import scrollphathd
import threading
import time

import os

from base64 import decodestring
from xml.etree import ElementTree

try:
    import queue
except ImportError:
    import Queue as queue

class OutputScrollPhatHD(pivumeter.OutputDevice):
    def __init__(self):
        super(OutputScrollPhatHD, self).__init__()

        self.running = False
        self.busy = False
        scrollphathd.set_brightness(0.2)
        self.messages = queue.Queue()

        self._thread = threading.Thread(target=self.run_messages)
        self._thread.daemon = True
        self._thread.start()

    def run_messages(self):
        self.running = True
        while self.running:
            try:
                message = self.messages.get(False)
                self.busy = True
                scrollphathd.clear()
                length = scrollphathd.write_string(message)
                scrollphathd.set_pixel(length + 17, 0, 0)
                scrollphathd.show()
                time.sleep(1)
                for x in range(length):
                    if not self.running: break
                    scrollphathd.scroll(1)
                    scrollphathd.show()
                    time.sleep(0.05)
             
                scrollphathd.clear()
                self.messages.task_done()
                self.busy = False
            except queue.Empty:
                pass
            time.sleep(1)

    def setup(self):
        pass

    def display_fft(self, bins):
        if self.busy: return
        self.busy = True
        scrollphathd.set_graph(bins, low=0, high=65535, brightness=1.0, x=0, y=0)
        scrollphathd.show() 
        self.busy = False

    def display_vu(self, left, right):
        pass

    def cleanup(self):
        self.running = False
        self._thread.join()

print("""
Press Ctrl+C then hit Enter to exit.
""")

output_device = OutputScrollPhatHD()

pivumeter.run(output_device)

try:
    last_song = None

    while pivumeter.running:
        f = os.open('/tmp/shairport-sync-metadata', os.O_RDONLY|os.O_NONBLOCK)

        try:
            buf = ''
            while True:
                c = os.read(f, 1)
                buf += c
                if buf.endswith("</item>"):
                    break

            data = ElementTree.fromstring(buf)
            #print(buf)
            ptype = data.find('type').text
            pcode = data.find('code').text
            payload = data.find('data')
            if payload is not None and payload.get('encoding') == 'base64':
                payload = decodestring(payload.text) 

            # Song title
            if ptype == '636f7265' and pcode == '6d696e6d':
                if payload.strip() != last_song:
                    output_device.messages.put(payload.strip())
                    last_song = payload

        except os.error as e:
            if e.errno == 11:
                os.close(f)
                continue

            raise e

        os.close(f)
        time.sleep(0.001)

except KeyboardInterrupt:
    pass



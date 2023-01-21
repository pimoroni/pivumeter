#!/usr/bin/env python
import websocket
import json
import pivumeter
import signal
import scrollphathd
import threading
import time

import os

from base64 import decodestring
from xml.etree import ElementTree


MOPIDY_HOST = "192.168.0.155:6680"


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

                    if not self.messages.empty():
                        scrollphathd.clear()
                        scrollphathd.show()
                        break
             
                scrollphathd.clear()
                self.messages.task_done()
                self.busy = False
            except queue.Empty:
                pass
            time.sleep(0.1)

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

def on_message(ws, message):
    message = json.loads(message)
    #print(message)
    artist_name = None
    track_name = None

    if 'tl_track' in message and message['event'] in ['track_playback_started', 'track_playback_resumed']:
        track = message['tl_track']['track']

        if 'artists' in track:
            artist = track['artists'][0]
            artist_name = artist['name']

        if 'name' in track:
            track_name = track['name']

        if artist_name is not None and track_name is not None:
            output_device.messages.put("{}: {}".format(artist_name, track_name))

    if message['event'] in ['track_playback_paused']:
        output_device.messages.put("Paused...")


def on_error(ws, error):
    print(error)

def on_close(ws):
    print("### closed ###")

def on_open(ws):
    pass

if __name__ == "__main__":
    websocket.enableTrace(True)

    ws = websocket.WebSocketApp("ws://{}/mopidy/ws".format(MOPIDY_HOST),
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)

    ws.on_open = on_open

    ws.run_forever()


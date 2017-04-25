try:
    import socketserver
except ImportError:
    import SocketServer as socketserver

import struct
import os
import time
import threading
import signal

import phatbeat

SOCKET_FILE = "/tmp/pivumeter.socket"

base_colours = [(0,0,0) for x in range(8)]

try:
    os.remove(SOCKET_FILE)
except OSError:
    pass

BRIGHTNESS = 255.0

def generate_base_colours(brightness = 255.0):
    for x in range(phatbeat.CHANNEL_PIXELS):
        base_colours[x] = (float(brightness)/phatbeat.CHANNEL_PIXELS-1) * x, float(brightness) - ((255/phatbeat.CHANNEL_PIXELS-1) * x), 0

def update_vu():
    running = True
    while running:
        display_vu(left, right)

def display_vu(left, right):
    left /= 2000.0
    right /= 2000.0

    left = max(min(left, 8),0)
    right = max(min(right, 8),0)

    for x in range(phatbeat.CHANNEL_PIXELS):
        val = 0

        if left > 1:
            val = 1
        elif left > 0:
            val = left

        r, g, b = [int(c * val) for c in base_colours[x]]

        phatbeat.set_pixel(x, r, g, b, channel=1)
        left -= 1 

        val = 0

        if right > 1:
            val = 1
        elif right > 0:
            val = right

        r, g, b = [int(c * val) for c in base_colours[x]]

        phatbeat.set_pixel(7-x, r, g, b, channel=0)
        right -= 1

    phatbeat.show()


class VUHandler(socketserver.BaseRequestHandler):
    def get_value(self):
        data = self.request.recv(4)

        if len(data) < 4:
            raise ValueError("Insufficient data")

        return struct.unpack("<L", data)[0]

    def handle(self):
        while server.running:
            try:
                left = self.get_value()
                right = self.get_value()
            except ValueError:
                break

            display_vu(left, right)

        print("Finished...")
        display_vu(0, 0)
        return

server = socketserver.ThreadingUnixStreamServer(SOCKET_FILE, VUHandler)

if __name__ == "__main__":
    #t_phatbeat = threading.Thread(target=update_vu)
    #t_phatbeat.daemon = True
    #t_phatbeat.start()

    generate_base_colours()

    def shutdown(signum, frame):
        print("Shutting down...")
        server.running = False
        server.shutdown()

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    thread_server = threading.Thread(target=server.serve_forever)
    thread_server.start()
    server.running = True
    while server.running:
        pass
    thread_server.join()
    #while True:
    #    for x in range(1000):
    #        display_vu(x * 16, x * 16)
    #        time.sleep(0.001)
    #    for x in reversed(range(1000)):
    #        display_vu(x * 16, x * 16)
    #        time.sleep(0.001)

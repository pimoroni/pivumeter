try:
    import socketserver
except ImportError:
    import SocketServer as socketserver

import struct
import os
import time
import threading
import signal
import sys
from datetime import datetime

try:
    import queue
except ImportError:
    import Queue as queue

import blinkt

SOCKET_FILE = "/tmp/pivumeter.socket"


try:
    os.remove(SOCKET_FILE)
except OSError:
    pass

BRIGHTNESS = 255.0

LOG_LEVEL = 0

LOG_INFO = 0
LOG_WARN = 1
LOG_FAIL = 2
LOG_DEBUG = 3

def log(msg, level=LOG_INFO):
    if level < LOG_LEVEL: return

    sys.stdout.write(str(datetime.now()))
    sys.stdout.write(": ")
    sys.stdout.write(msg)
    sys.stdout.write("\n")
    sys.stdout.flush()

class OutputDevice(threading.Thread):
    def __init__(self):
         self.fifo = queue.Queue()
         super(OutputDevice, self).__init__()
         self.stop_event = threading.Event()
         self.daemon = True

    def display_vu(self, left, right):
        pass

    def cleanup(self):
        pass

    def queue(self, item):
        self.fifo.put(item)

    def run(self):
        while not self.stop_event.is_set():
            try:
                left, right = self.fifo.get(False)
                self.display_vu(left, right)
                self.fifo.task_done()
            except queue.Empty:
                pass

    def start(self):
        if not self.isAlive():
            self.stop_event.clear()
            super(OutputDevice, self).start()

    def stop(self):
        if self.isAlive():
            self.stop_event.set()
            self.cleanup()
            self.join()

class OutputBlinkt(OutputDevice):
    def __init__(self):
        self.base_colours = [(0,0,0) for x in range(blinkt.NUM_PIXELS)]
        self.generate_base_colours(BRIGHTNESS)

        super(OutputBlinkt, self).__init__()

    def generate_base_colours(self, brightness = 255.0):
        for x in range(blinkt.NUM_PIXELS):
            self.base_colours[x] = (float(brightness)/blinkt.NUM_PIXELS-1) * x, float(brightness) - ((255/blinkt.NUM_PIXELS-1) * x), 0

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
            level -= 1 

        blinkt.show()

    def cleanup(self):
        self.display_vu(0, 0)


class VUHandler(socketserver.BaseRequestHandler):
    def get_long(self):
        data = self.request.recv(4)

        if len(data) < 4:
            raise ValueError("Insufficient data")

        return struct.unpack("<L", data)[0]

    def get_data(self):
        data = self.request.recv(80)
  
        if len(data) < 80:
            print("Ugh insufficient data, length: {}".format(len(data)))
            raise ValueError("Insufficient data")

        return struct.unpack("<LL", data[:8])
        #return self.get_long(), self.get_long()

    def setup(self):
        log("Client connected")

    def finish(self):
        log("Client disconnected")

        # Output a final 0, 0 to clear the device 
        self.server.output_device.queue((0, 0))

    def handle(self):
        self.request.settimeout(1)

        while self.server.running:
            try:
                left, right = self.get_data()
            except ValueError: # If insufficient data is received, assume the client has gone away
                break

            self.server.output_device.queue((left, right))

class VUServer(socketserver.ThreadingUnixStreamServer):
    def __init__(self, address, handler, output_device):
        self.running = False
        self.output_device = output_device()
        socketserver.ThreadingUnixStreamServer.__init__(self, address, handler)

    def serve_forever(self):
        log("Serve called...")
        self.running = True
        self.output_device.start()
        socketserver.ThreadingUnixStreamServer.serve_forever(self)

    def shutdown(self):
        self.running = False
        log("Stopping Output Device")
        self.output_device.stop()
        log("Stopping Server")
        socketserver.ThreadingUnixStreamServer.shutdown(self)
        log("Shutdown Complete")


if __name__ == "__main__":
    server = VUServer(SOCKET_FILE, VUHandler, OutputBlinkt)
    thread_server = threading.Thread(target=server.serve_forever)

    def shutdown(signum, frame):
        log("Interrupt Caught: Shutting Down")
        server.shutdown()
        thread_server.join()

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    log("Starting Server")
    thread_server.start()
    signal.pause()


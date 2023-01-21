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


SOCKET_FILE = "/tmp/pivumeter.socket"

running = False

try:
    os.remove(SOCKET_FILE)
except OSError:
    pass

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
         self.setup()

    def setup(self):
        pass

    def display_vu(self, left, right):
        pass

    def display_fft(self, bins):
        pass

    def cleanup(self):
        pass

    def queue(self, left, right, fft_bins=None):
        self.fifo.put((left, right, fft_bins))

    def run(self):
        while not self.stop_event.is_set():
            left, right, fft_bins = None, None, None

            while True:
                try:
                    left, right, fft_bins = self.fifo.get(False)
                    self.fifo.task_done()
                except queue.Empty:
                    break

            if left is not None:
                self.display_vu(left, right)
                if fft_bins is not None: self.display_fft(fft_bins)

    def start(self):
        if not self.isAlive():
            self.stop_event.clear()
            super(OutputDevice, self).start()

    def stop(self):
        if self.isAlive():
            self.stop_event.set()
            self.cleanup()
            self.join()

class VUHandler(socketserver.BaseRequestHandler):
    def get_data(self):
        data = self.request.recv(12)
  
        if len(data) < 12:
            raise ValueError("Insufficient header data")

        # Unpack the first 2 longs into left/right amplitude
        left, right = struct.unpack("<LL", data[:8])

        # Unpack the final long into our FFT bin size
        bin_size = struct.unpack("<L", data[8:])[0]

        fft_bins = []

        if bin_size > 0:
            data = self.request.recv(bin_size * 4)

            if len(data) < bin_size * 4:
                raise ValueError("Insufficient FFT bin data")

            # Unpack <bin_size> longs into our FFT bins
            fft_bins = list(struct.unpack("<" + "L" * bin_size, data))

        return left, right, fft_bins

    def setup(self):
        log("Client connected")

    def finish(self):
        log("Client disconnected")

        # Output a final 0, 0 to clear the device 
        self.server.output_device.queue(0, 0)

    def handle(self):
        self.request.settimeout(10)
        #self.request.setblocking(False)

        while self.server.running:
            try:
                left, right, fft_bins = self.get_data()
            except ValueError: # If insufficient data is received, assume the client has gone away
                break
            except socketserver.socket.timeout:
                break
            except socketserver.socket.error as e:
                if e.errno == 11:
                    continue

                break

            self.server.output_device.queue(left, right, fft_bins)

class VUServer(socketserver.ThreadingUnixStreamServer):
    def __init__(self, address, handler, output_device):
        self.running = False
        if isinstance(output_device, OutputDevice):
            self.output_device = output_device
        else:
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

def run(output_device):
    global running

    server = VUServer(SOCKET_FILE, VUHandler, output_device)
    os.chmod(SOCKET_FILE,0o777)
    thread_server = threading.Thread(target=server.serve_forever)

    def shutdown(signum, frame):
        global running

        log("Interrupt Caught: Shutting Down")
        server.shutdown()
        thread_server.join()

        running = False

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    log("Starting Server")
    thread_server.start()
    running = True

if __name__ == "__main__":
    run(OutputBlinkt)
    signal.pause()


# -*- coding: utf-8 -*-

import threading
import time
import sys
import os
import Adafruit_GPIO.SPI as SPI
import Adafruit_SSD1306
import codecs
import zmq
import json
import signal

import Image
import ImageDraw
import ImageFont

# Constants
ip_address = "127.0.0.1"
port_number = 10010
RST = 24  # Raspberry Pi pin configuration:

class DisplayThread(threading.Thread):
    '''
    Control OLED display using Adafruit library.
    JSON command format:
        - Display: {"command": "display", "option":{"message":[line1, line2, line3, ... ]}}
        - Clear:   {"command": "clear"}
        - Stop:    {"command": "stop"}
    '''

    def __init__(self):
        '''
        Initializes the Adafruit SSD1306 library, the image object, and the font object.
        '''
        super(DisplayThread, self).__init__()
        # Initialize SSD1306 library
        global RST
        self._disp = Adafruit_SSD1306.SSD1306_128_64(rst=RST)
        try:
            self._disp.begin()
        except:
            print("Error: I2C communication failed.")
            exit(-1)
        self._disp.clear()
        self._disp.display()

        # Create blank image for drawing
        # Make sure to create image with mode '1' for 1-bit color
        self.width = self._disp.width
        self.height = self._disp.height
        self._image = Image.new('1', (self.width, self.height))

        # Get drawing object to draw on image
        self._draw = ImageDraw.Draw(self._image)

        self.font = ImageFont.load_default()
        # wget http://www.geocities.jp/littlimi/arc/misaki/misaki_ttf_2015-04-10.zip
        #font = ImageFont.truetype('/home/pi/work/install/misakifont/misaki_gothic.ttf', font_size, encoding='unic')
        #font = ImageFont.truetype('/usr/share/fonts/truetype/roboto/Roboto-Light.ttf', font_size, encoding='unic')
        #font = ImageFont.truetype('fonts/8-bit_pusab.ttf', font_size, encoding='unic')
        #font = ImageFont.truetype('fonts/PrStart.ttf', font_size, encoding='unic')
        #font = ImageFont.truetype('fonts/PixelCharas.ttf', font_size, encoding='unic')

        # Calculate font width, height
        self.font_width, self.font_height = self.font.getsize("0")

        # Adjust font height if necessary (comment in)
        # font_height+=1

        # Message to be displayed (array of line string)
        self._lines = []

        # Condition object to notify update request
        self.update_request_condition = threading.Condition()

        # Flag to stop the main loop
        self._stopped = False

    def clear(self):
        ''' Clears the display. '''
        self._draw.rectangle(
            (0, 0, self.width, self.height), outline=0, fill=0)
        self._disp.clear()
        self._disp.display()

    def _update_display(self):
        '''
        Updates the display content by sending new image data via the I2c
        interface. This method takes some time (~100ms) to complete, and
        therefore, is called by worker thread to minimize the latency to
        send the reply to the client.
        '''
        # Clear internal buffer
        self._draw.rectangle(
            (0, 0, self.width, self.height), outline=0, fill=0)
        # Draw each line
        n_width = 32
        line_index = 0
        line_index_max = 8
        for line in self._lines:
            x = 0
            y = line_index * self.font_height
            line = line[0:n_width]
            self._draw.text((x, y), line.strip(), font=self.font, fill=128)
            line_index += 1
            if(line_index == line_index_max):
                break
        self._disp.image(self._image)
        self._disp.display()
        self._message_updated = False

    def display(self, lines):
        '''
        Displays the provided message.
        '''
        self._lines = lines
        self._message_updated = True
        with self.update_request_condition:
            self.update_request_condition.notify()

    def stop(self):
        '''
        Stops the thread.
        '''
        self._stopped = True

    def run(self):
        print("Starting the DisplayServer thread...")
        print("Press Ctrl-C to quit.")
        while not self._stopped:
            with self.update_request_condition:
                self.update_request_condition.wait(0.5)  # sec
            if self._message_updated:
                self._update_display()

# Instantiate DisplayServer
display_thread = DisplayThread()
display_thread.start()

# Main loop
def run_server():
    context = zmq.Context()
    responder = context.socket(zmq.REP)
    responder.bind("tcp://{:s}:{:d}".format(ip_address, port_number))

    display_thread.display(["Waiting for a client", "at {}".format(ip_address)])

    while True:
        # Wait until a message from a client is received
        request = responder.recv()
        print "Recieved request: [%s]" % request
        # Convert to a JSON object
        json_command = json.loads(request)
        json_reply = {}
        # Process command if the received JSON contains "command" entry
        if "command" in json_command:
            if json_command["command"] == "display":
                if "option" in json_command and "message" in json_command["option"]:
                    message = json_command["option"]["message"]
                    if isinstance(message, list):
                        display_thread.display(message)
                    else:
                        display_thread.display(str(message).split("\n"))
                    json_reply = {"status": "ok",
                                 "message": "Message displayed"}
                else:
                    message_string = "'message' option not found for 'display' command"
                    json_reply = {"status": "error", "message": message_string}
            elif json_command["command"] == "clear":
                display_thread.clear()
                json_reply = {"status": "ok", "message": "Display cleaered"}
            elif json_command["command"] == "stop":
                display_thread.stop()
                json_reply = {"status": "ok",
                             "message": "Display server thread stopped"}
            elif json_command["command"] == "ping":
                json_reply = {"status": "ok",
                             "message": "responding to a ping"}
            else:
                message_string = "invalid command '{}'".format(json_command["command"])
                json_reply = {"status": "error", "message": message_string}
        else:
            print("Warning: received message does not contain 'command'")
            print("---------------------------------------------")
            print(json_command)
            print("---------------------------------------------")
            json_reply = {"status": "error", "message": "invalid command"}
        responder.send(json.dumps(json_reply))

    # Finalize
    display_thread.stop()

# Define signal handler to stop DisplayServer thread
# Note: If this is not implemented, the main thread will stop when
#       Ctrl-C is received, but the DisplayServer thread continue
#       to run as zombie and cannot be stopped (requiring explicit
#       kill command from the shell).
def signal_handler(signal, frame):
    print('Ctrl+C is received! Finishing the program.')
    display_thread.stop()
    sys.exit(0)

# Register the signal handler
signal.signal(signal.SIGINT, signal_handler)

# Start server by entering the main loop
run_server()

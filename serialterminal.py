#!/usr/bin/env python

'''
howto send test string:
import serial
ser = serial.Serial('/dev/cu.wchusbserial1410', 19200, timeout=.1)
ser.write("Hello".encode())
'''

import serial
import argparse

parser = argparse.ArgumentParser(description='simpler serial port listener', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-p', '--port', type=str, default='', required=True, help='give port to listen from, eg "/dev/cu.wchusbserial1410"')
parser.add_argument('-b', '--baudrate', type=int, default=19200, help='give port speed in baud, eg 19200')

args = parser.parse_args()

print("Listening on {port}, {baud}".format(port=args.port, baud=args.baudrate))

client = serial.Serial(args.port, args.baudrate, timeout=.1)
try:
    while True:
        data = client.read()
        # data = client.readline()[:-2]
        if data:
            print(data.decode('utf-8'), end='')
except KeyboardInterrupt:
    pass

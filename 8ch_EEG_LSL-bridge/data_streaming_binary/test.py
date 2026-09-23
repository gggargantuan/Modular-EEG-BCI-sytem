import serial
import struct

ser = serial.Serial("COM9",500000)

while True:

    if ser.read(1) != b'\xAA':
        continue

    if ser.read(1) != b'\x55':
        continue

    payload = ser.read(36)

    counter = struct.unpack('<I', payload[:4])[0]

    channels = struct.unpack('<8i', payload[4:])

    print(counter, channels)
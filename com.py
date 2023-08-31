import time
import logging
import json
import serial
import os

import serial.tools.list_ports


from ctypes import *
import time
import threading


from datetime import datetime


ser = serial.Serial("/dev/ttyACM1", 115200)

ser.write(b'R')
ret = ser.readline().decode('ascii')
print(ret)

ser.write(b'T')
ret = ser.readline().decode('ascii').strip()
print(ret)
ret = ser.readline().decode('ascii').strip()
print(ret)

if (ret != "ff00 ff00 ff00 ff00"):
    print("MCPs not working\n")
    exit(1)

file_path = 'input.txt'
numbers_list = []

try:
    with open(file_path, 'r') as file:
        for line in file:
            number = int(line.strip())  # Convert the line to a float
            numbers_list.append(number)
except FileNotFoundError:
    print("File not found")
except Exception as e:
    print(f"An error occurred: {e}")


ser.write(b'I')

for number in numbers_list:
        number_str = str(number)  # Convert number to string and add newline
        ser.write(number_str.encode() + " ".encode())   # Write the number as bytes to the serial port
ser.write('\r\n'.encode())
ser.flush()  # Flush the output buffer

ret = ser.readline().decode('ascii')
print(ret)
ret = ser.readline().decode('ascii')
print(ret)

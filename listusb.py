#!/usr/bin/python3

from pathlib import Path
import random
import math
import sys
import copy
import os
#import python-matplotlib
#import python-matplotlib.pyplot as plt
import numpy as np
import argparse
import serial
import serial.tools.list_ports
from datetime import datetime
import time

#todo - try ACQ and reset on failure

def list_usb_serial_devices(device_type):
    available_ports = serial.tools.list_ports.comports()
    
    usb_serial_devices = []
    
    for port in available_ports:
 #       print(port.description)
        if device_type in port.description:
            usb_serial_devices.append((port.device, port.serial_number,port.location))
    
    return usb_serial_devices




if __name__ == '__main__':

 
# Get the current date and time
    current_datetime = datetime.now()

# Format the date and time as a string in a specific format
    formatted_datetime = current_datetime.strftime("%Y-%m-%d-%H-%M")
    


    # Get a list of all available serial ports
    
    device_type = "Pico - Board CDC"  # Modify this to match the device type you're looking for
    
    usb_serial_devices = list_usb_serial_devices(device_type)
    sers = []
    if usb_serial_devices:
        print("USB Serial Devices:")
        for device, serial_number,location in usb_serial_devices:
            print(f"Device: {device}, Serial Number: {serial_number}, Location: {location}")
            ser = serial.Serial(device,115200)
            sers.append(ser)
    else:
        print("No USB Serial Devices found.")
        
 

#!/usr/bin/python3

from pathlib import Path
import random
import math
import sys
import copy
import os
import matplotlib.pyplot as plt
import numpy as np
import argparse
import serial
import serial.tools.list_ports
from datetime import datetime
import time

def list_usb_serial_devices(device_type):
    available_ports = serial.tools.list_ports.comports()
    
    usb_serial_devices = []
    
    for port in available_ports:
        print(port.description)
        if device_type in port.description:
            usb_serial_devices.append((port.device, port.serial_number))
    
    return usb_serial_devices

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="Command line options")

    parser.add_argument(
        "-d", "--dir", help="Folder containing the dicam files", required=False)
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Enable verbose output")
    parser.add_argument("-s", "--scan", action="store_true",
                        help="Enable file generation")

    parser.add_argument(
        "-f", "--file", help="File name containing the translations and rotations", required=False)
    parser.add_argument("-a", "--address",
                        help="Address of device", required=True)

    args = parser.parse_args()

    # Get a list of all available serial ports
    
    device_type = "usb serial"  # Modify this to match the device type you're looking for
    
    usb_serial_devices = list_usb_serial_devices(device_type)
    sers = []
    if usb_serial_devices:
        print("USB Serial Devices:")
        for device, serial_number in usb_serial_devices:
            print(f"Device: {device}, Serial Number: {serial_number}")
            ser = serial.Serial(device,115200)
            sers.append(ser)
    else:
        print("No USB Serial Devices found.")
        
        
    
    
    for ser in sers:
        ser.write(b'D')
    
        ret = ser.readline().decode('ascii')
        print(ret)
    

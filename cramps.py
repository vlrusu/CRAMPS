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
            usb_serial_devices.append((port.device, port.serial_number))
    
    return usb_serial_devices



SCANFILE = "input"
DATAFILE = "crampdata"
LOGFILE = "log"

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="Command line options")

    
    parser.add_argument("-s", "--scan", action="store_true",
                        help="Perform a cramp scan")

    parser.add_argument(
        "-f", "--file", help="File name containing the translations and rotations", required=False)
    parser.add_argument("-a", "--address",
                        help="Address of device", required=True)
    parser.add_argument("-n", "--nsamples",
                        help="Number of required data poits to take", required=False, default = 0)


    parser.add_argument("-o", "--poweroff",action="store_true",
                        help="Power off")

    args = parser.parse_args()

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
        for device, serial_number in usb_serial_devices:
            print(f"Device: {device}, Serial Number: {serial_number}")
            ser = serial.Serial(device,115200)
            sers.append(ser)
    else:
        print("No USB Serial Devices found.")
        
    serport = 0
    deviceid = args.address
    for ser in sers:
        
        ser.write(b'D')
    
        ret = ser.readline().decode('ascii').strip()
        if (ret==deviceid) :
            serport = ser
        print(ret)
    ser=serport
    if serport == 0:
        print("Device ID not found")
        exit()
        
    
    LOGFILE = LOGFILE + "_" + deviceid+"_"+ formatted_datetime+".log"
    log = open(LOGFILE, "w")
    
    #check whether to power off
    if (args.poweroff):
        ser.write(b'O')
        exit()
        
    #just in case    
    ser.write(b'R')
    ser.readline()
        
    #first thig, power up
    
    ser.write(b'P')
    ser.readline()
    
    time.sleep(2)
    #then do a MCP test
    ser.write(b'T') 
    ser.readline().decode('ascii')
    tmp = ser.readline().decode('ascii').strip()
    if (tmp != 'beef beef beef beef'):
        log.write("MCPs not initialized properly\n")
        log.write(tmp)
#        exit()
        
        
    #now initialize
    ser.write(b'I')
    tmp = ser.readline().decode('ascii').strip()
    if (tmp != '0 0 0 0') :
        log.write("MCPs not returning retc correctly\n")
        log.write(tmp)
        exit()
    
    while (tmp != "Initialization complete"):
        tmp=ser.readline().decode('ascii').strip()
    
    #if a scan is required stop at that
    if (args.scan):
        ser.write(b'S')
        tmp=[]
        ser.readline()
        while(1):
            line = ser.readline().decode('ascii').strip()
            log.write(line)
        
            if (line == "Scanning complete"):
                break
        exit()
    
    #now do a scan
    ser.write(b'S')
    tmp=[]
    ser.readline()
    while(1):
        line = ser.readline().decode('ascii').strip()
        
        if (line == "Scanning complete"):
            break
        tmp.append(line)

    SCANFILE = SCANFILE + "_" + deviceid+"_"+ formatted_datetime+".txt"
    with open(SCANFILE, 'w') as file:
        for i in range(len(tmp)):
            file.write(tmp[i] + "\n")
            
    file.close()
    
    
    #now start acquisition for the desired number of samples
    nsample = int(args.nsamples)
    
    DATAFILE = DATAFILE + "_" + deviceid+"_"+formatted_datetime + ".dat"
    file = open(DATAFILE, "w")
    count = 0
    ser.write(b'A')
    line = ser.readline().decode('ascii')
    log.write(line)
    while count<nsample:
        line = ser.readline().decode('ascii')
        file.write(line)
        count = count+1
    file.close()
    ser.write(b'R')
    
    
    log.close()
        

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
import psutil

#todo - try ACQ and reset on failure
#todo - deal with adding new picos while a process is curently running. Right now that is not supported

def check_process_exists(process_name):
    count = 0
    for process in psutil.process_iter(attrs=['name']):
        try:
            if process.info['name'] == process_name:
                count=count+1
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    return count


def list_usb_serial_devices(device_type):
    available_ports = serial.tools.list_ports.comports()
    
    usb_serial_devices = []
    
    for port in available_ports:
 #       print(port.description)
        if device_type in port.description:
            usb_serial_devices.append((port.device, port.serial_number))
    
    return usb_serial_devices

def count_different_bits(word1, word2):
    # Perform a bitwise XOR to find the differing bits
    xor_result = word1 ^ word2
    
    # Initialize a count for differing bits
    count = 0
    
    # Count the set bits in the XOR result
    while xor_result:
        count += xor_result & 1
        xor_result >>= 1
    
    return count

SCANFILE = "input"
DATAFILE = "crampdata"
LOGFILE = "log"
MAPFILE = "usbdevices.map"

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="Command line options")

    
    parser.add_argument("-s", "--scan", action="store_true",
                        help="Perform a cramp scan")

    parser.add_argument(
        "-f", "--file", help="File name containing the translations and rotations", required=False)
    parser.add_argument("-a", "--address",
                        help="Address of device", required=True)
    parser.add_argument("-n", "--nsamples",
                        help="Number of required data points to take", required=False, default = 0)


    parser.add_argument("-o", "--poweroff",action="store_true",
                        help="Power off")

    args = parser.parse_args()

# Get the current date and time
    current_datetime = datetime.now()

# Format the date and time as a string in a specific format
    formatted_datetime = current_datetime.strftime("%Y-%m-%d-%H-%M")
    
#first check if a process with name cramps already is running
    script_name = os.path.basename(__file__)
    print("The name of the current script is:", script_name)
    countprocesses =  check_process_exists(script_name)
    print ("process="+str(countprocesses));

    device_type = "Pico - Board CDC"  # Modify this to match the device type you're looking for
    
    mapdata={}

    if countprocesses == 1:
        #this is the first process, nothing else is running, so ok to remap
        mapdatafile = open(MAPFILE, "w")
        
        # Get a list of all available serial ports
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
            exit()

        for ser in sers:

            ser.write(b'R')
            ser.readline()
            ser.write(b'D')
    
            ret = ser.readline().decode('ascii').strip()
            mapdatafile.write(ret + " "+ ser.port + "\n")

        mapdatafile.close()

            

    mapdatafile = open(MAPFILE, "r")
    lines = mapdatafile.readlines()
    for item in lines:
        idevice, acmnumber  = item.split()
        mapdata[idevice] = acmnumber  

#    print(mapdata)

    deviceid = args.address

    ser = serial.Serial(mapdata[deviceid],115200)

    print(f'deviceid={deviceid} port={mapdata[deviceid]}')
    if ser == 0:
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
    tmp = ser.readline().decode('ascii').strip().split()

    for imcp in range(len(tmp)):
        mcpnumber = int(tmp[imcp], 16)

        diff_bits = count_different_bits(mcpnumber, 0xbeef)
#        print(hex(mcpnumber)+ " "+hex(0xbeef)+ " "+str(diff_bits))
        if (diff_bits > 0):
            if  (diff_bits<6):
                print("MCPs not initialized properly, but the program will continue, since only "+ str(diff_bits) + " bits are different " + str(imcp) + " " + tmp[imcp]+ " " +"\n")
                log.write("MCPs not initialized properly, but the program will continue, since only "+ str(diff_bits) + " bits are different " + str(imcp) + " " + tmp[imcp]+ " " +"\n")

            else:
                print("MCPs not initialized properly and program will stop, since "+ str(diff_bits) + " bits are different " + str(imcp) + " " + tmp[imcp]+ " " +"\n")
                exit()
        
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
            print(line)
        
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
    line = ser.readline().decode('ascii')
    log.write(line)
    while count<nsample:
        line = ser.readline().decode('ascii')
        file.write(line)
        count = count+1
    file.close()
    ser.write(b'R')
    
    
    log.close()
        

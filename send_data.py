#!/usr/bin/python3

import serial
import os
import time
import sys
import psutil
import socket
import subprocess
from serial.tools import list_ports

# ser = serial.Serial("/dev/ttyACM0", baudrate=115200)

def pico_com_port():
    global pico_port
    picoVID = "2E8A" # Pico USB Vendor ID
    picoPID = "000A" # Pico USB product ID
    port_list = serial.tools.list_ports.comports()
    
    for device in port_list:
        if (device.vid != None or device.pid != None):
            if ('{:04X}'.format(device.vid) == picoVID and
                '{:04X}'.format(device.pid) == picoPID):
                pico_port = device.device
                break
            pico_port = None
            print("Raspberry Pi Pico failed to open serial port. Please check VID and PID IDs are correct.")
            
pico_com_port()

ser = serial.Serial(pico_port, 115200)

# Set the process name to check
process_name = "emulationstation"

vid_stopped = False

while True:
    while True:
        # Check if Retropie is running
        for proc in psutil.process_iter(['name']):
            if proc.info['name'] == process_name:
                # Send a command to stop the video
                ser.write("X".encode())
                ser.close()
                vid_stopped = True
                break
        if vid_stopped:
            break
            
        # Wait before checking again
        time.sleep(1)
    
    # Loop until serial connection is established
    while True:
        try:
            ser = serial.Serial(pico_port, 115200)
            break
        except serial.SerialException:
            time.sleep(0.1)
            
    # SD Usage
    while True:
        sd_stat = os.statvfs("/")
        total = sd_stat.f_frsize * sd_stat.f_blocks / (1024 * 1024 * 1024)
        used = (sd_stat.f_frsize * (sd_stat.f_blocks - sd_stat.f_bfree)) / (1024 * 1024 * 1024)
        disk_data = "{:.0f}/{:.0f}GB\n".format(used, total)

    # CPU Temperature
        with open('/sys/class/thermal/thermal_zone0/temp', 'r') as temp_file:
            temp = temp_file.readline()
            temp = float(temp.strip()) / 1000
        cpu_temp = "{:.1f}C\n".format(temp)

    # Clock Speed
        with open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r") as clock_file:
            clock_speed = float(clock_file.read().strip()) / 1000
        clock_speed_data = "{:.0f}MHz\n".format(clock_speed)

    # RAM Usage
        mem = psutil.virtual_memory()
        total_ram = mem.total / (1024 * 1024 * 1024)
        used_ram = mem.used / (1024 * 1024 * 1024)
        ram_data = "{:.1f}/{:.1f}GB\n".format(used_ram, total_ram)

    # IP Address
        host_name = os.popen("hostname -I").read()
        ip_address = host_name

        ser.write(b"A" + disk_data.encode('utf-8'))
        ser.write(b"B" + cpu_temp.encode('utf-8'))
        ser.write(b"C" + clock_speed_data.encode('utf-8'))
        ser.write(b"D" + ram_data.encode('utf-8'))
        ser.write(b"E" + ip_address.encode('utf-8'))

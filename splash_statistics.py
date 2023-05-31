# splash_statistics.py -- Statistics and Liveliness Monitor -- bdt -- 2023-01-01
# Copyright (c) 2023 Benjamin D. Todd.
#
# This routine sends statistics data to the Pico every five seconds.  It also monitors for the power down situation
# as well as startup/shutdown of the game emulator.

import fcntl
import os
import serial
import sys
import time
import psutil
import socket
import subprocess
from serial.tools import list_ports
import RPi.GPIO as GPIO

REPORT_INTERVAL = 5                         # Time between statistics updates in seconds.
SLEEP_INTERVAL = 0.100                      # Time between state checks in seconds.
SHUTDOWN_PIN = 25                           # Shutdown pin.  Set by RetroLiteMonitorScript.  Active High on shutdown. 

process_name = "emulationstation"           # Set the process name to check emulation station

done = False
haveSerialLock = False
lastStatsTime = time.time() - 999           # Pretend we already sent stats a long time ago
needShutdown = 0                            # 0=No, 1=Powering down, 2=Shutdown because of software
needStats = True
processWasRunning = False                   # Used so we can detect that the game emulator was stopped

# Open lock file

lock_file = open('/tmp/lock_file', 'w')

def check_connection(port):
    if port is None:
        return False
    return os.path.exists(port)

def get_pico_port():
    picoVID = "2E8A" # Pico USB Vendor ID
    picoPID = "000A" # Pico USB product ID\
    port_list = serial.tools.list_ports.comports()

    for device in port_list:
        if (device.vid != None or device.pid != None):
            if ('{:04X}'.format(device.vid) == picoVID and
                '{:04X}'.format(device.pid) == picoPID):
                return device.device

    return None

# Free the Serial Lock

def FreeLock():
    global haveSerialLock

    if haveSerialLock:
        fcntl.flock(lock_file, fcntl.LOCK_UN)
        haveSerialLock = False

# Get the I/O lock, non-blocking.
#
# @return Sets and returns haveSerialLock.  The value will be True if
# locking was successful.

def GetLock():
    global haveSerialLock

    haveSerialLock = False

    try:
        fcntl.flock(lock_file, fcntl.LOCK_EX | fcntl.LOCK_NB)  # Acquire lock before opening serial port
        haveSerialLock = True
    except BlockingIOError as error:
        print(error)

    return haveSerialLock;

def shutdownPinCallback(channel):
    global needShutdown
    if GPIO.input(channel):
        needShutdown = 2

def wait_for_connection():
    # Loop until serial connection is established
    connection = None
    while connection is None:
        connection = get_pico_port()
        if connection is None:
            time.sleep(10)
            print("Pico not connected")

# Set up GPIO

GPIO.setmode(GPIO.BCM)
GPIO.setup(SHUTDOWN_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.add_event_detect(SHUTDOWN_PIN, GPIO.RISING, callback=shutdownPinCallback, bouncetime=200)

while not done:
    wait_for_connection()
    
    pico_port = get_pico_port()
    connected = check_connection(pico_port)
    
    if not connected:
        time.sleep(SLEEP_INTERVAL)
    else:
        now = time.time()

        try:
            processIsRunning = False  # Check if Emulation Station is running now

            for proc in psutil.process_iter(['name']):
                if proc.info['name'] == process_name:
                    processIsRunning = True
                    break

            if not processIsRunning:
                if processWasRunning:           # Process went away!
                    if needShutdown == 0:
                        needShutdown = 2
                    processWasRunning = False

            if needShutdown != 0:
                if GetLock():
                    ser = serial.Serial(pico_port, 115200)
                    ser.write(b'SD' + str(needShutdown).encode('utf-8') + b"\n")  # Tell the Pico to shutdown with SD1 or SD2 command.
                    ser.close()

                    if needShutdown == 1:
                        done = True
                    else:                               # Basically, just restart the loop
                        needShutdown = 0
            elif processIsRunning:
                processWasRunning = True                # Now we can determine if the process went away

                if now - lastStatsTime >= REPORT_INTERVAL and GetLock():
                    ser = serial.Serial(pico_port, 115200)
                    ser.write(b"X\n")  # This will stop the splash screen if necessary

                    # SD Usage
                    sd_stat = os.statvfs("/")
                    total = sd_stat.f_frsize * sd_stat.f_blocks / (1024 * 1024 * 1024)
                    used = (sd_stat.f_frsize * (sd_stat.f_blocks - sd_stat.f_bfree)) / (1024 * 1024 * 1024)
                    disk_data = "{:.0f}/{:.0f}GB\n".format(used, total)

                    # CPU Temperature
                    with open('/sys/class/thermal/thermal_zone0/temp', 'r') as temp_file:
                        temp = temp_file.readline()
                        temp = float(temp.strip()) / 1000
                    temp = "{:.1f}C\n".format(temp)

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

                    ser.write(b"A" + disk_data.encode('utf-8') + b"\n")
                    ser.write(b"B" + temp.encode('utf-8') + b"\n")
                    ser.write(b"C" + clock_speed_data.encode('utf-8') + b"\n")
                    ser.write(b"D" + ram_data.encode('utf-8') + b"\n")
                    ser.write(b"E" + ip_address.encode('utf-8') + b"\n")

                    ser.close()
                    lastStatsTime = now
                    
        except Exception as error:
            print(error)
        finally:
            FreeLock()

        time.sleep(SLEEP_INTERVAL)

lock_file.close()
GPIO.cleanup()
sys.exit(needShutdown)  # Exit code can only currently be 1 (shutdown via power button). or 1.  2 is game end, and doesn't terminate the loop.

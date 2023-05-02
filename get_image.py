#!/usr/bin/python3

from lib2to3.pgen2 import pgen
from operator import truediv
from pickle import TRUE
import subprocess
import serial
from serial import tools
from serial.tools import list_ports
import sys
import time
import os
import struct
import linecache
import xml.etree.ElementTree as et
import xml.sax.saxutils as saxutils
import cv2
import psutil
import fcntl
import random
from PIL import Image, ImageDraw

def pico_com_port():
    global pico_ports
    picoVID = "2E8A" # Pico USB Vendor ID
    picoPID = "000A" # Pico USB product ID
    port_list = serial.tools.list_ports.comports()

    pico_ports = []
    for device in port_list:
        if (device.vid != None or device.pid != None):
            if ('{:04X}'.format(device.vid) == picoVID and
                '{:04X}'.format(device.pid) == picoPID):
                pico_ports.append(device.device)

    if len(pico_ports) == 0:
        print("Raspberry Pi Pico not found. Please check VID and PID IDs are correct.")
    elif len(pico_ports) == 1:
        print("1 Pico device found at port: ", pico_ports[0])
    else:
        print(len(pico_ports), "Pico devices found at ports: ", pico_ports)

    return pico_ports

def get_img_directories():
	global wheel, screenshot, boxart, consol
	tempFile = open('/tmp/retropie-oled.log', 'r', -1)    # , "utf-8")
	retropie_oled_img_dir = tempFile.readlines()
	tempFile.close()
	screenshot = retropie_oled_img_dir[0].rstrip('\n')
	wheel = retropie_oled_img_dir[1].rstrip('\n')
	boxart = retropie_oled_img_dir[2].rstrip('\n')
	consol = retropie_oled_img_dir[3].rstrip('\n')

def center_crop(img,dim):
	width, height = img.shape[1], img.shape[0]
	crop_width = dim[0] if dim[0]<img.shape[1] else img.shape[1]
	crop_height = dim[1] if dim[1]<img.shape[0] else img.shape[0]
	start_x = (width - crop_width) // 2
	start_y = (height - crop_height) // 2
	return img[start_y:start_y+crop_height, start_x:start_x+crop_width]

GAME_START_FILE = '/dev/shm/runcommand.info'

def get_combined_image():
    img = cv2.imread(screenshot, cv2.IMREAD_UNCHANGED)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGBA)
    img2 = cv2.imread(wheel, cv2.IMREAD_UNCHANGED)
    img2 = cv2.cvtColor(img2, cv2.COLOR_BGR2RGBA)

    if img.shape[1] > img.shape[0]:
        r = 128.0 / img.shape[0]
        dim = (int(img.shape[1] * r), 128)
    elif img.shape[0] > img.shape[1]:
        r = 128.0 / img.shape[1]
        dim = (128, int(img.shape[0] * r))
    else:
        dim = (128,128)

    img = cv2.resize(img, dim, interpolation=cv2.INTER_AREA)
    img = cv2.copyMakeBorder(img, 128, 128 ,128, 128, cv2.BORDER_CONSTANT, value=[0,0,0,0])
    img = center_crop(img, (128,128))

    r = 128.0 / img2.shape[1]
    dim = (128, int(img2.shape[0] * r))

    img2 = cv2.resize(img2, dim, interpolation=cv2.INTER_AREA)
    img2 = cv2.copyMakeBorder(img2, 128, 128 ,128, 128, cv2.BORDER_CONSTANT, value=[0,0,0,0])
    img2 = center_crop(img2, (128,128))

    cv2.imwrite('screenshot_scaled.png', img)
    cv2.imwrite('wheel_scaled.png', img2)

    background = Image.fromarray(img)
    foreground = Image.fromarray(img2)

    background.paste(foreground, (0, 0), foreground)
    output = background.save('combined.png')
    
    len_argument = len(sys.argv)
    filesize = 0
    if (len_argument != 7):
        sys.exit(0)
    try:
        im=Image.open(sys.argv[1])
    except:
        print("Failed to open png file ", sys.argv[1])
        sys.exit(0)

    image_height = im.size[1]
    image_width = im.size[0]

    try:
        binoutfile = open(sys.argv[2],"wb")
    except:
        print ("Can't write the binary file %s" % sys.argv[2])
        sys.exit(0)

    pix = im.load()
    for h in range(image_height):
        for w in range(image_width):
            if w < im.size[0]:
                R=pix[w,h][0]>>3
                G=pix[w,h][1]>>2
                B=pix[w,h][2]>>3

                rgb = (R<<11) | (G<<5) | B

                binoutfile.write(struct.pack('H', rgb))
            else:
                rgb = 0

    binoutfile.close()

def get_boxart_image():
    img = cv2.imread(boxart, cv2.IMREAD_UNCHANGED)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGBA)

    if img.shape[0] > img.shape[1]:
        r = 128.0 / img.shape[0]
        dim = (int(img.shape[1] * r), 128)
    elif img.shape[1] > img.shape[0]:
        r = 128.0 / img.shape[1]
        dim = (128, int(img.shape[0] * r))
    else:
        dim = (128, 128)

    img = cv2.resize(img, dim, interpolation=cv2.INTER_AREA)
    img = cv2.copyMakeBorder(img, 128, 128, 128, 128, cv2.BORDER_CONSTANT, value=[0, 0, 0, 0])
    img = center_crop(img, (128, 128))

    cv2.imwrite('boxart_scaled.png', img)

    boxart_image = Image.fromarray(img)

    output = boxart_image.save('boxart.png')

    try:
        im=Image.open(sys.argv[3])
    except:
        print("Failed to open png file ", sys.argv[3])
        sys.exit(0)

    image_height = im.size[1]
    image_width = im.size[0]

    try:
        binoutfile = open(sys.argv[4],"wb")
    except:
        print ("Can't write the binary file %s" % sys.argv[4])
        sys.exit(0)


    pix = im.load()
    for h in range(image_height):
        for w in range(image_width):
            if w < im.size[0]:
                R=pix[w,h][0]>>3
                G=pix[w,h][1]>>2
                B=pix[w,h][2]>>3

                rgb = (R<<11) | (G<<5) | B

                binoutfile.write(struct.pack('H', rgb))
            else:
                rgb = 0

    binoutfile.close()

def get_consol_image():
    consol_dir = os.path.abspath(os.path.dirname(consol))
    consol_dir = os.path.join(consol_dir, os.path.basename(consol))
    consol_images = [f for f in os.listdir(consol_dir) if os.path.isfile(os.path.join(consol_dir, f)) and f.endswith('.png')]

    if not consol_images:
        print(f"Consol directory: {consol_dir}")
        sys.exit(0)

    consol_image_file = random.choice(consol_images)
    consol_image_path = os.path.join(consol_dir, consol_image_file)

    img = cv2.imread(consol_image_path, cv2.IMREAD_UNCHANGED)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGBA)

    img = cv2.copyMakeBorder(img, 128, 128 ,128, 128, cv2.BORDER_CONSTANT, value=[0,0,0,0])
    img = center_crop(img, (128,128))

    cv2.imwrite('consol.png', img)

    consol_image = Image.fromarray(img)

    output = consol_image.save('consol.png')    
    len_argument = len(sys.argv)
    filesize = 0
    if (len_argument != 7):
        sys.exit(0)
    try:
        im=Image.open(sys.argv[5])
    except:
        print("Failed to open png file ", sys.argv[5])
        sys.exit(0)

    image_height = im.size[1]
    image_width = im.size[0]

    try:
        binoutfile = open(sys.argv[6],"wb")
    except:
        print ("Can't write the binary file %s" % sys.argv[6])
        sys.exit(0)

    pix = im.load()
    for h in range(image_height):
        for w in range(image_width):
            if w < im.size[0]:
                R=pix[w,h][0]>>3
                G=pix[w,h][1]>>2
                B=pix[w,h][2]>>3

                rgb = (R<<11) | (G<<5) | B

                binoutfile.write(struct.pack('H', rgb))
            else:
                rgb = 0

    binoutfile.close()
    
# Define the path to the lock file
LOCK_FILE = "/tmp/lock_file"

# Open the lock file
lock_file = open(LOCK_FILE, 'w')

# Acquire the blocking lock
fcntl.flock(lock_file, fcntl.LOCK_EX)

while True:
    print("Waiting for game launch event file to be created...")
    while not os.path.exists(GAME_START_FILE):
        time.sleep(0.1)

    print("Game launch event file detected! Noice")
    while not os.path.exists('/tmp/retropie-oled.log'):
        time.sleep(0.1)

    get_img_directories()

    pico_ports = pico_com_port()

    for port in pico_ports:
        # Send combined image to Pico
        get_combined_image()
        ser = serial.Serial(port, 921600)
        ser.write(b"s\n")
        ser.write(b"start\n")
        data = open("combined.bin", "rb")
        ser.write(data.read())
        data.close()
        print("Combined image sent to Pico on port", port)
        ser.close()
                
        # Send boxart image to Pico
        get_boxart_image()
        data = open("boxart.bin", "rb")
        ser = serial.Serial(port, 921600)
        ser.write(data.read())
        data.close()
        print("Boxart image sent to Pico on port", port)
        ser.close()
        
        # Send console image to Pico
        get_consol_image()
        data = open("consol.bin", "rb")
        ser = serial.Serial(port, 921600)
        ser.write(data.read())
        data.close()
        print("Console image sent to Pico on port", port)
        ser.close()
        fcntl.flock(lock_file, fcntl.LOCK_UN)
        lock_file.close()
        print("Lock successfully released")
 

    # Remove the game start file
    os.remove(GAME_START_FILE)
    print("Game launch event file removed")
    time.sleep(5)
    break

#!/usr/bin/python3

from lib2to3.pgen2 import pgen
from operator import truediv
from pickle import TRUE
from serial.tools import list_ports
import sys
import os
import subprocess
import struct
import serial
import linecache
import xml.etree.ElementTree as et
import xml.sax.saxutils as saxutils

import cv2
from PIL import Image
from PIL import ImageDraw
from datetime import datetime

# Host selection (for debugging)
raspberryPi = 0
pc = 1

enable_boxart = 0

def pico_com_port():
    global pico_port
    picoVID = "2E8A" # Raspberry Pi Pico USB vendor ID
    picoPID = "000A" # Raspberry Pi Pico USB product ID
    port_list = serial.tools.list_ports.comports()

    for device in port_list:
        if (device.vid != None or device.pid != None): 
            if ('{:04X}'.format(device.vid) == picoVID and
                '{:04X}'.format(device.pid) == picoPID):
                pico_port = device.device
                break
            pico_port = None
            print("Raspberry Pi Pico failed to open serial port. Please check VID and PID IDs are correct.")

def get_cpu_temp():
    tempFile = open("/sys/class/thermal/thermal_zone0/temp")
    cpu_temp = tempFile.read()
    tempFile.close()
    return float(cpu_temp)/1000

def get_cpu_speed():
    tempFile = open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq")
    cpu_speed = tempFile.read()
    tempFile.close()
    return float(cpu_speed)/1000

def get_ip_address(cmd, cmdeth):
    ipaddr = run_cmd(cmd)

    count = len(ipaddr)
    if count == 0 :
        ipaddr = run_cmd(cmdeth)
    return ipaddr

def get_img_directories():
    global wheel, screenshot, boxart
    tempFile = open('/tmp/retropie_oled.log', 'r', -1, "utf-8")
    retropie_oled_img_dir = tempFile.readlines()
    tempFile.close()
    screenshot = retropie_oled_img_dir[0].rstrip('\n')
    wheel = retropie_oled_img_dir[1].rstrip('\n')
    boxart = retropie_oled_img_dir[2].rstrip('\n')
    
def get_game_metadata():
    tempFile = open('/tmp/retropie_oled.log', 'r', -1, "utf-8")
    gamemetadata = tempFile.readlines()
    tempFile.close()
    game_name = gamemetadata[0].split('/')[-1].rsplit('.',1)[0].split('(')[0].rstrip() # Get the current game name
    system_name = gamemetadata[0].split('/')[5] # Get the current system name
    gamelist_path = os.path.join('/opt/retropie/configs/all/emulationstation/gamelists', system_name, 'gamelist.xml')
    data = et.parse(gamelist_path)
    
    # Find all the games with matching names within gamelist_path
    game_list = data.findall(u".//game[name='{0}']".format(saxutils.escape(game_name)))
    
    for game in game_list:
        if game.find('desc') is not None:
            desc = str('Description is :\n' + game.find('desc').text)
            
        if game.find('') is not None:
            rating = str('Rating:\n' + game.find('rating').text)
            
        if game.find('releasedate') is not None:
            release_date = str ('Release Date:\n' + game.find('releasedate').text)
            
        if game.find('developer') is not None: 
            developer = str ('Developer:\n' + game.find('developer').text)
            
        if game.find('publisher') is not None: 
            publsiher = str ('Publisher:\n' + game.find('releasedate').text)
            
        if game.find('genre') is not None: 
            genre = str ('Genre:\n' + game.find('genre').text)

def center_crop(img, dim):
    width, height = img.shape[1], img.shape[0]
    crop_width = dim[0] if dim[0]<img.shape[1] else img.shape[1]
    crop_height = dim[1] if dim[1]<img.shape[0] else img.shape[0] 
    mid_x, mid_y = int(width/2), int(height/2)
    cw2, ch2 = int(crop_width/2), int(crop_height/2) 
    crop_img = img[mid_y-ch2:mid_y+ch2, mid_x-cw2:mid_x+cw2]
    return crop_img

def main():
    
    if raspberryPi: 
        if enable_boxart:
            get_img_directories()
            get_game_metadata()
            img = cv2.imread(boxart, cv2.IMREAD_UNCHANGED)
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGBA)
            img2 = cv2.imread(boxart, cv2.IMREAD_UNCHANGED)
            img2 = cv2.cvtColor(img2, cv2.COLOR_BGR2RGBA)
        else:   
            get_img_directories()
            get_game_metadata()
            img = cv2.imread(screenshot, cv2.IMREAD_UNCHANGED)
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGBA)
            img2 = cv2.imread(wheel, cv2.IMREAD_UNCHANGED)
            img2 = cv2.cvtColor(img2, cv2.COLOR_BGR2RGBA)
    elif pc: # PC mode (debug)
        img = cv2.imread('screenshot.png', cv2.IMREAD_UNCHANGED)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGBA)
        img2 = cv2.imread('wheel.png', cv2.IMREAD_UNCHANGED) 
        img2 = cv2.cvtColor(img2, cv2.COLOR_BGR2RGBA)
    
    # Grab bit-depth of screenshot image
    # pngchannels = img.shape[2]
    # datatype = str(img.dtype)
    # datatypeinbits = int(re.search(r'\d+', datatype).group())
    # bpp = pngchannels * datatypeinbits

    # img.shape[1] = width
    # img.shape[0] = height

    if img.shape[1] > img.shape[0]: # height smaller than width
        r = 128.0 / img.shape[0]
        dim = (int(img.shape[1] * r), 128)
    elif img.shape[0] > img.shape[1]: # width smaller than height
        r = 128.0 / img.shape[1]
        dim = (128, int(img.shape[0] * r))
    else: # 1:1 aspect ratio
        dim = (128,128)

    img = cv2.resize(img, dim, interpolation=cv2.INTER_AREA)
    img = cv2.copyMakeBorder(img, 128, 128, 128, 128, cv2.BORDER_CONSTANT,value=[0,0,0,0])
    img = center_crop(img, (128,128))

    r = 128.0 / img2.shape[1]
    dim = (128, int(img2.shape[0] * r))

    img2 = cv2.resize(img2, dim, interpolation=cv2.INTER_AREA)
    img2 = cv2.copyMakeBorder(img2, 128, 128, 128, 128, cv2.BORDER_CONSTANT,value=[0,0,0,0])
    img2 = center_crop(img2, (128,128))

    cv2.imwrite('screenshot_scaled.png', img)
    cv2.imwrite('wheel_scaled.png', img2)

    background = Image.fromarray(img)
    foreground = Image.fromarray(img2)

    output = Image.alpha_composite(background, foreground).save('combined.png')

    len_argument = len(sys.argv)
    filesize = 0
    if (len_argument != 3):
      print ("")
      print ("Correct Usage:")
      print ("\tpython png2rgb565.py <png_file> <binary_file>")
      print ("")
      sys.exit(0)

    try:
        im=Image.open(sys.argv[1])
        #print ("/* Image Width:%d Height:%d */" % (im.size[0], im.size[1]))
    except:
        print ("Fail to open png file ", sys.argv[1])
        sys.exit(0)

    image_height = im.size[1]
    image_width = im.size[0]

    try:
        binoutfile = open(sys.argv[2],"wb")
    except:
        print ("Can't write the binary file %s" % sys.argv[2])
        sys.exit(0)

    pix = im.load()  #load pixel array
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

    if raspberryPi:
        data = open("out.bin","rb")
        pico_com_port()
        ser = serial.Serial(pico_port, 921600)
        ser.write(data.read())  
        data.close()
    elif pc:
        ser = serial.Serial('COM3', 921600)
        data = open("out.bin","rb")
        ser.write(data.read())  
        data.close()

if __name__=="__main__":
  main()

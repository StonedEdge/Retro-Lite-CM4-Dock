# Retro Lite CM4 AIO Docking Station 

<p align="center">
   <img src="https://i.imgur.com/RhksMgQ.jpg" width = 50% height = 50%/>
</p>

# Description 
Note: Currently the code is a WIP and the project isn't completed yet so you won't be able to compile the program right now. 

A basic docking station designed to be used with the Retro Lite CM4 gaming console. The main features of this project include:


- Fully integrated 1.5" 128x128 SPI driven 16-bit color display 
- User configurable splashscreen on power boot
- USB-C charging 
- HDMI video output
- Multifunction pushbutton to switch modes

Modes currently planned are: 

1) Splashscreen until COM port detected 
2) Pi Stats - Shows basic system hardware stats 
3) Game metadata on boot (visuals)
4) Game metadata on boot (text)
5) Game statistics (play time)

# Features

- Console communicates to Pico over USB
- Shows disk usage, SoC temp, clock speed, RAM usage & IP address of the console
- Shows game metadata of the game/system booted, including:

a) **Current game cover (boxart)** - /opt/retropie/configs/all/emulationstation/downloaded_images/{system}/{rom_name}.png

b) **Screenshot + combined wheel image** - /home/pi/retropie/roms/{system}/media/wheel/{rom_name}.png && /home/pi/retropie/roms/{system}/media/screenshot/{rom_name}.png

c) **Randomized vector images of console released** - /home/pi/pico/pico-examples/pico-spi/consolevectors

d) **Game description using vertical scrolling** - /opt/retropie/configs/all/emulationstation/gamelists', system_name, 'gamelist.xml'

e) **Game developer** - /opt/retropie/configs/all/emulationstation/gamelists', system_name, 'gamelist.xml' 

f) **Release date** - /opt/retropie/configs/all/emulationstation/gamelists', system_name, 'gamelist.xml'

g) **Videos** - /home/pi/retropie/roms/{system}/media/videos/{rom_name}.mp4

h) Shows top 3 games played overall on the system, when a game is not running based on langest RetroStats: https://github.com/langest/RetroStats

i) Shows top 3 games played based on system launched, including duration of time played, based on langest RetroStats: https://github.com/langest/RetroStats

                     Wario Land 3 ▏ 6:36:28 ████████████████████
                Crash Team Racing ▏ 2:01:08 ██████
                   Super Mario 64 ▏ 1:59:56 ██████

j) Shows battery statistics, including time-to-full, % SoC from connected MAX17055 gauge

Other:
- Integrated sleep + wake mode, blanking after 1-min of no user input 
- 3 USB 2.0 ports for peripherals such as a keyboard, USB mouse 
- Written in C (Pico) & Python (CM4)
- Uses a RP2040 Pico microcontroller

# To-do
- [x] Detect current game/system booted using `runcommand`
- [x] Add Retrolite CM4 splashscreen on boot
- [ ] Stop splashscreen on COM port detect and switch to stats mode
- [x] Downscale + convert screenshot and wheel images to 128x128 RGB565 on Pi 4 (Python)
- [x] Detect RP2040 by Product/Vendor IDs (VID/PIDs) 
- [ ] Identify how to perform hardware scrolling of text on the SSD1351 for game descriptions 
- [ ] Identify how extract game text metadata using Python 
- [x] Send system stats to RP2040 over USB 
- [ ] Implement mode toggles 
- [ ] Implement a 1 min timeout which fades black to the OLED with no user input, wake up OLED on button press (Nice to have)
- [ ] Implement battery stats (Nice to have) 

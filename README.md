# Retro-Lite-CM4-Dock
<p align="center">
   <img src="https://i.imgur.com/RhksMgQ.jpg" width = 50% height = 50%/>
</p>

# Description 
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

- Shows disk usage, SoC temp, clock speed, RAM usage & IP address of the console 
- Shows game metadata of the game/system booted, including current game cover (boxart), screenshot + combined wheel image, game description using vertical scrolling, developer, release date, short video of game, etc, togglable with push button
- Shows top 3 games played overall on the system, when a game is not running based on langest RetroStats: https://github.com/langest/RetroStats
- Shows top 3 games played based on system launched, including duration of time played, based on langest RetroStats: https://github.com/langest/RetroStats

                     Wario Land 3 ▏ 6:36:28 ████████████████████
                Crash Team Racing ▏ 2:01:08 ██████
                   Super Mario 64 ▏ 1:59:56 ██████

- Shows battery statistics, including time-to-full, % SoC (WIP)
- Integrated sleep + wake mode, blanking after 1-min of no user input 
- 3 USB 2.0 ports for peripherals such as a keyboard, USB mouse 
- Writte in C/Python 
- Uses a RP2040 Pico microcontroller 

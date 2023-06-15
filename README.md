<div align="center">

# **Retro Lite CM4 AIO Docking Station**

</div>

<p align="center">
   <img src="https://imgur.com/gyIgzIV.jpg" width = 30% height = 30%/>
</p>
<p align="center">
   <img src="https://imgur.com/Ze7b57e.jpg" width = 30% height = 30%/>
</p>
<p align="center">
   <img src="https://imgur.com/WnWMyfu.jpg" width = 30% height = 30%/>
</p>

# Description 
Updated: 2023/06/16

Note: Code base is now functional in terms of splashscreen, statistics images and metadata with text wrap! 
Please see a to-do list for list of features still required to be completed. 

-----------------------------------------------------------------------------------------------------------

The Retro Lite CM4 docking station/hub is designed to be used with the Retro Lite CM4 gaming console however will work on any generic Linux host running RetroPie. 
The main goal of this project is to act as a metadata box for your RetroPie system & learn more about the games you are playing in real time - the aim is to preserve a small part of retro gaming history through this project. The project boasts the following features:

- Fully integrated 1.5" 128x128 SPI driven 16-bit color display 
- 3 modes, including **splashscreen**, **stats** and **game** mode
- 3 x external USB 2.0 ports to use for keyboard, mouse & other peripherals in desktop mode. USB controller compatible for low latency
- USB-C charging 
- HDMI video output 

Here is the general flow of the program and how it works when power is applied:

1) **Splashscreen mode**
   - Splashscreen is played until EmulationStation starts. After which, stats mode is entered
2) **Stats mode**
   - Shows general system statistics - IP address, SoC temperature, RAM usage, etc. Stats are updated in 5 second intervals
   - Retro stats - see your systems overall top 5 games played by (a) duration and (b) times played 
3) **Game mode**
   - Shows a combined image of the game being played on launch of any game
   - Cycle through 5 different metadata views via the push button attached to the docking station. These are: 

         a) Combined vector image of wheel and screenshot

         b) Official game boxart

         c) A randomized vector image of the real hardware the game could be played on. Includes all variants of every console (shoutouts to Ampersand for providing the high quality vectors!)

         d) A description of the game, which scrolls vertically along the display

         e) Other metadata, including rating, release date, developer, publisher, genre & # players per game 

# Installation
To install, run (won't currently work yet!)
```
git clone https://github.com/StonedEdge/Retro-Lite-CM4-Dock.git
sudo chmod +x install_dock.sh
./ install_dock.sh
```

I recommend scraping your media files via the inbuilt scraper in the RetroPie setup script menus. This will download all of the metadata required and automatically store it in /home/pi/RetroPie/roms/{system_name}/media. 

# Metadata directory locations 

a) **Current game cover (boxart)** - /opt/retropie/configs/all/emulationstation/downloaded_images/{system}/{rom_name}.png

b) **Screenshot + combined wheel image** - /home/pi/retropie/roms/{system}/media/wheel/{rom_name}.png && /home/pi/retropie/roms/{system}/media/screenshot/{rom_name}.png

c) **Randomized vector images of console released** - /home/pi/pico/pico-examples/pico-spi/consolevectors

d) **Game description using vertical scrolling** - /opt/retropie/configs/all/emulationstation/gamelists', system_name, 'gamelist.xml'

e) **Game developer** - /opt/retropie/configs/all/emulationstation/gamelists', system_name, 'gamelist.xml' 

f) **Release date** - /opt/retropie/configs/all/emulationstation/gamelists', system_name, 'gamelist.xml'

g) **Videos** - /home/pi/retropie/roms/{system}/media/videos/{rom_name}.mp4

Example command: 
```-c total -b 20 -n 3 -c times```

Translation:

"Get the total time (in hours, mins, seconds) played and display it in a bar chart of 20 steps. Get only the top 3 systems. Display times played as an integer"
 
                            
                     Wario Land 3 ▏ 3 ████████████████████
                Crash Team Racing ▏ 2 ██████
                   Super Mario 64 ▏ 1 ██████

i) Shows top 3 games played based on system launched, including duration of time played, based on langest RetroStats: https://github.com/langest/RetroStats

Example command: 
```-c total -b 20 -n 3 -e kodi```

Translation:

"Get the total time (in hours, mins, seconds) played and display it in a bar chart of 20 steps. Get only the top 3 systems, exlcuding Kodi"

                     Wario Land 3 ▏ 6:36:28 ████████████████████
                Crash Team Racing ▏ 2:01:08 ██████
                   Super Mario 64 ▏ 1:59:56 ██████

Arguments to use to show what you want on the OLED display. Can be adjusted by the user here: 
```
$ retro-stats-cli -h
usage: retro-stats-cli [-h] [-n LIST_LENGTH] [-f FILE] [-c CRITERIA]
                       [-m MINIMUM_SESSION_LENGTH] [-s SYSTEMS [SYSTEMS ...]]
                       [-e EXCLUDE_SYSTEMS [EXCLUDE_SYSTEMS ...]]
                       [-l LOOKBACK] [-w | -b BAR_CHART | -r]
                       
optional arguments:
  -h, --help            show this help message and exit
  -n LIST_LENGTH, --list-length LIST_LENGTH
                        how many entries to print, defaults to 25
  -f FILE, --file FILE  path to the stats file, defaults to
                        /home/pi/RetroPie/game_stats.log
  -c CRITERIA, --criteria CRITERIA
                        which criteria to order by, disabled for schedule
                        option, available options are: total (time), times
                        (played), average (session length), median (session
                        length), defaults to total
  -m MINIMUM_SESSION_LENGTH, --minimum-session-length MINIMUM_SESSION_LENGTH
                        skip sessions shorter than this number of seconds,
                        defaults to 120
  -s SYSTEMS [SYSTEMS ...], --systems SYSTEMS [SYSTEMS ...]
                        the systems you want statistics for, default will use
                        all systems
  -e EXCLUDE_SYSTEMS [EXCLUDE_SYSTEMS ...], --exclude-systems EXCLUDE_SYSTEMS [EXCLUDE_SYSTEMS ...]
                        skip the listed systems, default no systems
  -l LOOKBACK, --lookback LOOKBACK
                        Number of days lookback to use for the stats, defaults
                        to no limit (0)
  -w, --weekly-schedule
                        display weekly time schedule
  -b BAR_CHART, --bar-chart BAR_CHART
                        display bar chart instead of numbers, integer sets bar
                        length
  -r, --recently_played
                        print your game history
```

# Hardware 
![Image of Retro Lite CM4](https://i.imgur.com/LP2ecQt.png)

Pinout to connect the OLED screen to the docking station is located above. Gerbers and CAD (both docking station and separate OLED box) files will be updated soon once the software is completed. 

# To-do
- [x] Detect current game/system booted using `runcommand`
- [x] Add Retrolite CM4 splashscreen on boot 
- [x] Downscale + convert screenshot and wheel images to 128x128 RGB565 on Pi 4 (Python)
- [x] Detect RP2040 by Product/Vendor IDs (VID/PIDs)  
- [x] Identify how to extract game text metadata using Python 
- [x] Send system stats to RP2040 over USB 
- [x] Clear buffer when runcommand-onend.sh is executed when a game is closed, bringing back user to Pi stats view
- [x] Stop splashscreen on COM port detect and switch to stats mode
- [x] Send over RGB565 combined.png over serial without corruption, automatic switching from Pi stats to current game
- [x] Write a python script to scale the boxart correctly to fit the screen
- [x] Send over random image to Pico located in consolevectors based on current system being launched on RetroPie (no scaling necessary)
- [x] Implement a button state machine to switch between the stats and image mode
- [x] Implement a timeout which fades black to the OLED with no button input. Wake up OLED on button press after X mins of no activity 
- [x] Implement game metadata within game mode
- [x] Gracefully shutdown the OLED display when the console is powered down and restart it when console is repowered on
- [x] Identify how to perform hardware (or software) vertical scrolling of text on the SSD1351 for game descriptions 

# Nice to have/probably won't implement any time soon! 
- [ ] Write a python script to break up video into individual video frames on the Pi 4 
- [ ] Setup tinyUSB library to try and achieve 0.9MB/s transfer speeds for all frames, for 30 FPS video over USB 1.1 (max 1.5MB/s)
- [ ] Implement "retrostats" into stats mode to show user most played games by **times played** & **time duration** using the RetroStats scripts. Note : No RTC so time duration may not be accurate. 

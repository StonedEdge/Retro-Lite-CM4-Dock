#!/usr/bin/env bash

# Clear out retropie-oled.log previous ROM directory data
> /tmp/retropie-oled.log

# Get the system name
system=$1

# Get the full path filename of the ROM and store it in rom
rom=$3

# rom_bn receives $rom excluding everything from the first char to the last slash '/'
rom_bn="${rom##*/}"

GAMELIST1="/home/pi/RetroPie/roms/${system}/gamelist.xml"
GAMELIST2="/home/pi/.emulationstation/gamelists/${system}/gamelist.xml"

if [ -f ${GAMELIST1} ]
then
GAMELIST=${GAMELIST1}
else
GAMELIST=${GAMELIST2}
fi

title=`grep -s -w -A1 "${rom_bn}" ${GAMELIST} | awk '{getline;print}' | awk 'BEGIN {FS="<name>"} {print $2}' | awk 'BEGIN {FS="</name>"} {print $1}'`
title="${title%%(*}"
rom_bn="${rom_bn%.*}"

echo "$HOME/RetroPie/roms/$1/media/screenshot/${rom_bn}.png" >> /tmp/retropie-oled.log
echo "$HOME/RetroPie/roms/$1/media/wheel/${rom_bn}.png" >> /tmp/retropie-oled.log
echo "/opt/retropie/configs/all/emulationstation/downloaded_images/${system}/${rom_bn}-image.png" >> /tmp/retropie-oled.log
echo "$HOME/RetroliteOSD/consolevectors/${system}" >> /tmp/retropie-oled.log
echo $(date -u +%Y-%m-%dT%H:%M:%S%z)'|'start'|'$1'|'$2'|'$3'|'$4 >> ~/RetroPie/game_stats.log

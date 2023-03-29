#!/bin/bash

# Install python3 and pip3
sudo apt update
sudo apt install python3 python3-pip -y

# Install required python packages
pip3 install gitpython pyserial opencv-python psutil pillow

# Clone the repository
git clone https://github.com/StonedEdge/Retro-Lite-CM4-Dock.git

# Find the connected USB with Pico VID and PID
pico_port=""
pico_device=$(ls /dev/serial/by-id/*${picoVID}*${picoPID}* 2> /dev/null)
if [ -n "$pico_device" ]; then
    pico_port=$(realpath "$pico_device")
    echo "Found Pico at $pico_port"
else
    echo "Pico not found"
    exit 1
fi

# Copy main.uf2 to the Pico USB
sudo cp Retro-Lite-CM4-Dock/firmware/main.uf2 "$pico_port"

# Copy runcommand_onhooks files to /opt/retropie/configs/all
sudo cp -R Retro-Lite-CM4-Dock/runcommand_onhooks/* /opt/retropie/configs/all/

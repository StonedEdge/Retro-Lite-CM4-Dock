#!/bin/bash

echo "Installing python3 and pip3..."
# Install python3 and pip3
sudo apt update
if ! sudo apt install python3 python3-pip -y; then
    echo "Error: Failed to install python3 and pip3"
    exit 1
fi
echo "python3 has been successfully installed, moving on..."

echo "Checking if RetroStats is already installed..."
# Check if RetroStats is already installed
if pip3 show RetroStats >/dev/null 2>&1; then
    echo "RetroStats is already installed"
else
    echo "Installing RetroStats..."
    # Clone RetroStats repository
    cd RetroPie
    git clone https://github.com/langest/RetroStats
    cd RetroStats
    # Install RetroStats Python package
    if ! pip3 install . --user; then
        echo "Error: Failed to install RetroStats"
        exit 1
    fi
    echo "RetroStats has been successfully installed, moving on..."
fi

echo "Installing required python packages..."
# Install required python packages
if ! pip3 install gitpython pyserial opencv-python-headless psutil pillow; then
    echo "Error: Failed to install required python packages"
    exit 1
fi
echo "Python3 packages have been successfully installed, moving on..."

echo "Finding the connected USB with Pico VID and PID..."
# Find the connected USB with Pico VID and PID
pico_port=""
pico_device=$(ls /dev/serial/by-id/*${picoVID}*${picoPID}* 2> /dev/null)
if [ -n "$pico_device" ]; then
    pico_port=$(realpath "$pico_device")
    echo "Found Pico at $pico_port"
    echo "Raspberry Pi Pico has been successfully found on $pico_port! Nice job!"
else
    echo "Error: Raspberry Pi Pico is not connected - try again?"
    exit 1
fi

echo "Copying main.uf2 to the Pico USB..."
# Copy main.uf2 to the Pico USB
if ! sudo cp Retro-Lite-CM4-Dock/firmware/main.uf2 "$pico_port"; then
    echo "Error: Failed to copy main.uf2 to the Pico USB - check if Pico is connected. Pico not programmed!"
    exit 1
fi
echo "Pico successfully programmed with Retro Lite CM4 dock firmware. Nice job!"

echo "Copying runcommand_onhooks files to /opt/retropie/configs/all... to read game on startup"
# Copy runcommand_onhooks files to /opt/retropie/configs/all
if ! sudo cp -R Retro-Lite-CM4-Dock/runcommand_onhooks/* /opt/retropie/configs/all/; then
    echo "Error: Failed to copy runcommand_onhooks files to /opt/retropie/configs/all"
    exit 1
fi
echo "runcommand_onhooks successfully moved to the correct location. Nice!"

echo "Retro Lite CM4 docking firmware is now installed. Give it a try! Now exiting..."

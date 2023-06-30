#!/bin/bash

# Traverse all subdirectories in the given directory and convert JPEG images
# to PNG format
find /opt/retropie/configs/all/emulationstation/downloaded_images -type d | while read dir; do
  find "$dir" -maxdepth 1 -type f -name "*.jpg" -print0 | while read -d $'\0' file; do
    echo "Converting $file to PNG..."
    convert "$file" "${file%.jpg}.png"
    rm "$file" # remove original .jpg file
  done
done

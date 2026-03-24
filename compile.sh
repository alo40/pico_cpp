#!/bin/bash
echo "Starting tasks..."
cp oled_time.c main.c
rm -rf build
mkdir build
cd build
cmake ..
make
picotool load -f pico_app.uf2
cd ..
echo "All done!"

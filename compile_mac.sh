#!/bin/bash
set -e

echo "Starting tasks..."

cp oled_time.c main.c
rm -rf build
mkdir build
cd build

cmake -G Ninja ..
ninja

echo "Sending UF2 to Raspberry Pi 5..."

scp pico_app.uf2 raspi:~/pico_cpp/pico_app.uf2

echo "Flashing Pico from Raspberry Pi 5..."

ssh raspi 'picotool load -f -x ~/pico_cpp/pico_app.uf2'

echo "Build, transfer, and remote flash complete."

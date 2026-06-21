#!/bin/bash
set -e

echo "Starting tasks..."

# Optional clean build:
# ./compile_mac.sh clean
if [ "$1" = "clean" ]; then
    echo "Removing build folder..."
    rm -rf build
fi

cp oled_multiple.c main.c
# cp oled_dynamic.c main.c
# cp oled_time.c main.c # previous script

echo "Configuring project..."

cmake -S . -B build -G Ninja

echo "Building project..."

cmake --build build

echo "Preparing Raspberry Pi 5 target folder..."

ssh raspi 'mkdir -p ~/pico_cpp'

echo "Sending UF2 to Raspberry Pi 5..."

scp build/pico_app.uf2 raspi:~/pico_cpp/pico_app.uf2

echo "Flashing Pico from Raspberry Pi 5..."

ssh raspi 'picotool load -f -x ~/pico_cpp/pico_app.uf2'

echo "Build, transfer, and remote flash complete."
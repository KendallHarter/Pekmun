#!/bin/bash

set -e

mkdir -p build/

${DEVKITARM}/bin/arm-none-eabi-g++ -fno-rtti -fno-exceptions -Wall -Wextra -Wpedantic -O3 -std=c++2a -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb -mthumb-interwork -c main.cpp -o build/main.o
${DEVKITARM}/bin/arm-none-eabi-g++ -fno-rtti -fno-exceptions -Wl,-Map,build/out.map -mthumb -mthumb-interwork -specs=gba.specs build/main.o -o build/out.elf
${DEVKITARM}/bin/arm-none-eabi-objcopy -O binary build/out.elf build/out.gba
${DEVKITPRO}/tools/bin/gbafix build/out.gba

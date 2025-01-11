#!/bin/sh
cl
yasm x86boot.asm
gcc -O3 -std=c99 -m32 -march=i386 -fno-PIC -nostdlib -ffreestanding \
-o dos.com -Wl,--nmagic,--script=com.ld main.c && dosbox dos.com \
&& cat x86boot dos.com > seraphim.img #\
#&& qemu-system-x86_64 seraphim.img

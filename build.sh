#!/bin/bash
echo "=== Компиляция Ассемблера ==="
nasm -f elf32 multiboot.asm -o multiboot.o

echo "=== Компиляция C ==="
gcc -m32 -march=i386 -ffreestanding -nostdlib -nodefaultlibs -c kernel.c -o kernel.o

echo "=== Линковка ==="
ld -m elf_i386 -Ttext 0x100000 multiboot.o kernel.o -o kernel.bin

echo "=== Сборка ISO ==="
cp kernel.bin iso/boot/
grub-mkrescue -o ocean.iso iso

echo "=== Готово! ==="

#!/bin/bash
set -e

echo "=== Компиляция Ассемблера ==="
nasm -f elf32 multiboot.asm -o multiboot.o

echo "=== Компиляция C ==="
gcc -m32 -march=i386 -ffreestanding -nostdlib -nodefaultlibs -c kernel.c -o kernel.o

# Если есть main.c, раскомментируй строчку ниже:
# gcc -m32 -march=i386 -ffreestanding -nostdlib -nodefaultlibs -c main.c -o main.o

echo "=== Линковка ==="
ld -m elf_i386 -Ttext 0x100000 multiboot.o kernel.o -o kernel.bin

echo "=== Подготовка структуры ISO ==="
# Создаем папки под ISO, если их нет
mkdir -p iso/boot/grub

# Копируем ядро и конфиг GRUB
cp kernel.bin iso/boot/
cp grub.cfg iso/boot/grub/grub.cfg

echo "=== Сборка ISO ==="
grub-mkrescue -o ocean.iso iso

echo "=== Готово! ==="

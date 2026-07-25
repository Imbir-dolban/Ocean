# Компилятор и флаги для i386 bare metal
CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-ld

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32 \
         -fno-pic -fno-stack-protector -fno-builtin -nostdlib \
         -I.
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# Целевые файлы
KERNEL = kernel.elf
ISO = os.iso

# Если main.c тоже используется, добавьте main.o в OBJS ниже:
OBJS = boot.o kernel.o

all: $(ISO)

# Сборка ISO образа для GRUB
$(ISO): $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/kernel.elf
	echo 'menuentry "Ocean Kernel" {' > isodir/boot/grub/grub.cfg
	echo '	multiboot /boot/kernel.elf' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir
	@echo "Build successful: $(ISO)"

# Линковка ядра
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Компиляция C (автоматическое правило для всех .c файлов)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ассемблер
boot.o: boot.s
	$(AS) --32 $< -o $@

# Запуск в QEMU
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

# Очистка
clean:
	rm -f *.o $(KERNEL) $(ISO)
	rm -rf isodir

.PHONY: all run clean

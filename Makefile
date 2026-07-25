# Компилятор и флаги для i386 bare metal
CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32 \
         -fno-pic -fno-stack-protector -fno-builtin -nostdlib \
         -I.
LDFLAGS = -T linker.ld -nostdlib -lgcc

# Целевые файлы
KERNEL = kernel.elf
ISO = os.iso

OBJS = boot.o kernel.o

all: $(ISO)

# Сборка ISO образа для GRUB
$(ISO): $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/kernel.elf
	cat > isodir/boot/grub/grub.cfg << 'EOF'
menuentry "Ocean Kernel" {
	multiboot /boot/kernel.elf
}
EOF
	grub-mkrescue -o $(ISO) isodir 2>/dev/null
	@echo "Build successful: $(ISO)"

# Линковка ядра
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Компиляция C
kernel.o: kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ассемблер
boot.o: boot.s
	$(AS) $< -o $@

# Запуск в QEMU
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -serial stdio

# Очистка
clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	rm -rf isodir

.PHONY: all run clean
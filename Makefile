# Компилятор и флаги для i386 bare metal
CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-ld

# Получаем короткий хэш коммита из Git (если repo инициализирован)
GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "dev")

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32 \
         -fno-pic -fno-stack-protector -fno-builtin -nostdlib \
         -DGIT_HASH=\"$(GIT_HASH)\" -I.

LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# Целевые файлы
KERNEL = kernel.elf
ISO = os.iso

# Выбери актуальные объектники (kernel.o или main.o)
OBJS = boot.o kernel.o

all: $(ISO)

# Сборка ISO образа для GRUB
$(ISO): $(KERNEL)
	@mkdir -p isodir/boot/grub
	@cp $(KERNEL) isodir/boot/kernel.elf
	@echo 'menuentry "Ocean Kernel ($(GIT_HASH))" {' > isodir/boot/grub/grub.cfg
	@echo '	multiboot /boot/kernel.elf' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir
	@echo "--- Build successful: $(ISO) (Commit: $(GIT_HASH)) ---"

# Линковка ядра
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Компиляция всех .c файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ассемблер
boot.o: boot.s
	$(AS) --32 $< -o $@

# Запуск в QEMU
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

# Очистка перед пушем/пересборкой
clean:
	rm -rf *.o $(KERNEL) $(ISO) isodir

.PHONY: all run clean

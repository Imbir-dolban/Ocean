# boot.s - Multiboot1 Header + Entry Point for i386
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Stack size: 16 KiB
.set STACK_SIZE, 16384
.lcomm stack_bottom, STACK_SIZE

.section .text
.global _start
.type _start, @function
_start:
    # Установка указателя стека (растет вниз)
    movl $stack_bottom + STACK_SIZE, %esp

    # Вызов C-функции kernel_main
    call kernel_main

    # Если kernel_main вернул управление — вечный цикл
    cli
1:  hlt
    jmp 1b

.size _start, . - _start
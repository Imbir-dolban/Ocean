ENTRY(_start)

SECTIONS
{
    . = 1M; /* Ядро загружается по адресу 0x00100000 (1 MB) */

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)   /* Multiboot header MUST be first */
        *(.text)        /* Код (boot.s, kernel.c) */
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata*)     /* Read-only data */
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)        /* Initialized data */
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)       /* Uninitialized data (global vars, stack) */
        *(.bss)
    }

    /* Удаляем секции, не нужные для bare-metal */
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.build-id) }
}
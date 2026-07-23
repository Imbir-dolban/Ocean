[bits 32]

section .multiboot
align 4
    dd 0x1BADB002            ; Magic number
    dd 0x00                  ; Flags
    dd - (0x1BADB002 + 0x00) ; Checksum

section .bss
align 16
stack_bottom:
    resb 16384               ; Выделяем 16 КБ под стек
stack_top:

section .text
global _start
extern _start_c

_start:
    cli                      ; Выключаем прерывания
    mov esp, stack_top       ; Устанавливаем указатель стека на вершину!
    
    call _start_c            ; Передаем управление в Си

.loop:
    hlt                      ; Если Си каким-то образом завершился — спим
    jmp .loop

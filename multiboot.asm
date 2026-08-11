[bits 32]

section .multiboot
align 4
    dd 0x1BADB002            ; Числа
    dd 0x00                  ; Флаги
    dd - (0x1BADB002 + 0x00) ; КонтрСумма

section .bss
align 16
stack_bottom:
    resb 16384               ; Выделяем 16 КБ под стек
stack_top:

section .text
global _start
extern kernel_main

_start:
    cli                      ; Выключаем прерывания
    mov esp, stack_top       ; Устанавливаем указатель стека
    xor ebp, ebp             ; Очищаем base pointer
    
    call kernel_main         ; Передаем управление в Си

.loop:
    hlt                      ; Если Си каким-то образом завершился — спим
    jmp .loop

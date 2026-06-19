bits 32
section .text
global _start
extern _start_c

_start:
        jmp real_start           ; Прыгаем мимо заголовка на код запуска
        
        align 4                  ; Выравнивание по 4 байта (требование Multiboot)
        dd 0x1BADB002            ; Магическое число
        dd 0x00                  ; Флаги
        dd - (0x1BADB002 + 0x00) ; Контрольная сумма

real_start:
        cli                      ; Выключаем прерывания
        call _start_c            ; Улетаем в С
        hlt                      ; Если С вернул управление тушим процессор

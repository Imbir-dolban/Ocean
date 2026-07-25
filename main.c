// main.c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================
// КОНСТАНТЫ
// ============================================================
#define VIDEO_MEMORY ((volatile uint16_t*)0xB8000)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define SCREEN_CELLS (SCREEN_WIDTH * SCREEN_HEIGHT) // 2000 ячеек

#define STACK_SIZE 256
#define INPUT_BUF_SIZE 128

#define VGA_ATTR_WHITE_ON_BLACK 0x07

// ============================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================
static size_t cursor_pos = 0; // Индекс ячейки [0..1999]

static int rpn_stack[STACK_SIZE];
static int rpn_top = 0;
static int rpn_error = 0; // 0=OK, 1=Overflow, 2=Underflow, 3=DivZero

// ============================================================
// LOW-LEVEL I/O
// ============================================================
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ============================================================
// VGA DRIVER (Cell-based)
// ============================================================
static void vga_putc_at(char c, uint8_t attr, size_t x, size_t y) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    VIDEO_MEMORY[y * SCREEN_WIDTH + x] = ((uint16_t)attr << 8) | (uint8_t)c;
}

static void vga_clear_line(size_t y) {
    for (size_t x = 0; x < SCREEN_WIDTH; x++)
        vga_putc_at(' ', VGA_ATTR_WHITE_ON_BLACK, x, y);
}

void terminal_scroll(void) {
    // Копируем строки 1..24 в 0..23
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            VIDEO_MEMORY[y * SCREEN_WIDTH + x] = VIDEO_MEMORY[(y + 1) * SCREEN_WIDTH + x];
        }
    }
    vga_clear_line(SCREEN_HEIGHT - 1);
    cursor_pos = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        cursor_pos = (cursor_pos / SCREEN_WIDTH + 1) * SCREEN_WIDTH;
    } else if (c == '\b') {
        if (cursor_pos > 0) {
            cursor_pos--;
            vga_putc_at(' ', VGA_ATTR_WHITE_ON_BLACK, cursor_pos % SCREEN_WIDTH, cursor_pos / SCREEN_WIDTH);
        }
    } else {
        vga_putc_at(c, VGA_ATTR_WHITE_ON_BLACK, cursor_pos % SCREEN_WIDTH, cursor_pos / SCREEN_WIDTH);
        cursor_pos++;
    }

    while (cursor_pos >= SCREEN_CELLS) terminal_scroll();
}

void terminal_writestring(const char* s) {
    while (*s) terminal_putchar(*s++);
}

// ============================================================
// STRING / NUM UTILS
// ============================================================
size_t strlen(const char* s) { size_t l=0; while(s[l]) l++; return l; }

void itoa(int val, char* buf, int base) {
    char tmp[32]; int i=0, neg=0;
    if (val == 0) { buf[0]='0'; buf[1]=0; return; }
    if (val < 0 && base==10) { neg=1; val=-val; }
    while (val) { int r=val%base; tmp[i++]=(r>9)?'A'+r-10:'0'+r; val/=base; }
    if (neg) tmp[i++]='-';
    for (int j=0; j<i; j++) buf[j]=tmp[i-1-j];
    buf[i]=0;
}

// ============================================================
// RPN ENGINE
// ============================================================
static void rpn_push(int v) {
    if (rpn_top < STACK_SIZE) rpn_stack[rpn_top++] = v;
    else rpn_error = 1;
}

static int rpn_pop(void) {
    if (rpn_top > 0) return rpn_stack[--rpn_top];
    rpn_error = 2; return 0;
}

void run_rpn(const char* input) {
    rpn_top = 0; rpn_error = 0;
    int current_num = 0;
    bool in_number = false;

    for (size_t i = 0; input[i]; i++) {
        char c = input[i];

        if (c >= '0' && c <= '9') {
            current_num = current_num * 10 + (c - '0');
            in_number = true;
        } else {
            // Если число закончилось (пробел, оператор, конец строки) — пушим
            if (in_number) {
                rpn_push(current_num);
                if (rpn_error) { terminal_writestring("Error: Stack Overflow\n"); return; }
                current_num = 0;
                in_number = false;
            }

            if (c == ' ' || c == '\t') continue; // Пропускаем разделители

            if (c == '+' || c == '-' || c == '*' || c == '/') {
                int b = rpn_pop();
                int a = rpn_pop();
                if (rpn_error) { terminal_writestring("Error: Stack Underflow\n"); return; }

                switch (c) {
                    case '+': rpn_push(a + b); break;
                    case '-': rpn_push(a - b); break;
                    case '*': rpn_push(a * b); break;
                    case '/': 
                        if (b == 0) { rpn_error = 3; terminal_writestring("Error: Div by zero\n"); return; }
                        rpn_push(a / b); 
                        break;
                }
                if (rpn_error) { terminal_writestring("Error: Stack Overflow\n"); return; }
            }
            // Игнорируем неизвестные символы молча (или можно ругаться)
        }
    }

    // Последнее число в конце строки
    if (in_number) {
        rpn_push(current_num);
        if (rpn_error) { terminal_writestring("Error: Stack Overflow\n"); return; }
    }

    // Результат
    if (rpn_error == 2) terminal_writestring("Error: Stack Underflow\n");
    else if (rpn_top == 1) {
        terminal_writestring("Result: ");
        char buf[32]; itoa(rpn_pop(), buf, 10);
        terminal_writestring(buf);
        terminal_writestring("\n");
    } else if (rpn_top > 1) terminal_writestring("Error: Too many operands\n");
    else terminal_writestring("Error: Empty expression\n");
}

// ============================================================
// KEYBOARD DRIVER (US Layout, Set 1) с поддержкой SHIFT
// ============================================================
// Маппинг для нажатых клавиш (Make codes < 0x80)
static const char keymap_normal[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};

static const char keymap_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' '
};

static bool shift_pressed = false;

char keyboard_getchar(void) {
    // Ждем данные в буфере контроллера (порт 0x64, бит 0)
    while ((inb(0x64) & 1) == 0);
    uint8_t sc = inb(0x60);

    // Break code (отпускание) — старший бит 1
    if (sc & 0x80) {
        uint8_t make = sc & 0x7F;
        // Отслеживаем только Shift'ы (Left=0x2A, Right=0x36)
        if (make == 0x2A || make == 0x36) shift_pressed = false;
        return 0;
    }

    // Make code (нажатие)
    if (sc == 0x2A || sc == 0x36) { shift_pressed = true; return 0; }
    if (sc >= 128) return 0; // Extended keys (E0 prefix) игнорируем для простоты

    return shift_pressed ? keymap_shift[sc] : keymap_normal[sc];
}

// ============================================================
// KERNEL ENTRY
// ============================================================
void _start(void) {
    // 1. Init Screen
    cursor_pos = 0;
    for (size_t i = 0; i < SCREEN_CELLS; i++) VIDEO_MEMORY[i] = (VGA_ATTR_WHITE_ON_BLACK << 8) | ' ';

    terminal_writestring("-- Ocean Kernel RPN Calculator --\n");
    terminal_writestring("Enter expression (e.g. 12 34 + *). Spaces required between numbers.\n");
    terminal_writestring("> ");

    // 2. Input Loop
    char input_buf[INPUT_BUF_SIZE];
    size_t buf_len = 0;

    while (1) {
        char c = keyboard_getchar();
        if (!c) continue; // Shift, Ctrl, Alt, Break codes -> ignore

        if (c == '\n') {
            terminal_putchar('\n');
            input_buf[buf_len] = '\0';
            run_rpn(input_buf);
            terminal_writestring("> ");
            buf_len = 0;
        } else if (c == '\b') {
            if (buf_len > 0) {
                buf_len--;
                terminal_putchar('\b');
            }
        } else if (buf_len < INPUT_BUF_SIZE - 1) {
            input_buf[buf_len++] = c;
            terminal_putchar(c);
        }
    }
}
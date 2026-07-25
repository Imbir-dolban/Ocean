// kernel.c
#include <stdint.h>
#include <stddef.h>

// ============================================================
// КОНСТАНТЫ И ТИПЫ
// ============================================================
#define VIDEO_MEMORY ((volatile uint16_t*)0xB8000)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

#define MAX_FILES 10
#define MAX_FILENAME 16
#define MAX_FILE_SIZE 128
#define INPUT_BUF_SIZE 64
#define CMD_MAX_LEN 16
#define RPN_STACK_SIZE 100

#define VGA_COLOR_WHITE_ON_BLACK 0x07

// ============================================================
// STRING UTILS (Объявлены первыми, чтобы их видели все функции ниже)
// ============================================================
size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n-- && *s1 && (*s1 == *s2)) { s1++; s2++; }
    return (n == (size_t)-1) ? 0 : (*(const unsigned char*)s1 - *(const unsigned char*)s2);
}

void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

void strncpy_safe(char* dest, const char* src, size_t dest_size) {
    size_t i = 0;
    for (; i < dest_size - 1 && src[i]; i++) dest[i] = src[i];
    dest[i] = '\0';
}

void itoa(int num, char* buf, int base) {
    char tmp[32];
    int i = 0;
    unsigned int uval;

    if (num == 0) { buf[0] = '0'; buf[1] = '\0'; return; }

    if (num < 0 && base == 10) {
        buf[0] = '-';
        buf++;
        uval = (unsigned int)(-(long)num);
    } else {
        uval = (unsigned int)num;
    }

    while (uval > 0) {
        int rem = uval % base;
        tmp[i++] = (rem > 9) ? (rem - 10 + 'A') : (rem + '0');
        uval /= base;
    }

    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

// ============================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================
static size_t cursor_pos = 0; // Позиция в ячейках (0..SCREEN_SIZE-1)

typedef struct {
    char name[MAX_FILENAME];
    char content[MAX_FILE_SIZE];
    size_t size;
    int used;
} File;

static File files[MAX_FILES];

static int rpn_stack[RPN_STACK_SIZE];
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

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

// ============================================================
// VGA TEXT MODE DRIVER
// ============================================================
static void vga_put_entry_at(char c, uint8_t color, size_t x, size_t y) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    VIDEO_MEMORY[y * SCREEN_WIDTH + x] = ((uint16_t)color << 8) | (uint8_t)c;
}

static void vga_clear_line(size_t y) {
    for (size_t x = 0; x < SCREEN_WIDTH; x++) {
        vga_put_entry_at(' ', VGA_COLOR_WHITE_ON_BLACK, x, y);
    }
}

void terminal_scroll(void) {
    for (size_t y = 0; y < SCREEN_HEIGHT - 1; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            VIDEO_MEMORY[y * SCREEN_WIDTH + x] = VIDEO_MEMORY[(y + 1) * SCREEN_WIDTH + x];
        }
    }
    vga_clear_line(SCREEN_HEIGHT - 1);
    
    if (cursor_pos >= SCREEN_WIDTH) {
        cursor_pos -= SCREEN_WIDTH;
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        cursor_pos = (cursor_pos / SCREEN_WIDTH + 1) * SCREEN_WIDTH;
    } else if (c == '\b') {
        if (cursor_pos > 0) {
            cursor_pos--;
            vga_put_entry_at(' ', VGA_COLOR_WHITE_ON_BLACK, cursor_pos % SCREEN_WIDTH, cursor_pos / SCREEN_WIDTH);
        }
    } else {
        vga_put_entry_at(c, VGA_COLOR_WHITE_ON_BLACK, cursor_pos % SCREEN_WIDTH, cursor_pos / SCREEN_WIDTH);
        cursor_pos++;
    }

    while (cursor_pos >= SCREEN_SIZE) {
        terminal_scroll();
    }
}

void terminal_write(const char* data, size_t len) {
    for (size_t i = 0; i < len; i++) terminal_putchar(data[i]);
}

void terminal_writestring(const char* str) {
    terminal_write(str, strlen(str));
}

// ============================================================
// RPN CALCULATOR
// ============================================================
void rpn_push(int val) {
    if (rpn_top < RPN_STACK_SIZE) rpn_stack[rpn_top++] = val;
    else rpn_error = 1;
}

int rpn_pop(void) {
    if (rpn_top > 0) return rpn_stack[--rpn_top];
    rpn_error = 2;
    return 0;
}

void run_rpn(const char* buf) {
    rpn_top = 0;
    rpn_error = 0;
    int current_num = 0;
    int in_number = 0;

    for (size_t i = 0; buf[i]; i++) {
        char c = buf[i];

        if (c >= '0' && c <= '9') {
            current_num = current_num * 10 + (c - '0');
            in_number = 1;
        } else {
            if (in_number) {
                rpn_push(current_num);
                if (rpn_error) { terminal_writestring("Error: Stack Overflow\n"); return; }
                current_num = 0;
                in_number = 0;
            }

            if (c == ' ' || c == '\t') continue;

            if (c == '+' || c == '-' || c == '*' || c == '/') {
                int b = rpn_pop();
                int a = rpn_pop();
                if (rpn_error) { terminal_writestring("Error: Stack Underflow\n"); return; }

                switch (c) {
                    case '+': rpn_push(a + b); break;
                    case '-': rpn_push(a - b); break;
                    case '*': rpn_push(a * b); break;
                    case '/': 
                        if (b == 0) { rpn_error = 3; terminal_writestring("Error: Division by zero\n"); return; }
                        rpn_push(a / b); 
                        break;
                }
                if (rpn_error) { terminal_writestring("Error: Stack Overflow\n"); return; }
            }
        }
    }

    if (in_number) {
        rpn_push(current_num);
        if (rpn_error) { terminal_writestring("Error: Stack Overflow\n"); return; }
    }

    if (rpn_error == 2) terminal_writestring("Error: Stack Underflow\n");
    else if (rpn_top == 1) {
        terminal_writestring("Result: ");
        char buf_num[32];
        itoa(rpn_pop(), buf_num, 10);
        terminal_writestring(buf_num);
        terminal_writestring("\n");
    } else if (rpn_top > 1) terminal_writestring("Error: Too many operands\n");
    else terminal_writestring("Error: Empty expression\n");
}

// ============================================================
// RAM FILE SYSTEM
// ============================================================
void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
        files[i].name[0] = '\0';
        files[i].content[0] = '\0';
    }
}

void fs_ls(void) {
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            terminal_writestring("  ");
            terminal_writestring(files[i].name);
            terminal_writestring(" (");
            char size_buf[16];
            itoa(files[i].size, size_buf, 10);
            terminal_writestring(size_buf);
            terminal_writestring(" bytes)\n");
            count++;
        }
    }
    if (count == 0) terminal_writestring("No files found.\n");
}

int fs_touch(const char* name) {
    if (strlen(name) == 0) return -3;
    
    char safe_name[MAX_FILENAME];
    strncpy_safe(safe_name, name, MAX_FILENAME);
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, safe_name) == 0) return -1;
    }
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            strcpy(files[i].name, safe_name);
            files[i].size = 0;
            files[i].content[0] = '\0';
            files[i].used = 1;
            return 0;
        }
    }
    return -2;
}

int fs_write(const char* name, const char* content) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            size_t len = strlen(content);
            if (len >= MAX_FILE_SIZE) len = MAX_FILE_SIZE - 1;
            for (size_t j = 0; j < len; j++) files[i].content[j] = content[j];
            files[i].content[len] = '\0';
            files[i].size = len;
            return 0;
        }
    }
    return -1;
}

void fs_cat(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            terminal_writestring(files[i].content);
            terminal_writestring("\n");
            return;
        }
    }
    terminal_writestring("Error: File not found.\n");
}

int fs_rm(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            files[i].used = 0;
            return 0;
        }
    }
    return -1;
}

// ============================================================
// KEYBOARD DRIVER (US Layout Scancode Set 1)
// ============================================================
static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

char keyboard_getchar(void) {
    while ((inb(0x64) & 1) == 0) {
        __asm__ __volatile__("pause");
    }
    uint8_t scancode = inb(0x60);
    
    if (scancode >= 0x80) return 0;
    
    return scancode_to_ascii[scancode];
}

// ============================================================
// SHELL
// ============================================================
void print_prompt(void) {
    terminal_writestring("> ");
}

void process_command(char* cmd_buf) {
    terminal_putchar('\n');
    
    size_t len = strlen(cmd_buf);
    if (len == 0) { print_prompt(); return; }

    size_t cmd_end = 0;
    while (cmd_buf[cmd_end] != ' ' && cmd_buf[cmd_end] != '\0') cmd_end++;

    char command[CMD_MAX_LEN];
    size_t copy_len = (cmd_end < CMD_MAX_LEN - 1) ? cmd_end : CMD_MAX_LEN - 1;
    for (size_t i = 0; i < copy_len; i++) command[i] = cmd_buf[i];
    command[copy_len] = '\0';

    char* args = &cmd_buf[cmd_end];
    while (*args == ' ') args++;

    if (strcmp(command, "help") == 0) {
        terminal_writestring("Commands:\n");
        terminal_writestring("  help                  - Show this manual\n");
        terminal_writestring("  ls                    - List files\n");
        terminal_writestring("  touch [file]          - Create empty file\n");
        terminal_writestring("  write [file] [text]   - Write text to file\n");
        terminal_writestring("  cat [file]            - Show file content\n");
        terminal_writestring("  rm [file]             - Delete file\n");
        terminal_writestring("  calc [expr]           - RPN Calculator (e.g. calc 5 6 +)\n");
        terminal_writestring("  clear                 - Clear screen\n");
    } 
    else if (strcmp(command, "clear") == 0) {
        cursor_pos = 0;
        for (size_t i = 0; i < SCREEN_SIZE; i++) VIDEO_MEMORY[i] = (VGA_COLOR_WHITE_ON_BLACK << 8) | ' ';
    } 
    else if (strcmp(command, "ls") == 0) fs_ls();
    else if (strcmp(command, "touch") == 0) {
        int res = fs_touch(args);
        if (res == -1) terminal_writestring("Error: File exists.\n");
        else if (res == -2) terminal_writestring("Error: Memory full.\n");
        else if (res == -3) terminal_writestring("Usage: touch [filename]\n");
        else terminal_writestring("File created.\n");
    } 
    else if (strcmp(command, "cat") == 0) {
        if (strlen(args) == 0) terminal_writestring("Usage: cat [filename]\n");
        else fs_cat(args);
    } 
    else if (strcmp(command, "rm") == 0) {
        if (strlen(args) == 0) terminal_writestring("Usage: rm [filename]\n");
        else {
            int res = fs_rm(args);
            if (res == -1) terminal_writestring("Error: Not found.\n");
            else terminal_writestring("File deleted.\n");
        }
    } 
    else if (strcmp(command, "write") == 0) {
        size_t name_end = 0;
        while (args[name_end] != ' ' && args[name_end] != '\0') name_end++;
        
        if (name_end == 0 || args[name_end] == '\0') {
            terminal_writestring("Usage: write [filename] [text]\n");
        } else {
            char fname[MAX_FILENAME];
            size_t i = 0;
            for (; i < name_end && i < MAX_FILENAME - 1; i++) fname[i] = args[i];
            fname[i] = '\0';

            char* text = &args[name_end];
            while (*text == ' ') text++;
            
            int res = fs_write(fname, text);
            if (res == -1) terminal_writestring("Error: File not found.\n");
            else terminal_writestring("Content written.\n");
        }
    } 
    else if (strcmp(command, "calc") == 0) {
        if (strlen(args) == 0) terminal_writestring("Usage: calc [rpn_expression]\n");
        else run_rpn(args);
    } 
    else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(command);
        terminal_writestring("\nType 'help' for info.\n");
    }

    print_prompt();
}

// ============================================================
// KERNEL ENTRY POINT
// ============================================================
void _start_c(void) {
    cursor_pos = 0;
    for (size_t i = 0; i < SCREEN_SIZE; i++) {
        VIDEO_MEMORY[i] = (VGA_COLOR_WHITE_ON_BLACK << 8) | ' ';
    }

    fs_init();

    terminal_writestring("-- Ocean Kernel Terminal OS --\n");
    terminal_writestring("Type 'help' to see available commands.\n");
    print_prompt();

    char input_buf[INPUT_BUF_SIZE];
    size_t buf_idx = 0;

    while (1) {
        char c = keyboard_getchar();
        if (c == 0) continue;

        if (c == '\n') {
            input_buf[buf_idx] = '\0';
            process_command(input_buf);
            buf_idx = 0;
        } 
        else if (c == '\b') {
            if (buf_idx > 0) {
                buf_idx--;
                terminal_putchar('\b');
            }
        } 
        else {
            if (buf_idx < INPUT_BUF_SIZE - 1) {
                input_buf[buf_idx++] = c;
                terminal_putchar(c);
            }
        }
    }
}

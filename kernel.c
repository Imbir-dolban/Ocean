#define VIDEO_MEMORY ((char*)0xB8000)

// === СТЭК ДЛЯ ОПЗ ===
int stack[100];
int top = 0;

void push(int val) {
    if (top < 100) stack[top++] = val;
}

int pop() {
    if (top > 0) return stack[--top];
    return 0;
}

// === ВЫВОД ТЕКСТА ===
int cursor_pos = 0;

void print_char(char c) {
    if (c == '\n') {
        cursor_pos = ((cursor_pos / 160) + 1) * 160;
    } 
    else if (c == '\b') { // ОБРАБОТКА СТИРАНИЯ
        if (cursor_pos >= 2) {
            cursor_pos -= 2;
            VIDEO_MEMORY[cursor_pos] = ' ';
            VIDEO_MEMORY[cursor_pos + 1] = 0x07;
        }
    } 
    else {
        VIDEO_MEMORY[cursor_pos] = c;
        VIDEO_MEMORY[cursor_pos + 1] = 0x07;
        cursor_pos += 2;
    }
}

void print_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) print_char(str[i]);
}

// Вывод чисел на экран (так как printf у нас нет)
void print_int(int num) {
    if (num == 0) {
        print_char('0');
        return;
    }
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    char buf[32];
    int i = 0;
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        print_char(buf[j]);
    }
}

// === ПАРСЕР ОПЗ ===
void run_rpn(char* buf, int len) {
    int current_num = 0;
    int in_number = 0;

    for (int i = 0; i < len; i++) {
        char c = buf[i];

        if (c >= '0' && c <= '9') {
            current_num = current_num * 10 + (c - '0');
            in_number = 1;
        } else {
            if (in_number) {
                push(current_num);
                current_num = 0;
                in_number = 0;
            }

            if (c == '+') {
                int b = pop();
                int a = pop();
                push(a + b);
            } else if (c == '-') {
                int b = pop();
                int a = pop();
                push(a - b);
            } else if (c == '*') {
                int b = pop();
                int a = pop();
                push(a * b);
            }
        }
    }
    if (in_number) {
        push(current_num);
    }

    // Выводим результат (то, что осталось на вершине стека)
    print_string("\nResult: ");
    print_int(pop());
    print_string("\n> ");
}

// === КЛАВИАТУРА ===
unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ __volatile__("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// === ТОЧКА ВХОДА ===
void _start_c() {
    cursor_pos = 0;
    for(int i = 0; i < 80 * 25 * 2; i += 2) {
        VIDEO_MEMORY[i] = ' ';
        VIDEO_MEMORY[i+1] = 0x07;
    }

    print_string("-- Ocean Kernel Calculator --\n");
    print_string("Status: Full RPN OS Mode.\n");
    print_string("> ");

    char input_buf[64];
    int buf_idx = 0;
    unsigned char last_scancode = 0;

    while(1) {
        unsigned char scancode = inb(0x60);

        if (scancode != last_scancode) {
            if (scancode < 0x80 && scancode < 128) { 
                char c = scancode_to_ascii[scancode];
                
                if (c != 0) {
                    if (c == '\n') {
                        // Нажали Enter — считаем!
                        input_buf[buf_idx] = '\0';
                        run_rpn(input_buf, buf_idx);
                        buf_idx = 0; // Очищаем буфер для новой строки
                    } 
                    else if (c == '\b') {
                        // Нажали Backspace — удаляем из буфера и экрана
                        if (buf_idx > 0) {
                            buf_idx--;
                            print_char(c);
                        }
                    } 
                    else {
                        // Обычный символ — копим в буфер и на экран
                        if (buf_idx < 63) {
                            input_buf[buf_idx++] = c;
                            print_char(c);
                        }
                    }
                }
            }
            last_scancode = scancode;
        }
    }
}

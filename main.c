#define STACK_SIZE 15
#define VIDEO_MEMORY ((char*)0xB8000)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT * 2)

int stack[STACK_SIZE];
int top = 0;
int cursor_pos = 0;
int stack_error = 0; // 0 - OK, 1 - Overflow, 2 - Underflow

// === СДВИГ ЭКРАНА (СКРОЛЛИНГ) ===
void scroll() {
    // Сдвигаем экранную память на одну строку вверх (80 символов * 2 байта = 160 байт)
    for (int i = 0; i < SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * 2; i++) {
        VIDEO_MEMORY[i] = VIDEO_MEMORY[i + SCREEN_WIDTH * 2];
    }
    // Очищаем самую нижнюю строку экрана пробелами
    for (int i = SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * 2; i < SCREEN_SIZE; i += 2) {
        VIDEO_MEMORY[i] = ' ';
        VIDEO_MEMORY[i + 1] = 0x07;
    }
    cursor_pos -= SCREEN_WIDTH * 2;
}

// === ВЫВОД СИМВОЛА ===
void print_char(char c) {
    if (c == '\n') {
        // Перевод строки
        cursor_pos = ((cursor_pos / (SCREEN_WIDTH * 2)) + 1) * (SCREEN_WIDTH * 2);
    } 
    else if (c == '\b') {
        // Стирание предыдущего символа
        if (cursor_pos >= 2) {
            cursor_pos -= 2;
            VIDEO_MEMORY[cursor_pos] = ' ';
            VIDEO_MEMORY[cursor_pos + 1] = 0x07;
        }
    } 
    else {
        // Вывод обычного символа
        VIDEO_MEMORY[cursor_pos] = c;       
        VIDEO_MEMORY[cursor_pos + 1] = 0x07; 
        cursor_pos += 2;
    }

    // Защита от переполнения VGA-памяти
    while (cursor_pos >= SCREEN_SIZE) {
        scroll();
    }
}

// Печать строки
void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

// Печать целых чисел (необходима для вывода результатов калькулятора)
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


// === РАБОТА СО СТЕКОМ ===
void push(int value) {
    if (top >= STACK_SIZE) {
        stack_error = 1;
        print_string("E: mc went on the defensive\n");
        return;
    }
    stack[top] = value;
    top++;
}

int pop() {
    if (top == 0) {
         stack_error = 2; // Обозначаем ошибку опустошения, не выводя текст напрямую
         return 0;
    }
    top--;
    return stack[top];
}


// === ПАРСЕР ОПЗ ===
void run_rpn(char* buf, int len) {
    int current_num = 0;
    int in_number = 0;
    top = 0;           // Сброс стека
    stack_error = 0;   // Сброс ошибок

    for (int i = 0; i < len; i++) {
        char c = buf[i];

        if (c >= '0' && c <= '9') {
            current_num = current_num * 10 + (c - '0');
            in_number = 1;
        } else {
            if (in_number) {
                push(current_num);
                if (stack_error == 1) {
                    return; // Ошибка переполнения уже выведена в push()
                }
                current_num = 0;
                in_number = 0;
            }

            if (c == '+' || c == '-' || c == '*') {
                int b = pop();
                int a = pop();
                if (stack_error == 2) {
                    print_string("Error: stack is empty\n");
                    return;
                }

                if (c == '+') push(a + b);
                else if (c == '-') push(a - b);
                else if (c == '*') push(a * b);

                if (stack_error == 1) {
                    return;
                }
            }
        }
    }

    if (in_number) {
        push(current_num);
        if (stack_error == 1) {
            return;
        }
    }

    // Проверка корректности завершения выражения
    if (top == 1) {
        print_string("Result: ");
        print_int(pop());
        print_string("\n");
    } else if (top > 1) {
        print_string("Error: invalid expression (too many operands)\n");
    } else {
        if (stack_error == 2) {
            print_string("Error: stack is empty\n");
        } else {
            print_string("Error: empty expression\n");
        }
    }
}


// === ДРАЙВЕР КЛАВИАТУРЫ ===
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
void _start() {
    // Полная очистка экрана при запуске
    cursor_pos = 0;
    for (int i = 0; i < SCREEN_SIZE; i += 2) {
        VIDEO_MEMORY[i] = ' ';
        VIDEO_MEMORY[i+1] = 0x07;
    }

    print_string("-- Ocean Kernel Calculator --\n");
    print_string("Status: active.\n");
    print_string("> ");

    char input_buf[64];
    int buf_idx = 0;
    unsigned char last_scancode = 0;

    // Интерактивный цикл опроса портов клавиатуры
    while (1) {
        unsigned char scancode = inb(0x60);

        if (scancode < 0x80) { // Нажатие клавиши (Make-код)
            if (scancode != last_scancode) {
                if (scancode < 128) {
                    char c = scancode_to_ascii[scancode];
                    if (c != 0) {
                        if (c == '\n') {
                            // При нажатии Enter переходим на новую строку
                            print_char('\n');
                            
                            // Вызов парсера ОПЗ
                            input_buf[buf_idx] = '\0';
                            run_rpn(input_buf, buf_idx); 

                            print_string("> ");
                            buf_idx = 0;
                        } 
                        else if (c == '\b') {
                            if (buf_idx > 0) {
                                buf_idx--;
                                print_char(c);
                            }
                        } 
                        else {
                            if (buf_idx < 63) {
                                input_buf[buf_idx++] = c;
                                print_char(c);
                            }
                        }
                    }
                }
                last_scancode = scancode;
            }
        } else { // Отпускание клавиши (Break-код)
            unsigned char released_make = scancode & 0x7F;
            if (released_make == last_scancode) {
                last_scancode = 0; // Снимаем блокировку зажатия
            }
        }
    }
}
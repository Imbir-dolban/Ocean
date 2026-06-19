#define VIDEO_MEMORY ((char*)0xB8000)

int cursor_pos = 0;

// Функция вывода одного символа
void print_char(char c) {
    if (c == '\n') {
        cursor_pos = ((cursor_pos / 160) + 1) * 160;
    } else {
        VIDEO_MEMORY[cursor_pos] = c;
        VIDEO_MEMORY[cursor_pos + 1] = 0x07; // Белый текст на черном
        cursor_pos += 2;
    }
}

// Функция вывода строки
void print_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

// Функция для чтения байта из порта процессора
unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ __volatile__("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Таблица перевода скан-кодов в ASCII
char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// Главная точка входа ядра
void _start_c() {
    // Очищаем экран
    cursor_pos = 0;
    for(int i = 0; i < 80 * 25 * 2; i += 2) {
        VIDEO_MEMORY[i] = ' ';
        VIDEO_MEMORY[i+1] = 0x07;
    }

    print_string("-- Ocean Kernel Calculator --\n");
    print_string("Status: Keyboard test active.\n");
    print_string("Type something: ");

    unsigned char last_scancode = 0;

    // Вечный цикл опроса клавиатуры
    while(1) {
        unsigned char scancode = inb(0x60);

        if (scancode != last_scancode) {
            if (scancode < 0x80) { // Нажатие клавиши
                char c = scancode_to_ascii[scancode];
                if (c != 0) {
                    print_char(c);
                }
            }
            last_scancode = scancode;
        }
    }
}

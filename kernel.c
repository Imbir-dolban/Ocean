#define VIDEO_MEMORY ((char*)0xB8000)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

// === БАЗОВЫЕ СТРОКОВЫЕ ФУНКЦИИ (без stdlib.h) ===
int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] && s2[i]) {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        i++;
    }
    return s1[i] - s2[i];
}

void strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}


// === ВЫВОД ТЕКСТА И СКРОЛЛИНГ ===
int cursor_pos = 0;

void scroll() {
    // Сдвигаем все строки вверх на одну (80 символов * 2 байта = 160 байт)
    for (int i = 0; i < SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * 2; i++) {
        VIDEO_MEMORY[i] = VIDEO_MEMORY[i + SCREEN_WIDTH * 2];
    }
    // Очищаем последнюю строку
    for (int i = SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * 2; i < SCREEN_WIDTH * SCREEN_HEIGHT * 2; i += 2) {
        VIDEO_MEMORY[i] = ' ';
        VIDEO_MEMORY[i + 1] = 0x07;
    }
    cursor_pos -= SCREEN_WIDTH * 2;
}

void print_char(char c) {
    if (c == '\n') {
        cursor_pos = ((cursor_pos / (SCREEN_WIDTH * 2)) + 1) * (SCREEN_WIDTH * 2);
    } 
    else if (c == '\b') {
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

    // Циклическая проверка для предотвращения выхода за границы буфера
    while (cursor_pos >= SCREEN_WIDTH * SCREEN_HEIGHT * 2) {
        scroll();
    }
}

void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) print_char(str[i]);
}

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


// === СТЭК ДЛЯ ОПЗ ===
int stack[100];
int top = 0;
int rpn_error = 0; // 0 - OK, 1 - Overflow, 2 - Underflow

void push(int val) {
    if (top < 100) {
        stack[top++] = val;
    } else {
        rpn_error = 1; // Устанавливаем статус ошибки переполнения
    }
}

int pop() {
    if (top > 0) {
        return stack[--top];
    } else {
        rpn_error = 2; // Устанавливаем статус ошибки опустошения
        return 0;
    }
}


// === ПАРСЕР ОПЗ ===
void run_rpn(char* buf, int len) {
    int current_num = 0;
    int in_number = 0;
    top = 0;         // Сбрасываем стек перед вычислениями
    rpn_error = 0;   // Сбрасываем ошибки

    for (int i = 0; i < len; i++) {
        char c = buf[i];

        if (c >= '0' && c <= '9') {
            current_num = current_num * 10 + (c - '0');
            in_number = 1;
        } else {
            if (in_number) {
                push(current_num);
                if (rpn_error == 1) {
                    print_string("Error: Stack Overflow\n");
                    return;
                }
                current_num = 0;
                in_number = 0;
            }

            if (c == '+' || c == '-' || c == '*') {
                int b = pop();
                int a = pop();
                if (rpn_error == 2) {
                    print_string("Error: Stack Underflow\n");
                    return;
                }

                if (c == '+') push(a + b);
                else if (c == '-') push(a - b);
                else if (c == '*') push(a * b);

                if (rpn_error == 1) {
                    print_string("Error: Stack Overflow\n");
                    return;
                }
            }
        }
    }
    if (in_number) {
        push(current_num);
        if (rpn_error == 1) {
            print_string("Error: Stack Overflow\n");
            return;
        }
    }

    // Проверяем корректность финального состояния стека
    if (rpn_error == 2) {
        print_string("Error: Stack Underflow\n");
    } else if (top == 1) {
        print_string("Result: ");
        print_int(pop());
        print_string("\n");
    } else if (top > 1) {
        print_string("Error: Invalid Expression (Too many operands)\n");
    } else {
        print_string("Error: Empty Expression\n");
    }
}


// === ФАЙЛОВАЯ СИСТЕМА В ОЗУ (RAMFS) ===
#define MAX_FILES 10
#define MAX_FILENAME 16
#define MAX_FILE_SIZE 128

typedef struct {
    char name[MAX_FILENAME];
    char content[MAX_FILE_SIZE];
    int size;
    int used;
} File;

File files[MAX_FILES];

void fs_init() {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
        files[i].name[0] = '\0';
        files[i].content[0] = '\0';
    }
}

// Список файлов
void fs_ls() {
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            print_string("  ");
            print_string(files[i].name);
            print_string(" (");
            print_int(files[i].size);
            print_string(" bytes)\n");
            count++;
        }
    }
    if (count == 0) {
        print_string("No files found.\n");
    }
}

// Создание файла (безопасная очистка и усечение имени)
int fs_touch(const char* name) {
    if (strlen(name) == 0) return -3;
    
    // Создаем безопасную копию имени с жестким ограничением длины
    char safe_name[MAX_FILENAME];
    int idx = 0;
    while (name[idx] != '\0' && idx < MAX_FILENAME - 1) {
        safe_name[idx] = name[idx];
        idx++;
    }
    safe_name[idx] = '\0'; // Гарантируем закрывающий нуль-символ
    
    // Проверка на дубликат с использованием безопасного имени
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, safe_name) == 0) {
            return -1; 
        }
    }
    // Поиск свободного слота
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            strcpy(files[i].name, safe_name);
            files[i].size = 0;
            files[i].content[0] = '\0';
            files[i].used = 1;
            return 0;
        }
    }
    return -2; // Нет места
}

// Запись в файл
int fs_write(const char* name, const char* content) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            int len = strlen(content);
            if (len >= MAX_FILE_SIZE) {
                len = MAX_FILE_SIZE - 1;
            }
            for (int j = 0; j < len; j++) {
                files[i].content[j] = content[j];
            }
            files[i].content[len] = '\0';
            files[i].size = len;
            return 0;
        }
    }
    return -1; // Не найден
}

// Чтение файла
void fs_cat(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            print_string(files[i].content);
            print_string("\n");
            return;
        }
    }
    print_string("Error: File not found.\n");
}

// Удаление файла
int fs_rm(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            files[i].used = 0;
            return 0;
        }
    }
    return -1;
}


// === ИНТЕРПРЕТАТОР КОМАНД (ШЕЛЛ) ===
void process_command(char* cmd_buf) {
    print_string("\n"); // Переход на новую строку после ввода команды
    
    int len = strlen(cmd_buf);
    if (len == 0) {
        print_string("> ");
        return;
    }

    // Разделяем строку на команду и первый аргумент
    int cmd_end = 0;
    while (cmd_buf[cmd_end] != ' ' && cmd_buf[cmd_end] != '\0') {
        cmd_end++;
    }

    char command[16];
    int i;
    for (i = 0; i < cmd_end && i < 15; i++) {
        command[i] = cmd_buf[i];
    }
    command[i] = '\0';

    char* args = &cmd_buf[cmd_end];
    while (*args == ' ') {
        args++; // Пропускаем пробелы перед аргументами
    }

    // Обработка команд
    if (strcmp(command, "help") == 0) {
        print_string("Commands:\n");
        print_string("  help                  - Show this manual\n");
        print_string("  ls                    - List files in directory\n");
        print_string("  touch [file]          - Create an empty file\n");
        print_string("  write [file] [text]   - Write text to a file\n");
        print_string("  cat [file]            - Display file contents\n");
        print_string("  rm [file]             - Delete file\n");
        print_string("  calc [expression]     - Calculate RPN (ex: calc 5 10 +)\n");
        print_string("  clear                 - Clear screen\n");
    } 
    else if (strcmp(command, "clear") == 0) {
        cursor_pos = 0;
        for (int j = 0; j < SCREEN_WIDTH * SCREEN_HEIGHT * 2; j += 2) {
            VIDEO_MEMORY[j] = ' ';
            VIDEO_MEMORY[j + 1] = 0x07;
        }
    } 
    else if (strcmp(command, "ls") == 0) {
        fs_ls();
    } 
    else if (strcmp(command, "touch") == 0) {
        int res = fs_touch(args);
        if (res == -1) print_string("Error: File already exists.\n");
        else if (res == -2) print_string("Error: Memory is full.\n");
        else if (res == -3) print_string("Usage: touch [filename]\n");
        else print_string("File created.\n");
    } 
    else if (strcmp(command, "cat") == 0) {
        if (strlen(args) == 0) {
            print_string("Usage: cat [filename]\n");
        } else {
            fs_cat(args);
        }
    } 
    else if (strcmp(command, "rm") == 0) {
        if (strlen(args) == 0) {
            print_string("Usage: rm [filename]\n");
        } else {
            int res = fs_rm(args);
            if (res == -1) print_string("Error: File not found.\n");
            else print_string("File deleted.\n");
        }
    } 
    else if (strcmp(command, "write") == 0) {
        // Разделяем имя файла и текст
        int name_end = 0;
        while (args[name_end] != ' ' && args[name_end] != '\0') {
            name_end++;
        }

        if (name_end == 0 || args[name_end] == '\0') {
            print_string("Usage: write [filename] [text]\n");
        } else {
            char fname[MAX_FILENAME];
            int k;
            for (k = 0; k < name_end && k < MAX_FILENAME - 1; k++) {
                fname[k] = args[k];
            }
            fname[k] = '\0';

            char* text = &args[name_end];
            while (*text == ' ') text++; // Пропускаем лишние пробелы

            int res = fs_write(fname, text);
            if (res == -1) print_string("Error: File not found.\n");
            else print_string("Content written.\n");
        }
    } 
    else if (strcmp(command, "calc") == 0) {
        if (strlen(args) == 0) {
            print_string("Usage: calc [rpn_expression]\n");
        } else {
            run_rpn(args, strlen(args));
        }
    } 
    else {
        print_string("Unknown command: ");
        print_string(command);
        print_string("\nType 'help' for info.\n");
    }

    print_string("> ");
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
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 2; i += 2) {
        VIDEO_MEMORY[i] = ' ';
        VIDEO_MEMORY[i+1] = 0x07;
    }

    fs_init(); // Инициализация RAMFS

    print_string("-- Ocean Kernel Terminal OS --\n");
    print_string("Type 'help' to see available commands.\n");
    print_string("> ");

    char input_buf[64];
    int buf_idx = 0;

    while (1) {
        // Проверяем бит 0 в порту 0x64: есть ли новый скан-код в буфере?
        if (inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);

            // Обрабатываем только нажатия клавиш (Make-коды)
            if (scancode < 0x80) { 
                char c = scancode_to_ascii[scancode];

                if (c != 0) {
                    if (c == '\n') {
                        input_buf[buf_idx] = '\0';
                        process_command(input_buf);
                        buf_idx = 0; // Очищаем буфер
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
        }
    }
}
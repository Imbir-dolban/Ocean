#define STACK_SIZE 15
#define VIDEO_MEMORY ((char*)0xB8000)
#define SCREEN_WIDTH 80

int stack[STACK_SIZE];
int top = 0;
int cursor_pos = 0;

// Очистка экрана и вывод символа напрямую в видеопамять VGA
void print_char(char c) {
    if (c == '\n') {
        // Перевод строки: прыгаем на начало следующей строки экрана
        cursor_pos = ((cursor_pos / (SCREEN_WIDTH * 2)) + 1) * (SCREEN_WIDTH * 2);
        return;
    }
    
    VIDEO_MEMORY[cursor_pos] = c;       // Сам символ
    VIDEO_MEMORY[cursor_pos + 1] = 0x07; // Цвет: белый текст на черном фоне
    cursor_pos += 2;
}

// Печать целой строки
void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

void push(int value) {
    if (top >= STACK_SIZE) {
        print_string("E: mc went on the defensive\n");
        return;
    }
    stack[top] = value;
    top++;
}

int pop() {
    if (top == 0) {
         print_string("E: stack is empty\n");
         return 0;
    }
    top--;
    return stack[top];
}

//Точка входа _start
void _start_c() {
    // Очищаем экран (заполняем пробелами)
    cursor_pos = 0;
    for(int i = 0; i < 80 * 25 * 2; i += 2) {
        VIDEO_MEMORY[i] = ' ';
        VIDEO_MEMORY[i+1] = 0x07;
    }

    print_string("-- Ocean Kernel Calculator --\n");
    print_string("Status: Bare-metal active.\n");
    print_string("> ");

    // Вечный цикл, чтобы процессор не ушел в ребут
    while(1) {
        // Тут должен быть опрос портов клавиатуры, но пока просто стоим
    }
}

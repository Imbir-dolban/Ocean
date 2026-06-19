# Ocean Kernel Calculator (RPN)

Низкоуровневое ядро калькулятора на Си, работающее по принципу обратной польской записи (ОПЗ/RPN). Разрабатывается в рамках экосистемы проектов **Ocean**.

### ⚠️ Статус проекта
* **Версия:** Beta v2.0 уже вышла в .iso файл!
* **Язык интерфейса:** RU (Транслит для вывода в консоль)

### Для запуска .c файла!

### 🛠 Сборка и запуск
Для компиляции в Linux Mint (или любой другой системе с GCC) выполните в терминале:
```bash
gcc main.c -o calc
./calc
-----------------------------------------
# Ocean Kernel Calculator (RPN)

A low-level C calculator kernel using the reverse Polish notation (RPN) principle. Developed within the **Ocean** project ecosystem.

### ⚠️ Project Status
* **Version:** Beta v1.0
* **Interface Language:** RU (Transliterated for console output)

### 🚀 Kernel Features
* Uses a fixed-size stack with overflow protection.
* High calculation accuracy thanks to the transition to the `double` data type.
* Built-in division-by-zero checking (`E: na 0 / nelzua`).

### 🛠 Building and Running
To compile on Linux Mint (or any other system with GCC), run the following in a terminal:
```bash
gcc main.c -o calc
./calc

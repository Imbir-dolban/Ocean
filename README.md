# Ocean Kernel Calculator (RPN)

Низкоуровневое ядро калькулятора на Си, работающее по принципу обратной польской записи (ОПЗ/RPN). Разрабатывается в рамках экосистемы проектов **Ocean**.

### ⚠️ Статус проекта
* **Версия:** Beta v1.0
* **Язык интерфейса:** RU (Транслит для вывода в консоль)

### 🚀 Особенности ядра
* Использование стека фиксированного размера с защитой от переполнения.
* Высокая точность вычислений благодаря переходу на тип данных `double`.
* Встроенная проверка деления на ноль (`E: na 0 / nelzua`).

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
__________________________________
# OMK — Ocean Meteo Kernel v0.1

Система мониторинга погоды (барометр + датчик ветра) на языке **Forth** для микроконтроллеров ATmega328P.

## 📋 Описание
Проект предназначен для сбора данных о давлении и скорости ветра. 
Использует аппаратный I2C для связи с барометром и АЦП для замера параметров ветра.

## 🛠 Железо
* Микроконтроллер: ATmega328P (Arduino Uno/Nano/Mini)
* Барометр: подключен по I2C (адрес $EE/$EF)
* Датчик ветра: подключен к аналоговому входу

## 🚀 Как запустить
1.  Скомпилируйте файл `OMK.f` вашим Forth-компилятором.
2.  Загрузите прошивку в контроллер.
3.  После загрузки в консоли появится сообщение `OMK v0.1 LOADED`.
4.  Введите `main` для запуска цикла опроса датчиков.

## ⚖️ Лицензия
Этот проект распространяется под лицензией **GPLv3**. 
*Взял — открой код.*
---
Разработано в рамках **South Ocean Project**.  
Автор: Imbir (tg: @CeIeron)


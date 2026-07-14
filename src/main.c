#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>    
#include "cpu.h"
#include "memory.h"
#include "logger.h"
#include "tests.h"

char * os_disk_image = "rt11v400.dsk";  //файл с ОС

extern Word reg[8]; 

struct termios original_tty_settings;

void restore_terminal_atexit(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tty_settings);
}

int main (int argc, char * argv[])  {
    // 1. УСТАНОВКА ПО УМОЛЧАНИЮ: выставляем базовый уровень логирования LOG_INFO
    set_log_level(LOG_INFO);

    // 2. Обработка ключей изменения уровня логирования (если они переданы)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-logoff") == 0 || strcmp(argv[i], "-quiet") == 0) {
            set_log_level(LOG_OFF);
        }
        if (strcmp(argv[i], "-error") == 0) {
            set_log_level(LOG_ERROR);
        }
        if (strcmp(argv[i], "-info") == 0) {
            set_log_level(LOG_INFO);
        }
        if (strcmp(argv[i], "-trace") == 0) {
            set_log_level(LOG_TRACE);
        }
        if (strcmp(argv[i], "-debug") == 0) {
            set_log_level(LOG_DEBUG);
        }
    }

    // 3. Проверка режима тестирования (оставляем без изменений)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--testall") == 0) {
            run_all_tests(); 
            return 0;
        }

        if (strcmp(argv[i], "--test") == 0) {
            if (i + 1 < argc) {
                const char *arg = argv[i + 1];
                if (isdigit((unsigned char)arg[0])) { 
                    int id = atoi(arg); 
                    run_test_by_id(id); 
                } else {
                    run_test_by_name(arg); 
                }
                return 0; 
            } else {
                print_log(LOG_ERROR, "Error: The --test option requires a test name or ID argument!");
                return 1;
            }
        }
    }

    // 4. Ищем, передал ли пользователь конкретное имя файла.
    // Файл — это любой аргумент, который НЕ начинается с символа '-'
    const char * filename = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            filename = argv[i];
            break;
        }
    }

    // 5. ДИСПЕТЧЕРИЗАЦИЯ РЕЖИМОВ
    if (filename == NULL) {
        // Если имя файла НЕ указано, по умолчанию стартует операционная система RT-11!
        // Она запустится с уровнем LOG_INFO (или с тем флагом, который передал пользователь)
        boot_rt11();
        return 0;
    }

    // РЕЖИМ ЗАПУСКА ТЕКСТОВОГО ФАЙЛА (Выполняется, если имя файла передано явно)
    print_log(LOG_INFO, "==================================================");
    print_log(LOG_INFO, "        PDP-11 EMULATOR v1.0 BY ROMAN KROL        ");
    print_log(LOG_INFO, "==================================================");

    load_file(filename);
    run();
    
    return 0;
}

void boot_rt11(void) {
    const char * dsk_filename = "rt11v400.dsk"; 

    print_log(LOG_INFO, "==================================================");
    print_log(LOG_INFO, "        HARDWARE BOOT: RT-11 OPERATING SYSTEM     ");
    print_log(LOG_INFO, "==================================================");
    print_log(LOG_INFO, "Mounting OS image: %s", dsk_filename);

    // 1. Полностью сбрасываем состояние процессора в ноль
    reset_cpu_state();

    // 2. Имитируем аппаратное ПЗУ загрузки (ROM Boot) для диска RK05:
    rk11_rkda = 0000000;          // Стартуем строго с 0-го сектора диска
    rk11_rkba = 0001000;          // Буфер в ОЗУ: Кладём бут-код на адрес 001000
    rk11_rkwc = 0177400;          // Читаем ровно 256 слов (512 байт = 1 сектор)

    // Проверяем физическое наличие файла диска в корне проекта
    FILE * f = fopen(os_disk_image, "rb");
    if (f == NULL) {
        print_log(LOG_ERROR, "FATAL: Image file '%s' not found! Put it in project root.", dsk_filename);
        return;
    }
    fclose(f);

    // 3. Вызываем физическое чтение Сектора 0 контроллером RK11 в память mem[]
    // Запись бита GO в регистр управления RKCS запускает DMA-обмен
    w_write(0177404, 0000004, MEMSPACE);

    print_log(LOG_INFO, "Boot sector (Sector 0) loaded into memory successfully.");
    print_log(LOG_INFO, "Handing over control to RT-11 bootloader...");
    print_log(LOG_INFO, "--------------------------------------------------");

    // Аппаратная инициализация массива регистров по канону DEC перед стартом:
    reg[0] = 0;        // Номер системного привода (Drive 0) в R0
    reg[1] = 0177404;  // Физический адрес базового регистра управления RKCS в R1
    reg[6] = 0002000;  // Ставим стек SP (R6) чуть ниже начала кода бут-сектора

    // Переводим консоль Linux/WSL в сырой неблокирующий режим termios
    struct termios new_t;
    tcgetattr(STDIN_FILENO, &original_tty_settings); 
    atexit(restore_terminal_atexit); // Авто-восстановление консоли при выходе

    new_t = original_tty_settings;
    new_t.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &new_t);

    // 4. Запускаем процессор СТРОГО с адреса загрузки бут-сектора!
    PC = 0001000; 
    run();
}
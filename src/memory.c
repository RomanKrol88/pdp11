#include "config.h"
#include "memory.h"
#include "logger.h"
#include "cpu.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>

extern char * os_disk_image;

Byte mem[MEMSIZE];          //оперативная память

//ПЕРИФЕРИЙНЫЕ УСТРОЙСТВА ВВОДА-ВЫВОДА (MEMORY-MAPPED I/O):
//Клавиатура KL11
Byte keyboard_rcsr = 0;     //регистр состояния приемника (7-й бит — флаг Ready, 6-й бит — флаг IE)
Byte keyboard_rbuf = 0;     //регистр данных приемника (буфер хранения ASCII-кода нажатой клавиши)

//Системный таймер KW11-L
Byte timer_lks     = 0;     //регистр состояния часов (7-й бит — флаг тика LCM, 6-й бит — флаг IE)

//Дисковый контроллер RK11 (Управление дисководами RK05)
Word rk11_rkds     = 0;     //регистр состояния привода (биты-флаги готовности головок, блокировки и т.д.)
Word rk11_rker     = 0;     //регистр ошибок (биты-флаги сбоев чтения, переполнения или плохих секторов)
Word rk11_rkwc     = 0;     //счетчик слов (числовой параметр: сколько 16-битных слов нужно перенести)
Word rk11_rkba     = 0;     //адрес шины (числовой параметр: указатель на буфер адреса в ОЗУ эмулятора mem)
Word rk11_rkda     = 0;     //адрес на диске (числовые параметры: упакованные номера сектора, дорожки и диска)

//Центральный регистр управления контроллера диска RK11
// Биты 1-3 — числовой код команды (2 — чтение, 1 — запись секторов)
// 6-й бит — флаг разрешения прерываний (Interrupt Enable)
// 7-й бит — флаг готовности контроллера (Ready). Изначально равен 1 (октально 0200)
Word rk11_rkcs     = 0000200; 

static int check_keyboard(void);        //функция проверяет, нажата ли клавиша в stdin без блокировки программы
static int load_data(FILE * file);      //функция читает данные из файла и записывает в память (возвращает код ошибки или 0, если прочитано без ошибок)

void b_write (Address adr, Byte val) {
    //при обращении к ODATA выводим на экран символ
    if (adr == ODATA) {
        if (current_log_level == LOG_TRACE || current_log_level == LOG_DEBUG) {
            //режим вывода с трассировкой
            print_log(LOG_OUTPUT, "%c", val);
        } else {
            //режим вывода без трассировки
            static int is_new_line = 1;

            if (val == '\r') {
                return;
            }

            if (is_new_line && !output_print) {
                printf("[PDP11 OUTPUT] ");
                is_new_line = 0;
            }

            //перевод строки в конце ввода
            if (val == '\0' || val == '\n') {
                printf("\n");
                fflush(stdout);
                is_new_line = 1;
                return;
            }

            putchar(val);
            fflush(stdout);
        }
        return; 
    }

    //запись в регистр состояния таймера LKS
    if (adr == LKS) {
        //программа может менять только 6-й бит (разрешение прерываний)
        //7-й бит принудительно сбрасывается в 0
        timer_lks = (val & 000100); 
        return;
    }

    //изолируем случайную запись в регистр OSTAT
    if (adr == OSTAT) {
        return; 
    }

     //перехват байтовой записи в регистры диска RK11
    if (adr >= RKDS && adr <= RKDA) {
        // Запись байта в регистры ввода-вывода обычно запрещена или эквивалентна слову.
        // Перенаправляем в w_write, выравнивая адрес на четную границу слова.
        w_write(adr & ~1, (Word)val, MEMSPACE);
        return;
    }

    mem[adr] = val;
}

Byte b_read (Address adr) {
    if (adr == OSTAT) {
        return 000200; 
    }

    //чтение регистра состояния клавиатуры RCSR
    if (adr == RCSR) {
        //если бит готовности еще не взведен, проверяем реальную клавиатуру Linux
        if ((keyboard_rcsr & 000200) == 0) {
            if (check_keyboard()) {
                char c;
                if (read(STDIN_FILENO, &c, 1) > 0) {
                    keyboard_rbuf = (Byte)c;
                    keyboard_rcsr |= 000200; //взводим 7-й бит готовности (Ready = 1)
                }
            }
        }
        return keyboard_rcsr;
    }

    //чтение регистра данных клавиатуры RBUF
    if (adr == RBUF) {
        Byte val = keyboard_rbuf;
        keyboard_rcsr &= ~000200; //АППАРАТНЫЙ СБРОС: гасим Ready-флаг после чтения символа
        return val;
    }

    //чтение регистра состояния таймера LKS
    if (adr == LKS) {
        Byte val = timer_lks;
        timer_lks &= ~000200; // АППАРАТНЫЙ СБРОС: чтение регистра сбрасывает флаг готовности
        return val;
    }

    //перехват байтового чтения регистров диска RK11
    if (adr >= RKDS && adr <= RKDA) {
        Word w_val = w_read(adr & ~1);
        if (adr & 1) {
            return (Byte)((w_val >> 8) & 0xFF);
        }
        return (Byte)(w_val & 0xFF);
    }

    return mem[adr];
}

void w_write (Address adr, Word val, int space) {
    if (space == REGSPACE) {                        //проверка адресного пространства (куда писать значение)
        w_reg_write(adr, val); 
        return;
    }

    //при обращении к ODATA выводим на экран символ
    if (adr == ODATA) {
        Byte byte_val = (Byte)(val & 0xFF);
        
        if (current_log_level == LOG_TRACE || current_log_level == LOG_DEBUG) {
            print_log(LOG_OUTPUT, "%c", byte_val);
        } else {
            static int is_new_line = 1;

            if (byte_val == '\r') {
                return;
            }

            if (byte_val == '\0' || byte_val == '\n') {
                printf("\n");
                fflush(stdout);
                is_new_line = 1;
                return;
            }

            if (is_new_line && !output_print) {
                printf("[PDP11 OUTPUT] ");
                is_new_line = 0;
            }

            putchar(byte_val);
            fflush(stdout);
        }
        return;
    }

    //словесная запись в регистр таймера
    if (adr == LKS) {
        b_write(LKS, (Byte)(val & 0xFF));
        return;
    }

    //изолируем случайную запись в регистр OSTAT
    if (adr == OSTAT) {
        return;
    }

    //перехват записи в регистры диска RK11
    if (adr == RKDS) { rk11_rkds = val; return; }
    if (adr == RKER) { rk11_rker = val; return; }
    if (adr == RKWC) { rk11_rkwc = val; return; }
    if (adr == RKBA) { rk11_rkba = val; return; }
    if (adr == RKDA) { rk11_rkda = val; return; }
    
    if (adr == RKCS) {
        rk11_rkcs = val;
        //если бит Ready (0200) сброшен в 0, контроллер уходит выполнять дисковый обмен
        if ((rk11_rkcs & 0000200) == 0) {
            rk11_step(); 
        }
        return;
    }

    //оригинальные системные коды ошибок DEC
    if ((adr & 1) != 0) {
        EMULATOR_EXIT(EXIT_MEM_ALIGNMENT, "Odd word address alignment at %06o (Trap Vector 4)", adr);
    }
    if (adr >= MEMSIZE - 1) {
        EMULATOR_EXIT(EXIT_MEM_BOUNDS, "Address %06o out of memory bounds (Trap Vector 10)", adr);
    }

    // Перехват записи в системные порты конфигурации
    if (adr == 0177776 || adr == 0177570 || adr == 0172000 || adr == 0172540 || adr == 0172032 || adr == 0177546 || adr == 0177746 || adr == 0177760) {
        return; 
    }

    if (adr >= 0160000) {
        do_trap4();
        return;
    }

    mem[adr] = (Byte)(val & 0xFF);                  //младший байт (остаток от деления на 256)   
    mem[adr + 1] = (Byte)((val >> 8) & 0xFF);       //старший байт (сдвиг на 8 бит вправо)
}

Word w_read (Address a) {
    assert((a & 1) == 0);       //проверка, что адрес слова четный

    if (a == OSTAT) {
        return 0200;
    }

    //чтение клавиатуры по словесному адресу (младший байт слова совпадает с адресом регистра)
    if (a == RCSR) {
        return (Word)b_read(RCSR);
    }
    if (a == RBUF) {
        return (Word)b_read(RBUF);
    }

    //словесное чтение таймера
    if (a == LKS) {
        return (Word)b_read(LKS);
    }

    //перехват чтения регистров диска RK11
    if (a == RKDS) return rk11_rkds;
    if (a == RKER) return rk11_rker;
    if (a == RKCS) return rk11_rkcs;
    if (a == RKWC) return rk11_rkwc;
    if (a == RKBA) return rk11_rkba;
    if (a == RKDA) return rk11_rkda;

    // Перехват системных портов конфигурации оборудования PDP-11
    if (a == 0177776 || a == 0177570 || a == 0172000 || a == 0172540 || a == 0172032 || a == 0177546 || a == 0177746 || a == 0177760) {
        return 0; 
    }

    if (a >= 0160000) {
        do_trap4();
        return 0; 
    }
    
    Word w = mem[a + 1];
    w = w << 8;
    w = w | mem[a];
    return w & 0xFFFF;
}

void mem_dump(Address adr, int size) {
    print_log(LOG_INFO, "Memory dump at address %06o, size %d bytes:", adr, size);

    for (int i = 0; i < size; i += 2) { //идем только по четным адресам (i + 2)
        Address a = adr + i;
        Word w = w_read(a);
        print_log(LOG_TRACE, "%06o: %06o %04x\n", a, w, w);       //вывод по формату "адрес: восьмеричное_слово шестнадцатеричное_слово"
    }
}

static int load_data(FILE * file) {
    unsigned int block_adr;
    int n;
    
    while (fscanf(file, "%x %x", &block_adr, &n) == 2) {    //сперва читаем адрес блока и количество записываемых байт
        print_log(LOG_TRACE, "Loading block: address 0x%X (octal 0%o), size %d bytes", block_adr, block_adr, n);
        for (int i = 0; i < n; i++) {
            unsigned int byte_value;

            if (fscanf(file, "%x", &byte_value) != 1)       //читаем значение байта
                return 1;                                   //код ошибки 1: ошибка чтения

            if (byte_value > 0xFF)                          //проверяем, что читаем именно байт
                return 2;                                   //код ошибки 2: некорректные данные в файле

            if ((block_adr + i) >= MEMSIZE)                 //проверка на переполнение памяти
                return 3;                                   //код ошибки 3: адрес памяти вне допустимого значения

            b_write((Address)(block_adr + i), (Byte)(byte_value));      //записываем значение в память
        }
    }
    return 0;
}

void load_file(const char * filename) {
    FILE * file_input = fopen(filename, "r");

    print_log(LOG_INFO, "Loading data from: %s", filename);    //печатаем какой именно файл мы загрузили

    if (file_input == NULL) {      
        perror(filename);   
        EMULATOR_EXIT(EXIT_FILE_UNKNOWN, "");        
    }

    int exit_code = load_data(file_input);

    fclose(file_input);

    if (exit_code) {
        print_log(LOG_ERROR, "Error parsing file '%s'", filename);
        
        ExitCode final_code;

        switch (exit_code) {
            case 1:
                print_log(LOG_ERROR, "Unexpected end of file or data corruption");
                final_code = EXIT_FILE_CORRUPTION;
                break;
            case 2:
                print_log(LOG_ERROR, "Invalid byte value detected");
                final_code = EXIT_FILE_INVALID_VAL;
                break;
            case 3:
                print_log(LOG_ERROR, "Attempted write out of available memory bounds");
                final_code = EXIT_FILE_MEM_BOUNDS;
                break;
            default:
                print_log(LOG_ERROR, "Unknown error (code %d)", exit_code);
                final_code = EXIT_FILE_UNKNOWN;
                break;
        }

        EMULATOR_EXIT(final_code, "");
    }

    print_log(LOG_INFO, "File loaded into memory successfully");
}

static int check_keyboard(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

void rk11_step(void) {
    // 1. Извлекаем код операции из регистра управления RKCS
    // Биты 1-3 (маска 016) задают команду. Сдвигаем вправо на 1 бит.
    Word command = (rk11_rkcs >> 1) & 07;

    // Нас интересует только команда ЧТЕНИЯ СЕКТОРА (код команды равен 2)
    if (command == 2) {
        // Открываем файл-образ диска в бинарном режиме чтения по глобальному имени
        FILE * disk = fopen(os_disk_image, "rb");
        
        if (disk == NULL) {
            print_log(LOG_ERROR, ">>> RK11 ERROR: Cannot open disk image file '%s'!", os_disk_image);
            rk11_rkcs |= 0100200; 
            return;
        }

        // 2. Рассчитываем физическое смещение в файле диска
        // В оригинальном RKDA младшие 4 бита (0-3) — это номер сектора (0-11)
        Word sector = rk11_rkda & 017; 
        // Биты 4-12 задают номер цилиндра/дорожки
        Word track = (rk11_rkda >> 4) & 0377;
        
        // Линейный номер сектора на диске: (дорожка * 12 секторов на дорожке) + текущий сектор
        long sector_index = (track * 12) + sector;
        // Смещение в байтах от начала файла: каждый сектор строго по 512 байт
        long file_offset = sector_index * 512;

        // Позиционируем указатель чтения в файле образа
        if (fseek(disk, file_offset, SEEK_SET) != 0) {
            print_log(LOG_ERROR, ">>> RK11 ERROR: fseek failed to offset %ld!", file_offset);
            rk11_rkcs |= 0100200;
            fclose(disk);
            return;
        }

        print_log(LOG_TRACE, ">>> RK11 READ: Track %d, Sector %d (Offset %ld bytes)", track, sector, file_offset);

        // 3. Вычисляем реальное количество слов для переноса
        // Так как RKWC отрицательный, получаем модуль числа: ~WC + 1
        int words_to_read = 0;
        if (rk11_rkwc != 0) {
            words_to_read = (int)((~rk11_rkwc + 1) & 0xFFFF);
        }

        // 4. Пословный цикл переноса данных в ОЗУ эмулятора mem[]
        Address current_mem_addr = rk11_rkba;
        int words_transferred = 0;

        for (int i = 0; i < words_to_read; i++) {
            Word disk_word = 0;
            
            // ИСПРАВЛЕНО: Читаем строго по 2 байта (1 целое 16-битное слово) за раз!
            if (fread(&disk_word, 2, 1, disk) != 1) {
                break; 
            }

            // ПРЯМОЙ DMA-ПЕРЕНОС В ОЗУ:
            if (current_mem_addr < MEMSIZE - 1) {
                mem[current_mem_addr] = (Byte)(disk_word & 0xFF);         
                mem[current_mem_addr + 1] = (Byte)((disk_word >> 8) & 0xFF); 
            }

            current_mem_addr += 2;
            words_transferred++;
        }

        // Вышли из цикла чтения. Проверяем, почему он завершился:
        if (words_transferred < words_to_read) {
            print_log(LOG_ERROR, ">>> RK11 KАТАСТРОФА: Цикл прерван! Прочитано только %d слов из %d. Ошибка файла: %d", 
                      words_transferred, words_to_read, ferror(disk));
        } else {
            print_log(LOG_INFO, ">>> RK11 УСПЕХ: Полностью прочитано %d слов бут-сектора!", words_transferred);
        }

        fclose(disk);

        // 5. АППАРАТНОЕ ОБНОВЛЕНИЕ РЕГИСТРОВ ПОСЛЕ ОПЕРАЦИИ:
        // Счетчик слов RKWC увеличивается на количество перенесенных слов
        rk11_rkwc = (Word)((rk11_rkwc + words_transferred) & 0xFFFF);
        // Адрес шины RKBA продвигается вперед на объем перенесенных данных
        rk11_rkba = current_mem_addr;

        print_log(LOG_TRACE, ">>> RK11 SUCCESS: Transferred %d words to memory address %06o", words_transferred, rk11_rkba);
    }

    // В ЛЮБОМ СЛУЧАЕ: Возвращаем 7-й бит готовности контроллера Ready в единицу (0200)
    rk11_rkcs |= 000220; 
}

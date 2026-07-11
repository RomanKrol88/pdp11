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

Byte mem[MEMSIZE];          //оперативная память

#define OSTAT 0177564       //регистр состояния дисплея (флаг готовности)
#define ODATA 0177566       //регистр данных дисплея (ASCII-код символа)
#define RCSR  0177560       //регистр состояния приемника (клавиатуры)
#define RBUF  0177562       //регистр данных приемника (ASCII-код нажатой клавиши)

Byte keyboard_rcsr = 0;     //флаг состояния клавиатуры (взводится 7-й бит при готовности)
Byte keyboard_rbuf = 0;     //буфер хранения ASCII-кода нажатой клавиши

//функция проверяет, нажата ли клавиша в stdin без блокировки программы
static int check_keyboard(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

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

    //изолируем случайную запись в регистр OSTAT
    if (adr == OSTAT) {
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

    return mem[adr];
}

void w_write (Address adr, Word val, int space) {
    if (space == REGSPACE) {                        //проерка адресного пространства (куда писать значение)
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

    //изолируем случайную запись в регистр OSTAT
    if (adr == OSTAT) {
        return;
    }

    //оригинальные системные коды ошибок DEC
    if ((adr & 1) != 0) {
        EMULATOR_EXIT(EXIT_MEM_ALIGNMENT, "Odd word address alignment at %06o (Trap Vector 4)", adr);
    }
    if (adr >= MEMSIZE - 1) {
        EMULATOR_EXIT(EXIT_MEM_BOUNDS, "Address %06o out of memory bounds (Trap Vector 10)", adr);
    }

    mem[adr] = (Byte)(val & 0xFF);                  //младший байт (остаток от деления на 256)   
    mem[adr + 1] = (Byte)((val >> 8) & 0xFF);       //старший байт (сдвиг на 8 бит вправо)
}

Word w_read (Address a) {
    assert((a & 1) == 0);       //проверка, что адрес слова четный
    assert(a < MEMSIZE - 1);    //проверка, что адрес не выходит за границы памяти

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
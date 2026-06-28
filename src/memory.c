#include "config.h"
#include "memory.h"
#include "logger.h"
#include "cpu.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

Byte mem[MEMSIZE];          //оперативная память

#define OSTAT 0177564       //регистр состояния дисплея (флаг готовности)
#define ODATA 0177566       //регистр данных дисплея (ASCII-код символа)

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

            if (is_new_line) {
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
        return 0200; 
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
            if (is_new_line) {
                printf("[PDP11 OUTPUT] ");
                is_new_line = 0;
            }
            if (byte_val == '\0' || byte_val == '\n') {
                printf("\n");
                fflush(stdout);
                is_new_line = 1;
                return;
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

    assert((adr & 1) == 0);                         //проверка, что адрес слова четный
    assert(adr < MEMSIZE - 1);                      //проверка, что адрес не выходит за границы памяти

    mem[adr] = (Byte)(val & 0xFF);                  //младший байт (остаток от деления на 256)   
    mem[adr + 1] = (Byte)((val >> 8) & 0xFF);       //старший байт (сдвиг на 8 бит вправо)
}

Word w_read (Address a) {
    assert((a & 1) == 0);                         //проверка, что адрес слова четный
    assert(a < MEMSIZE - 1);                      //проверка, что адрес не выходит за границы памяти

    if (a == OSTAT) {
        return 0200;
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
        exit(errno);        
    }

    int exit_code = load_data(file_input);

    fclose(file_input);

    if (exit_code) {
        print_log(LOG_ERROR, "Error parsing file '%s'", filename);
        
        switch (exit_code) {
            case 1:
                print_log(LOG_ERROR, "Unexpected end of file or data corruption");
                break;
            case 2:
                print_log(LOG_ERROR, "Invalid byte value detected");
                break;
            case 3:
                print_log(LOG_ERROR, "Attempted write out of available memory bounds");
                break;
            default:
                print_log(LOG_ERROR, "Unknown error (code %d)", exit_code);
                break;
        }

        exit(exit_code);

    }

    print_log(LOG_INFO, "File loaded into memory successfully");
}
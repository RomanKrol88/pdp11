#include "memory.h"
#include "logger.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

static Byte mem[MEMSIZE];          //память (ОЗУ)
//static Word reg[8];                //дополнительная память (регистры)

static int load_data(FILE * file);      //функция читает данные из файла и записывает в память (возвращает код ошибки или 0, если прочитано без ошибок)


void b_write (Address adr, Byte val) {
    mem[adr] = val;
}

Byte b_read (Address adr) {
    return mem[adr];
}

void w_write (Address adr, Word val) {
    assert((adr & 1) == 0);                         //проверка, что адрес слова четный
    assert(adr < MEMSIZE - 1);                      //проверка, что адрес не выходит за границы памяти

    mem[adr] = (Byte)(val & 0xFF);                  //младший байт (остаток от деления на 256)   
    mem[adr + 1] = (Byte)((val >> 8) & 0xFF);       //старший байт (сдвиг на 8 бит вправо)
}

Word w_read (Address a) {
    assert((a & 1) == 0);                         //проверка, что адрес слова четный
    assert(a < MEMSIZE - 1);                      //проверка, что адрес не выходит за границы памяти

    Word w = mem[a + 1];
    w = w << 8;
    w = w | mem[a];
    return w & 0xFFFF;
}

void mem_dump(Address adr, int size) {
    print_log(LOG_INFO, "Дамп памяти по адресу %06o размером %d байт:\n", adr, size);

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

    print_log(LOG_INFO, "Загрузка данных из файла: %s\n", filename);    //печатаем какой именно файл мы загрузили

    if (file_input == NULL) {      
        perror(filename);   
        exit(errno);        
    }

    int exit_code = load_data(file_input);

    fclose(file_input);

    if (exit_code) {
        print_log(LOG_ERROR, "Ошибка при разборе файла '%s'\n", filename);
        
        switch (exit_code) {
            case 1:
                print_log(LOG_ERROR, "Неожиданный конец файла или повреждение данных\n");
                break;
            case 2:
                print_log(LOG_ERROR, "Обнаружено некорректное значение байта\n");
                break;
            case 3:
                print_log(LOG_ERROR, "Попытка записи за пределы доступной памяти\n");
                break;
            default:
                print_log(LOG_ERROR, "Неизвестная ошибка (код %d)\n", exit_code);
                break;
        }

        exit(exit_code);

    }

    print_log(LOG_INFO, "Файл успешно загружен в память\n");
}

void test_mem(void) {
    Address a;
    Byte b0, b1, bres;
    Word w, wres;
    signed char signed_b0, signed_b1, signed_bres;
    signed short signed_w;

    print_log(LOG_INFO, "Запуск проверки памяти\n");

    //пишем байт, читаем байт
    print_log(LOG_INFO, "Пишем и читаем байт по четному адресу\n");
    a = 0;
    b0 = 0x12;
    b_write(a, b0);
    bres = b_read(a);
    //тут полезно написать отладочную печать a, b0, bres
    print_log(LOG_TRACE, "a = %06o b0 = %hhx bres = %hhx\n", a, b0, bres);
    assert(b0 == bres);
    
    //пишем слово, читаем слово
    print_log(LOG_INFO, "Пишем и читаем слово\n");
    a = 2;        // другой адрес
    w = 0x3456;
    w_write(a, w);
    wres = w_read(a);
    //тут полезно написать отладочную печать a, w, wres
    print_log(LOG_TRACE, "a = %06o w = %04x wres = %04x\n", a, w, wres);
    assert(w == wres);
    
    //пишем 2 байта, читаем 1 слово
    print_log(LOG_INFO, "Пишем 2 байта, читаем слово\n");
    a = 4;        // другой адрес
    w = 0xa1b2;
    //little-endian, младшие разряды по меньшему адресу
    b0 = 0xb2;
    b1 = 0xa1;    
    b_write(a, b0);
    b_write(a+1, b1);
    wres = w_read(a);
    //тут полезно написать отладочную печать a, w, wres
    print_log(LOG_TRACE, "a = %06o b1 = %02hhx b0 = %02hhx wres = %04x\n", a, b1, b0, wres);
    assert(w == wres);

    //еще тесты:

    //чтение и запись байта по нечетному адресу
    print_log(LOG_INFO, "Пишем и читаем байт по нечетному адресу\n");
    a = 1;
    b0 = 0x78;
    b_write(a, b0);
    bres = b_read(a);
    print_log(LOG_TRACE, "a = %06o b0 = %hhx bres = %hhx\n", a, b0, bres);
    assert(b0 == bres);

    //пишем слово, читаем побайтово (проверка Little-Endian)
    print_log(LOG_INFO, "Пишем слово, читаем побайтово\n");
    a = 6;
    w = 0xCDE1;
    w_write(a, w);
    b0 = b_read(a);     // должен быть младший байт: 0xE1
    b1 = b_read(a + 1); // должен быть старший байт: 0xCD
    print_log(LOG_TRACE, "a = %06o w = %04x b1 = %02hhx b0 = %02hhx\n", a, w, b1, b0);
    assert(b0 == 0xE1);
    assert(b1 == 0xCD);

    //проверка отрицательных чисел (старший бит равен 1):

    //пишем и читаем отрицательный байт
    print_log(LOG_INFO, "Пишем и читаем отрицательный байт\n");
    a = 1; 
    signed_b0 = -123; 
    b_write(a, (Byte)signed_b0);
    signed_bres = (signed char)b_read(a);
    print_log(LOG_TRACE, "a = %06o signed_b0 = %d signed_bres = %d\n", a, signed_b0, signed_bres);
    assert(signed_b0 == signed_bres);

    //пишем отрицательное слово, читаем побайтово (проверка Little-Endian)
    print_log(LOG_INFO, "Пишем отрицательное слово, читаем побайтово\n");
    a = 10;
    signed_w = -3678;    //0xF1A2
    w_write(a, (Word)signed_w);
    signed_b0 = (signed char)b_read(a);       //должен быть младший байт: 0xA2 (-94)
    signed_b1 = (signed char)b_read(a + 1);   //должен быт старший байт: 0xF1 (-15)
    print_log(LOG_TRACE, "a = %06o signed_w = %04x signed_b1 = %d, signed_b0 = %d\n", a, (Word)signed_w, signed_b1, signed_b0);
    assert(signed_b0 == (signed char)0xA2);
    assert(signed_b1 == (signed char)0xF1);

    /*
    //тесты, вызывающие падение программы:
    print_log(LOG_INFO, "Пишем слово по нечетному адресу\n");
    w_write(1, 0x1234); 

    print_log(LOG_INFO, "Читаем слово по нечетному адресу\n");
    w_read(3);
    */

    print_log(LOG_INFO, "Проверка памяти окончена, ошибок не найдено\n");

}
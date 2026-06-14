#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#define MEMSIZE (64 * 1024)   // размер памяти 64 килобайта

typedef unsigned char Byte;
typedef unsigned short Word;
typedef Word Address;

typedef enum {      //уровни логирования
    LOG_OFF,
    LOG_ERROR, 
    LOG_INFO, 
    LOG_TRACE, 
    LOG_DEBUG
} Log_level;

static Log_level current_log_level = LOG_INFO;  //LOG_INFO - уровень логирования по умолчанию
static const char* level_strings[] = {"LOG_OFF", "LOG_ERROR", "LOG_INFO", "LOG_TRACE", "LOG_DEBUG"}; //наглядное оформление вывода логов

Byte mem[MEMSIZE];          // память

void b_write (Address adr, Byte val);                   //функция записывает значение (байт) val по адресу adr;
Byte b_read (Address adr);                              //функция считывает байт по адресу adr и возвращает его;
void w_write (Address adr, Word val);                   //функция записывает значение (слово) val по адресу adr;
Word w_read (Address adr);                              //функция считывает слово по адресу adr и возвращает его;
int load_data(FILE * file);                             //функция читает данные из файла и записывает в память (возвращает код ошибки или 0, если прочитано без ошибок)
void mem_dump(Address adr, int size);                   //функция выводит на печать size байт, начиная с адреса adr, в виде слов по формату "%06o: %06o %04x"
void load_file(const char * filename);                  //функция открывает файл, передает данные из файла в функцию чтения и закрывает файл
int set_log_level(int level);                           //функция установки порогового уровня логирования
void print_log(int level, const char* format, ...);     //функция для обработки и вывода логов

void test_mem();        //тесты работы с памятью

int main (int argc, char * argv[])  {
    const char * filename = (argc > 1) ? argv[1] : "data.txt";      //по-умолчанию используем файл data.txt
    
    set_log_level(LOG_DEBUG);

    test_mem();

    //load_data(stdin);

    load_file(filename);

    mem_dump(0x40, 20);
    printf("\n");
    mem_dump(0x200, 0x26);

    return 0;
}

void test_mem() {
    Address a;
    Byte b0, b1, bres;
    Word w, wres;

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

    print_log(LOG_INFO, "Проверка памяти окончена, ошибок не найдено\n");

}

void b_write (Address adr, Byte val) {
    mem[adr] = val;
}

Byte b_read (Address adr) {
    return mem[adr];
}

void w_write (Address adr, Word val) {
    assert((adr & 1) == 0);                         //проверка, что адрес слова четный
    mem[adr] = (Byte)(val & 0xFF);                  //младший байт (остаток от деления на 256)   
    mem[adr + 1] = (Byte)((val >> 8) & 0xFF);       //старший байт (сдвиг на 8 бит вправо)
}

Word w_read (Address a)
{
    Word w = mem[a + 1];
    w = w << 8;
    w = w | mem[a];
    return w & 0xFFFF;
}

void mem_dump(Address adr, int size) {
    for (Address a = adr; a < adr + size; a += 2) { //идем только по четным адресам (а + 2)
        Word w = w_read(a);
        print_log(LOG_TRACE, "%06o: %06o %04x\n", a, w, w);       //вывод по формату "адрес: восьмеричное_слово шестнадцатеричное_слово"
    }
}

int load_data(FILE * file) {
    
    unsigned int block_adr;
    int n;
    
    while (fscanf(file, "%x %x", &block_adr, &n) == 2) {       //сперва читаем адрес блока и количество записываемых байт
        for (int i = 0; i < n; i++) {
            unsigned int byte_value;
            if (fscanf(file, "%x", &byte_value) != 1)       //читаем значение байта
                 return 1;
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

    unsigned int exit_code = load_data(file_input);

    fclose(file_input);

    if (exit_code) {
        exit(exit_code);
    }
}

int set_log_level(int level) {
    int old_level = (int)current_log_level;
    current_log_level = (Log_level)level;
    return old_level;
}

void print_log(int level, const char* format, ...) {
    if (level > (int)current_log_level) {
        return;
    }

    printf("[%s] ", level_strings[level]);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#ifndef MEMORY_H
#define MEMOTY_H

#include "config.h"

void b_write (Address adr, Byte val);                   //функция записывает значение (байт) val по адресу adr;
Byte b_read (Address adr);                              //функция считывает байт по адресу adr и возвращает его;
void w_write (Address adr, Word val, int space);        //функция записывает значение (слово) val по адресу adr в адресное пространство (регистры или ОЗУ);
Word w_read (Address adr);                              //функция считывает слово по адресу adr и возвращает его;

void mem_dump(Address adr, int size);                   //функция выводит на печать size байт, начиная с адреса adr, в виде слов по формату "%06o: %06o %04x"
void load_file(const char * filename);                  //функция открывает файл, передает данные из файла в функцию чтения и закрывает файл
void test_mem(void);        //тесты работы с памятью

#endif
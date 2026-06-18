#ifndef CONFIG_H
#define CONFIG_H

#define MEMSIZE (64 * 1024)     //размер оперативной памяти компьютера (64 килобайта)
#define REGSIZE 8               //количество регистров процессора (сверхбыстрая память ядра процессора)

typedef unsigned char Byte;     //8-битный байт
typedef unsigned short Word;    //16-битное слово
typedef Word Address;           //16-битный адрес памяти

#endif
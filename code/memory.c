#include <stdio.h>
#include <assert.h>

#define MEMSIZE (64 * 1024)   // размер памяти 64 килобайта

typedef unsigned char Byte;
typedef unsigned short Word;
typedef Word Address;

Byte mem[MEMSIZE];          // память

void b_write (Address adr, Byte val);       //пишем значение (байт) val по адресу adr;
Byte b_read (Address adr);                  //читаем байт по адресу adr и возвращаем его;
void w_write (Address adr, Word val);       //пишем значение (слово) val по адресу adr;
Word w_read (Address adr);                  //читаем слово по адресу adr и возвращаем его;

void test_mem();        //тесты работы с памятью

int main() {

    test_mem();

    return 0;
}

void test_mem()
{
    Address a;
    Byte b0, b1, bres;
    Word w, wres;

    //пишем байт, читаем байт
    fprintf(stderr, "Пишем и читаем байт по четному адресу\n");
    a = 0;
    b0 = 0x12;
    b_write(a, b0);
    bres = b_read(a);
    //тут полезно написать отладочную печать a, b0, bres
    fprintf(stderr, "a = %06o b0 = %hhx bres = %hhx\n", a, b0, bres);
    assert(b0 == bres);
    //аналогично стоит проверить чтение и запись по нечетному адресу
    
    //пишем слово, читаем слово
    fprintf(stderr, "Пишем и читаем слово\n");
    a = 2;        // другой адрес
    w = 0x3456;
    w_write(a, w);
    wres = w_read(a);
    //тут полезно написать отладочную печать a, w, wres
    fprintf(stderr, "a = %06o w = %04x wres = %04x\n", a, w, wres);
    assert(w == wres);
    
    //пишем 2 байта, читаем 1 слово
    fprintf(stderr, "Пишем 2 байта, читаем слово\n");
    a = 4;        // другой адрес
    w = 0xa1b2;
    //little-endian, младшие разряды по меньшему адресу
    b0 = 0xb2;
    b1 = 0xa1;    
    b_write(a, b0);
    b_write(a+1, b1);
    wres = w_read(a);
    //тут полезно написать отладочную печать a, w, wres
    fprintf(stderr, "a = %06o b1 = %02hhx b0 = %02hhx wres = %04x\n", a, b1, b0, wres);
    assert(w == wres);

    //еще тесты:

    //чтение и запись байта по НЕчетному адресу
    fprintf(stderr, "Пишем и читаем байт по нечетному адресу\n");
    a = 1;
    b0 = 0x78;
    b_write(a, b0);
    bres = b_read(a);
    fprintf(stderr, "a = %06o b0 = %hhx bres = %hhx\n", a, b0, bres);
    assert(b0 == bres);

    //пишем слово, читаем побайтово (проверка Little-Endian)
    fprintf(stderr, "Пишем слово, читаем побайтово\n");
    a = 6;
    w = 0xCDE1;
    w_write(a, w);
    b0 = b_read(a);     // должен быть младший байт: 0xE1
    b1 = b_read(a + 1); // должен быть старший байт: 0xCD
    fprintf(stderr, "a = %06o w = %04x b1 = %02hhx b0 = %02hhx\n", a, w, b1, b0);
    assert(b0 == 0xE1);
    assert(b1 == 0xCD);

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

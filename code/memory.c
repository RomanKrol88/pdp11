#include <stdio.h>

typedef unsigned char Byte;
typedef unsigned short Word;
typedef Word Address;

void b_write (Address adr, Byte val);       //пишем значение (байт) val по адресу adr;
Byte b_read (Address adr);                  //читаем байт по адресу adr и возвращаем его;
void w_write (Address adr, Word val);       //пишем значение (слово) val по адресу adr;
Word w_read (Address adr);                  //читаем слово по адресу adr и возвращаем его;

int main() {

    
    return 0;
}
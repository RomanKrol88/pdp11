#ifndef CPU_H
#define CPU_H

#include "config.h"
#include "logger.h"

#define REGSPACE 1      //пространство адресов регистров
#define MEMSPACE 0      //пространство адресов оперативной памяти

typedef struct {
    Word val;       //значение
    Address adr;    //адрес
    int space;      //пространство адресов (регистры или ОЗУ)
} Arg;

typedef struct {
    Word mask;
    Word opcode;
    char * name;
    void (*do_command)(void);
} Command;

void format_arg(Arg arg, Word bits, char *out_str);     //функция вывода на печать лога
void reg_dump(void);                                    //функция дампа регистров
void run(void);                                         //функция распознавания и запуска программ
Arg get_mr(Word w);                                     //функция разбора агрумента на моду и регистр
Command parse_cmd(Word w);                              //декодер команд процессора
void w_reg_write(int r, Word val);                      //функция записи слова в регистр
void do_halt(void);                                     //функция остановки HALT (opcode = 000000)
void do_add(void);
void do_mov(void);
void do_nothing(void);

//тесты работы с командами процессора:
void test_parse_mov(void);          //тест, что мы правильно определяем команды mov, add, halt
void test_mode0(void);              //тест, что мы разобрали правильно аргументы ss и dd в mov R5, R3
void test_mov(void);                //тест, что mov и мода 0 работают верно в mov R5, R3
void test_mode1_toreg(void);        //тест на чтение, что мы разобрали правильно аргументы ss и dd в mov (R5), R3
void test_mode1_fromreg(void);      //тест на запись из регистра в память MOV R3, (R5)
void test_mode2_reg(void);          //тест на автоинкремент регистра MOV (R5)+, R3
void test_mode2_pc(void);           //тест на автоинкремент регистра R7 MOV #77, R3

#endif
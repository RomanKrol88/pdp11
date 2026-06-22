#include "config.h"
#include "cpu.h"
#include "memory.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

static Word reg[REGSIZE];           //регистры процессора (дополнительная память)
#define SP reg[6]                   //Stack Pointer (отдельно выделенный регистр R6)
#define PC reg[7]                   //Program Counter (отдельно выделенный регистр R7)

Command command[] = {                       //таблица команд
    {0170000, 0060000, "add", do_add},
    {0170000, 0010000, "mov", do_mov},
    {0170000, 0000000, "halt", do_halt},
};

#define COMMAND_COUNT (sizeof(command) / sizeof(command[0]))

static Arg ss, dd;  //переменные аргументов (ss - откуда, dd - куда)

static const Command unknown_command = {0, 0, "unknown", do_nothing}; //если функция не определена

void format_arg(Arg arg, Word bits, char *out_str) {
    int m = (bits >> 3) & 7;
    int r = bits & 7;

    switch (m) {
        case 0: sprintf(out_str, "R%d", r); break;
        case 1: sprintf(out_str, "(R%d)", r); break;
        case 2: 
            if (r == 7) sprintf(out_str, "#%o", arg.val);
            else sprintf(out_str, "(R%d)+", r);
            break;
        default: sprintf(out_str, "?"); break;
    }
}

void reg_dump() {
    print_log(LOG_TRACE, "R0:%o R1:%o R2:%o R3:%o R4:%o R5:%o R6:%o R7:%o\n", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
}

Command parse_cmd(Word w) {
    //поиск в таблице команд
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if ((w & command[i].mask) == command[i].opcode) {
            ss = get_mr(w >> 6);
            dd = get_mr(w);
            return command[i]; 
        }
    }
    
    return unknown_command;
}

void run(void) {
    // следующее слово будем читать по адресу 1000 (восьмеричное)
    PC = 01000;

    Word w;     //текущее слово, которое содержит команду
    
    while(1) {
        
        w = w_read(PC);                                 //читаем текущее слово
        Address current_pc = PC;                        //сохраняем текущее значение РС для вывода в лог
        PC += 2;                                        //PC сразу же указывает на следующее неразобранное слово
        Command cmd = parse_cmd(w);                     //декодируем считанное слово

        //если вернулась неизвестная команда, выводим ошибку
        if (strcmp(cmd.name, "unknown") == 0) {
            print_log(LOG_ERROR, "\nНеизвестная команда %06o по адресу %06o\n", w, PC - 2);
            exit(1);
        }

        //печатаем лог в стиле MACRO-11
        if (strcmp(cmd.name, "halt") == 0) {
            //для HALT выводим только имя
            print_log(LOG_TRACE, "%06o %06o: %s\n", current_pc, w, cmd.name);
        } 
        else {
            char ss_str[32] = "";
            char dd_str[32] = "";

            format_arg(ss, w >> 6, ss_str);
            format_arg(dd, w, dd_str);

            print_log(LOG_TRACE, "%06o %06o: %s %s, %s\n", current_pc, w, cmd.name, ss_str, dd_str);
        }

        //выполняем команду
        cmd.do_command();

        reg_dump();
    }
}

Arg get_mr(Word w) {
    Arg res;
    int m = (w >> 3) & 7;   //номер моды
    int r = w & 7;          //номер регистра

    switch (m) {
        //мода 0, R1
        case 0:
            res.adr = r;                //адрес - номер регистра
            res.val = reg[r];           //значение - число в регистре
            res.space = REGSPACE;       //записываем в регистр
            break;

        //мода 1, (R1)
        case 1:
            res.adr = reg[r];           //в регистре адрес
            res.val = w_read(res.adr);  //по адресу - значение
            res.space = MEMSPACE;       //записываем в память
            break;

        //мода 2, (R1)+ или #3
        case 2:
            res.adr = reg[r];           //в регистре адрес
            res.val = w_read(res.adr);  //по адресу - значение
            res.space = MEMSPACE;       //записываем в память
            reg[r] += 2;                //TODO: +1
            break;

        //мы еще не дописали другие моды
        default:
            print_log(LOG_ERROR, "Mode %d not implemented yet!\n", m);
            exit(1);
        }
    return res;
}

void w_reg_write(int r, Word val) {
    reg[r] = val;
}

void do_halt(void) {
    reg_dump();
    print_log(LOG_INFO, "THE END!!!\n");
    exit(0);
}

void do_mov(void) {
    //значение аргумента SS пишем по адресу аргумента DD
    w_write(dd.adr, ss.val, dd.space);
}
void do_add(void) {
    //сумму значений аргументов SS и DD пишем по адресу аргумента DD
    w_write(dd.adr, ss.val + dd.val, dd.space);
}

void do_nothing(void) {
    printf("unknown\n");
}

//тесты:
//тест, что мы правильно определяем команды MOV, ADD, HALT
void test_parse_mov(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);
    Command cmd = parse_cmd(0010604);
    assert(strcmp(cmd.name, "mov") == 0);
    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}
//тест, что мы разобрали правильно аргументы SS и DD в MOV R5, R3
void test_mode0(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);
    reg[3] = 12;    // dd
    reg[5] = 34;    // ss
    parse_cmd(0010503);
    assert(ss.val == 34);
    assert(ss.adr == 5);
    assert(dd.val == 12);
    assert(dd.adr == 3);
    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}
//тест, что MOV и мода 0 работают верно в mov R5, R3
 void test_mov(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);
    reg[3] = 12;    // dd
    reg[5] = 34;    // ss
    Command cmd = parse_cmd(0010503);
    cmd.do_command();
    assert(reg[3] == 34);
    assert(reg[5] == 34);
    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

// тест на чтение, что мы разобрали правильно аргументы SS и DD в MOV (R5), R3
void test_mode1_toreg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);
    // setup
    reg[3] = 12;    // dd
    reg[5] = 0200;  // ss
    w_write(0200, 34, MEMSPACE);

    Command cmd = parse_cmd(0011503);

    assert(ss.val == 34);
    assert(ss.adr == 0200);
    assert(dd.val == 12);
    assert(dd.adr == 3);

    cmd.do_command();

    assert(reg[3] == 34);
    //проверяем, что значение регистра не изменилось
    assert(reg[5] == 0200);

    //clean
    reg[3] = 0;
    reg[5] = 0;
    w_write(0200, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на запись из регистра в память MOV R3, (R5)
void test_mode1_fromreg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);
    //setup
    reg[3] = 75;    //dd
    reg[5] = 0400;  //ss
    w_write(0400, 0, MEMSPACE);

    Command cmd = parse_cmd(0010315);

    assert(ss.val == 75);
    assert(ss.adr == 3);
    assert(ss.space == REGSPACE);

    assert(dd.adr == 0400);
    assert(dd.space == MEMSPACE);

    cmd.do_command();

    //проверяем, что в ОЗУ по адресу 400 появилось число 75
    assert(w_read(0400) == 75);
    assert(reg[3] == 75);   //регистр не должен поменяться

    //clean
    reg[3] = 0;
    reg[5] = 0;
    w_write(0400, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на автоинкремент регистра MOV (R5)+, R3
void test_mode2_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;     //dd
    reg[5] = 0200;  //ss
    w_write(0200, 55, MEMSPACE);

    Command cmd = parse_cmd(0012503);

    assert(ss.val == 55);
    assert(ss.adr == 0200);
    assert(ss.space == MEMSPACE);

    assert(dd.val == 0);
    assert(dd.adr == 3);
    assert(dd.space == REGSPACE);

    cmd.do_command();

    assert(reg[3] == 55);
    assert(reg[5] == 0202); //значение в регистре R5 должно увеличиться на 2

    //clean
    reg[3] = 0;
    reg[5] = 0;
    w_write(000200, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на автоинкремент регистра R7 MOV #77, R3
void test_mode2_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;     //dd
    PC = 02000;    //ss
    w_write(002000, 77, MEMSPACE);

    Command cmd = parse_cmd(0012703);

    assert(ss.val == 77);
    assert(ss.adr == 02000);
    assert(ss.space == MEMSPACE);

    cmd.do_command();

    assert(reg[3] == 77);
    assert(PC == 02002); //PC должен увеличиться на 2

    //clean
    reg[3] = 0;
    PC = 0;
    w_write(02000, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}
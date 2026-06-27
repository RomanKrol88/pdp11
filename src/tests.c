#include "config.h"
#include "cpu.h"
#include "memory.h"
#include "logger.h"
#include "tests.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

extern Byte mem[MEMSIZE];
extern Word reg[REGSIZE];
extern Arg ss, dd;
extern int r, n, nn, xx;

typedef enum {
    TEST_MEM            =  1,
    TEST_PARSE_MOV      =  2,
    TEST_MOV            =  3,
    TEST_SOB            =  4,
    TEST_CLR            =  5,
    TEST_MODE0          =  6,
    TEST_MODE1_TOREG    =  7,
    TEST_MODE1_FROMREG  =  8,
    TEST_MODE2_REG      =  9,
    TEST_MODE2_PC       = 10,
    TEST_MODE3_REG      = 11,
    TEST_MODE3_PC       = 12,
    TEST_MODE4          = 13,
    TEST_MODE5          = 14,
    TEST_MODE6_REG      = 15,
    TEST_MODE6_PC       = 16,
    TEST_MODE7_REG      = 17,
    TEST_MODE7_PC       = 18
} TestID;

typedef struct {
    TestID id;
    const char * name;
    void (*run_func)(void);
} TestCase;

static const TestCase test_table[] = {
    {TEST_MEM,              "test_mem",                 test_mem},
    {TEST_PARSE_MOV,        "test_parse_mov",           test_parse_mov},
    {TEST_MOV,              "test_mov",                 test_mov},
    {TEST_SOB,              "test_sob",                 test_sob},
    {TEST_CLR,              "test_clr",                 test_clr},
    {TEST_MODE0,            "test_mode0",               test_mode0},
    {TEST_MODE1_TOREG,      "test_mode1_toreg",         test_mode1_toreg},
    {TEST_MODE1_FROMREG,    "test_mode1_fromreg",       test_mode1_fromreg},
    {TEST_MODE2_REG,        "test_mode2_reg",           test_mode2_reg},
    {TEST_MODE2_PC,         "test_mode2_pc",            test_mode2_pc},
    {TEST_MODE3_REG,        "test_mode3_reg",           test_mode3_reg},
    {TEST_MODE3_PC,         "test_mode3_pc",            test_mode3_pc},
    {TEST_MODE4,            "test_mode4",               test_mode4},
    {TEST_MODE5,            "test_mode5",               test_mode5},
    {TEST_MODE6_REG,        "test_mode6_reg",           test_mode6_reg},
    {TEST_MODE6_PC,         "test_mode6_pc",            test_mode6_pc},
    {TEST_MODE7_REG,        "test_mode7_reg",           test_mode7_reg},
    {TEST_MODE7_PC,         "test_mode7_pc",            test_mode7_pc},
};

#define TEST_SIZE (sizeof(test_table) / sizeof(test_table[0]))

//1. Функции запуска тестов с флагами:

void run_all_tests(void) {
    print_log(LOG_INFO, "=== STARTING GLOBAL EMULATOR TEST SUITE ===\n");


    for (size_t i = 0; i < TEST_SIZE; i++) {
        run_test_by_id(test_table[i].id);
    }

    print_log(LOG_INFO, "=== ALL TEST GROUPS COMPLETED SUCCESSFULLY ===\n");
}

void run_test_by_id(int id) {
    if (id < 1 || id >= (int)TEST_SIZE + 1) {
        print_log(LOG_ERROR, "Error: Unknown Test ID %d\n", id);
        exit(1);
    }

    print_log(LOG_INFO, "=== STARTING SINGLE TEST ID: [%d] NAME: <%s> ===\n", id, test_table[id - 1].name);

    switch ((TestID)id) {
        case TEST_MEM:              test_mem();               break;
        case TEST_PARSE_MOV:        test_parse_mov();         break;
        case TEST_MOV:              test_mov();               break;
        case TEST_SOB:              test_sob();               break;
        case TEST_CLR:              test_clr();             break;
        case TEST_MODE0:            test_mode0();             break;
        case TEST_MODE1_TOREG:      test_mode1_toreg();       break;
        case TEST_MODE1_FROMREG:    test_mode1_fromreg();     break;
        case TEST_MODE2_REG:        test_mode2_reg();         break;
        case TEST_MODE2_PC:         test_mode2_pc();          break;
        case TEST_MODE3_REG:        test_mode3_reg();         break;
        case TEST_MODE3_PC:         test_mode3_pc();          break;
        case TEST_MODE4:            test_mode4();             break;
        case TEST_MODE5:            test_mode5();             break;
        case TEST_MODE6_REG:        test_mode6_reg();         break;
        case TEST_MODE6_PC:         test_mode6_pc();          break;
        case TEST_MODE7_REG:        test_mode7_reg();         break;
        case TEST_MODE7_PC:         test_mode7_pc();          break;
    }

    print_log(LOG_INFO, "=== TEST <%s> PASSED SUCCESSFULLY ===\n", test_table[id - 1].name);
}

void run_test_by_name(const char *name) {
    for (size_t i = 0; i < TEST_SIZE; i++) {
        if (strcmp(test_table[i].name, name) == 0) {
            run_test_by_id(test_table[i].id); 
            return;
        }
    }
    print_log(LOG_ERROR, "Error: Test named '%s' not found!\n", name);
    exit(1);
}

//2. Юнит-тесты для проверки работы с оперативной памятью:

void test_mem(void) {
    Address a;
    Byte b0, b1, bres;
    Word w, wres;
    signed char signed_b0, signed_b1, signed_bres;
    signed short signed_w;

    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

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
    w_write(a, w, MEMSPACE);
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
    w_write(a, w, MEMSPACE);
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
    w_write(a, (Word)signed_w, MEMSPACE);
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

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//3. Юнит-тесты для проверки работы с командами процессора:

//тест на распознавание команды MOV, ADD, HALT
void test_parse_mov(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);
    Command cmd = parse_cmd(0010604);
    assert(strcmp(cmd.name, "mov") == 0);
    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на выполнение MOV по моде 0 в команде MOV R5, R3
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

//тест на выполнение SOB в команде SOB R1, LOOP
void test_sob(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup 1
    reg[1] = 5;       //счетчик цикла
    PC = 001020;      //SOB по адресу 1016, текущий PC = 1020

    Command cmd = parse_cmd(0077103);

    assert(r == 1);
    assert(nn == 3);

    cmd.do_command();

    assert(reg[1] == 4);
    assert(PC == 001012); 

    //setup 2
    reg[1] = 1;       //последняя итерация цикла
    PC = 001020;

    cmd.do_command();

    assert(reg[1] == 0);
    assert(PC == 001020);

    //clean
    reg[1] = 0; PC = 0;
    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на выполнение CLR в команде CLR R4
void test_clr(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[4] = 1234;

    Command cmd = parse_cmd(0005004);

    cmd.do_command();

    assert(reg[4] == 0);

    //clean еще раз=)
    reg[4] = 0;
    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//4. Юнит-тесты для проверки работы с модами адресации процессора:

//тест на чтение аргументов ss и dd в MOV R5, R3
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

//тест на чтение аргументов ss и dd в MOV (R5), R3
void test_mode1_toreg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);
    // setup
    reg[3] = 12;
    reg[5] = 0200;
    w_write(0200, 34, MEMSPACE);

    Command cmd = parse_cmd(0011503);

    assert(ss.val == 34);
    assert(ss.adr == 0200);
    assert(dd.val == 12);
    assert(dd.adr == 3);

    cmd.do_command();

    assert(reg[3] == 34);
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
    reg[3] = 75;
    reg[5] = 0400;
    w_write(0400, 0, MEMSPACE);

    Command cmd = parse_cmd(0010315);

    assert(ss.val == 75);
    assert(ss.adr == 3);
    assert(ss.space == REGSPACE);

    assert(dd.adr == 0400);
    assert(dd.space == MEMSPACE);

    cmd.do_command();

    assert(w_read(0400) == 75);
    assert(reg[3] == 75);

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
    reg[3] = 0;
    reg[5] = 0200;
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
    assert(reg[5] == 0202);

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
    reg[3] = 0;
    PC = 02000;
    w_write(002000, 77, MEMSPACE);

    Command cmd = parse_cmd(0012703);

    assert(ss.val == 77);
    assert(ss.adr == 02000);
    assert(ss.space == MEMSPACE);

    cmd.do_command();

    assert(reg[3] == 77);
    assert(PC == 02002);

    //clean
    reg[3] = 0;
    PC = 0;
    w_write(02000, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на автоинкремент косвенной моды регистра MOV @(R5)+, R3
void test_mode3_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 0200;
    w_write(0200, 0400, MEMSPACE);
    w_write(0400, 85, MEMSPACE);

    Command cmd = parse_cmd(0013503);

    assert(ss.val == 85);
    assert(ss.adr == 0400);
    assert(ss.space == MEMSPACE);

    cmd.do_command();

    assert(reg[3] == 85);
    assert(reg[5] == 0202);

    //clean
    reg[3] = 0; reg[5] = 0;
    w_write(0200, 0, MEMSPACE);
    w_write(0400, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на автоинкремент абсолютного режима PC MOV @#400, R3
void test_mode3_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;
    PC = 02000;
    w_write(02000, 0400, MEMSPACE);
    w_write(0400, 99, MEMSPACE);

    Command cmd = parse_cmd(0013703);

    assert(ss.val == 99);
    assert(ss.adr == 0400);
    assert(ss.space == MEMSPACE);

    cmd.do_command();

    assert(reg[3] == 99);
    assert(PC == 02002);

    //clean
    reg[3] = 0; PC = 0;
    w_write(02000, 0, MEMSPACE);
    w_write(0400, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на автодекремент MOV R3, -(R5)
void test_mode4(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 88;
    reg[5] = 0402;
    w_write(0400, 0, MEMSPACE);

    Command cmd = parse_cmd(0010345);

    assert(dd.adr == 0400);
    assert(dd.space == MEMSPACE);

    cmd.do_command();

    assert(w_read(0400) == 88);
    assert(reg[5] == 0400);

    //clean
    reg[3] = 0;
    reg[5] = 0;
    w_write(0400, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на автодекремент косвенной моды регистра MOV @-(R5), R3
void test_mode5(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 0402;
    
    w_write(0400, 0600, MEMSPACE); 
    w_write(0600, 95, MEMSPACE);

    Command cmd = parse_cmd(0015503);

    assert(ss.val == 95);
    assert(ss.adr == 0600);
    assert(ss.space == MEMSPACE);

    cmd.do_command();

    assert(reg[3] == 95);
    assert(reg[5] == 0400);

    //clean
    reg[3] = 0; reg[5] = 0;
    w_write(0400, 0, MEMSPACE);
    w_write(0600, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на индексную адресацию регистра MOV 4(R5), R3 
void test_mode6_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 0200;
    
    PC = 02000;
    w_write(02000, 4, MEMSPACE);
    
    w_write(0204, 66, MEMSPACE); 

    Command cmd = parse_cmd(0016503);

    assert(ss.val == 66);
    assert(ss.adr == 0204);
    assert(ss.space == MEMSPACE);
    assert(PC == 02002);

    cmd.do_command();

    assert(reg[3] == 66);
    assert(reg[5] == 0200);

    //clean
    reg[3] = 0; reg[5] = 0;
    w_write(02000, 0, MEMSPACE);
    w_write(0204, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на относительную адресацию через PC MOV 10(PC), R3
void test_mode6_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;
    PC = 02000;
    w_write(02000, 010, MEMSPACE);

    w_write(002012, 123, MEMSPACE);

    Command cmd = parse_cmd(0016703);

    assert(ss.val == 123);
    assert(ss.adr == 02012);
    assert(ss.space == MEMSPACE);
    assert(PC == 02002);

    cmd.do_command();

    assert(reg[3] == 123);

    //clean
    reg[3] = 0; PC = 0;
    w_write(02000, 0, MEMSPACE);
    w_write(02012, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на индексную косвенную адресацию регистра: MOV @4(R5), R3
void test_mode7_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 0200;
    
    PC = 02000;
    w_write(02000, 4, MEMSPACE);
    w_write(0204, 0600, MEMSPACE); 
    w_write(0600, 77, MEMSPACE); 

    Command cmd = parse_cmd(0017503);

    assert(ss.val == 77);
    assert(ss.adr == 0600);
    assert(ss.space == MEMSPACE);
    assert(PC == 02002);

    cmd.do_command();

    assert(reg[3] == 77);
    assert(reg[5] == 0200);

    //clean
    reg[3] = 0; reg[5] = 0;
    w_write(02000, 0, MEMSPACE);
    w_write(0204, 0, MEMSPACE);
    w_write(0600, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}

//тест на индексную косвенную адресацию регистра: MOV @4(R5), R3
void test_mode7_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...\n", __FUNCTION__);

    //setup
    reg[3] = 0;
    PC = 02000;
    w_write(02000, 010, MEMSPACE);
    w_write(02012, 0700, MEMSPACE);
    w_write(0700, 150, MEMSPACE);

    Command cmd = parse_cmd(0017703);

    assert(ss.val == 150);
    assert(ss.adr == 0700);
    assert(ss.space == MEMSPACE);
    assert(PC == 02002);

    cmd.do_command();

    assert(reg[3] == 150);

    //clean
    reg[3] = 0; PC = 0;
    w_write(02000, 0, MEMSPACE);
    w_write(02012, 0, MEMSPACE);
    w_write(0700, 0, MEMSPACE);

    print_log(LOG_TRACE,"Function <%s> is OK\n", __FUNCTION__);
}
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
    TEST_MODE7_PC       = 18,
    TEST_FLAGS_MOV_Z    = 19, 
    TEST_FLAGS_MOV_N    = 20,
    TEST_FLAGS_ADD_C    = 21,
    TEST_FLAGS_ADD_V    = 22,
    TEST_BR             = 23,
    TEST_BR_FORWARD     = 24,
    TEST_BR_BACKWARD    = 25,
    TEST_BRANCHES       = 26,
    TEST_TST            = 27,
    TEST_JSR_RTS        = 28,
    TEST_ASH            = 29,
    TEST_ADCB           = 30,
    TEST_ASHC           = 31,
    TEST_ASL            = 32,
    TEST_ASLB           = 33,
    TEST_ASR            = 34,
    TEST_ASRB           = 35,
    TEST_BIT_LOGIC      = 36,
    TEST_CLR_FL         = 37,
    TEST_CMPB           = 38,
    TEST_COMB           = 39,
    TEST_DECB           = 40,
    TEST_INC            = 41,
    TEST_JMP            = 42,
    TEST_NEGB           = 43,
    TEST_NOP            = 44,
    TEST_RESET          = 45,
    TEST_ROLB           = 46,
    TEST_RORB           = 47,
    TEST_SBCB           = 48,
    TEST_SET_FL         = 49,
    TEST_SUB            = 50,
    TEST_SWAB           = 51,
    TEST_SXT            = 52,
    TEST_XOR            = 53,
    TEST_MUL            = 54,
    TEST_DIV            = 55
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
    {TEST_FLAGS_MOV_Z,      "test_flags_mov_z",         test_flags_mov_zero},
    {TEST_FLAGS_MOV_N,      "test_flags_mov_n",         test_flags_mov_negative},
    {TEST_FLAGS_ADD_C,      "test_flags_add_c",         test_flags_add_carry},
    {TEST_FLAGS_ADD_V,      "test_flags_add_v",         test_flags_add_overflow},
    {TEST_BR,               "test_br",                  test_br},
    {TEST_BR_FORWARD,       "test_br_forward",          test_br_forward},
    {TEST_BR_BACKWARD,      "test_br_backward",         test_br_backward},
    {TEST_BRANCHES,         "test_branches",            test_branches},
    {TEST_TST,              "test_tst",                 test_tst},
    {TEST_JSR_RTS,          "test_jsr_rts",             test_jsr_rts},
    {TEST_ASH,              "test_ash",                 test_ash},
    {TEST_ADCB,             "test_adcb",                test_adcb},
    {TEST_ASHC,             "test_ashc",                test_ashc},
    {TEST_ASL,              "test_asl",                 test_asl},
    {TEST_ASLB,             "test_aslb",                test_aslb},
    {TEST_ASR,              "test_asr",                 test_asr},
    {TEST_ASRB,             "test_asrb",                test_asrb},
    {TEST_BIT_LOGIC,        "test_bit_logic_bytes",     test_bit_logic_bytes},
    {TEST_CLR_FL,           "test_clr_fl",              test_clear_flags},
    {TEST_CMPB,             "test_cmpb",                test_cmpb},
    {TEST_COMB,             "test_comb",                test_comb},
    {TEST_DECB,             "test_decb",                test_decb},
    {TEST_INC,              "test_inc",                 test_inc},
    {TEST_JMP,              "test_jmp",                 test_jmp},
    {TEST_NEGB,             "test_negb",                test_negb},
    {TEST_NOP,              "test_nop",                 test_nop},
    {TEST_RESET,            "test_reset",               test_reset},
    {TEST_ROLB,             "test_rolb",                test_rolb},
    {TEST_RORB,             "test_rorb",                test_rorb},
    {TEST_SBCB,             "test_sbcb",                test_sbcb},
    {TEST_SET_FL,           "test_set_fl",              test_set_flags},
    {TEST_SUB,              "test_sub",                 test_sub},
    {TEST_SWAB,             "test_swab",                test_swab},
    {TEST_SXT,              "test_sxt",                 test_sxt},
    {TEST_XOR,              "test_xor",                 test_xor},
    {TEST_MUL,              "test_mul",                 test_mul},
    {TEST_DIV,              "test_div",                 test_div},

};

#define TEST_SIZE (sizeof(test_table) / sizeof(test_table[0]))

//1. Функции запуска тестов с флагами:

void run_all_tests(void) {
    print_log(LOG_INFO, "=== STARTING GLOBAL EMULATOR TEST SUITE ===");


    for (size_t i = 0; i < TEST_SIZE; i++) {
        run_test_by_id(test_table[i].id);
    }

    print_log(LOG_INFO, "=== ALL TEST COMPLETED SUCCESSFULLY ===");
}

void run_test_by_id(int id) {
    if (id < 1 || id >= (int)TEST_SIZE + 1) {
        print_log(LOG_ERROR, "Error: Unknown Test ID %d", id);
        exit(1);
    }

    print_log(LOG_INFO, "=== STARTING SINGLE TEST ID: [%d] NAME: <%s> ===", id, test_table[id - 1].name);

    switch ((TestID)id) {
        case TEST_MEM           :   test_mem();                     break;
        case TEST_PARSE_MOV     :   test_parse_mov();               break;
        case TEST_MOV           :   test_mov();                     break;
        case TEST_SOB           :   test_sob();                     break;
        case TEST_CLR           :   test_clr();                     break;
        case TEST_MODE0         :   test_mode0();                   break;
        case TEST_MODE1_TOREG   :   test_mode1_toreg();             break;
        case TEST_MODE1_FROMREG :   test_mode1_fromreg();           break;
        case TEST_MODE2_REG     :   test_mode2_reg();               break;
        case TEST_MODE2_PC      :   test_mode2_pc();                break;
        case TEST_MODE3_REG     :   test_mode3_reg();               break;
        case TEST_MODE3_PC      :   test_mode3_pc();                break;
        case TEST_MODE4         :   test_mode4();                   break;
        case TEST_MODE5         :   test_mode5();                   break;
        case TEST_MODE6_REG     :   test_mode6_reg();               break;
        case TEST_MODE6_PC      :   test_mode6_pc();                break;
        case TEST_MODE7_REG     :   test_mode7_reg();               break;
        case TEST_MODE7_PC      :   test_mode7_pc();                break;
        case TEST_FLAGS_MOV_Z   :   test_flags_mov_zero();          break;
        case TEST_FLAGS_MOV_N   :   test_flags_mov_negative();      break;
        case TEST_FLAGS_ADD_C   :   test_flags_add_carry();         break;
        case TEST_FLAGS_ADD_V   :   test_flags_add_overflow();      break;
        case TEST_BR            :   test_br();                      break;
        case TEST_BR_FORWARD    :   test_br_forward();              break;
        case TEST_BR_BACKWARD   :   test_br_backward();             break;
        case TEST_BRANCHES      :   test_branches();                break;
        case TEST_TST           :   test_tst();                     break;
        case TEST_JSR_RTS       :   test_jsr_rts();                 break;
        case TEST_ASH           :   test_ash();                     break;
        case TEST_ADCB          :   test_adcb();                    break;
        case TEST_ASHC          :   test_ashc();                    break;
        case TEST_ASL           :   test_asl();                     break;
        case TEST_ASLB          :   test_aslb();                    break;
        case TEST_ASR           :   test_asr();                     break;
        case TEST_ASRB          :   test_asrb();                    break;
        case TEST_BIT_LOGIC     :   test_bit_logic_bytes();         break;
        case TEST_CLR_FL        :   test_clear_flags();             break;
        case TEST_CMPB          :   test_cmpb();                    break;
        case TEST_COMB          :   test_comb();                    break;
        case TEST_DECB          :   test_decb();                    break;
        case TEST_INC           :   test_inc();                     break;
        case TEST_JMP           :   test_jmp();                     break;
        case TEST_NEGB          :   test_negb();                    break;
        case TEST_NOP           :   test_nop();                     break;
        case TEST_RESET         :   test_reset();                   break;
        case TEST_ROLB          :   test_rolb();                    break;
        case TEST_RORB          :   test_rorb();                    break;
        case TEST_SBCB          :   test_sbcb();                    break;
        case TEST_SET_FL        :   test_set_flags();               break;
        case TEST_SUB           :   test_sub();                     break;
        case TEST_SWAB          :   test_swab();                    break;
        case TEST_SXT           :   test_sxt();                     break;
        case TEST_XOR           :   test_xor();                     break;
        case TEST_MUL           :   test_mul();                     break;
        case TEST_DIV           :   test_div();                     break;
    }

    print_log(LOG_INFO, "=== TEST <%s> PASSED SUCCESSFULLY ===", test_table[id - 1].name);
}

void run_test_by_name(const char *name) {
    for (size_t i = 0; i < TEST_SIZE; i++) {
        if (strcmp(test_table[i].name, name) == 0) {
            run_test_by_id(test_table[i].id); 
            return;
        }
    }
    print_log(LOG_ERROR, "Error: Test named '%s' not found!", name);
    exit(1);
}

//вспомогательная функция для сброса всех регистров и флагов процессора в исходное состояние (clear)
static void reset_cpu_state(void) {
    //обнуление регистров
    for (int i = 0; i < REGSIZE; i++) {
        reg[i] = 0;
    }
    //сброс флагов условий регистра состояния PSW
    flag_N = 0;
    flag_Z = 0;
    flag_V = 0;
    flag_C = 0;
    
    //очистка служебных переменных декодера
    byte_cmd = 0;
    r = 0;
    nn = 0;
    xx = 0;
    
    //сброс структуры операндов
    ss.val = 0; ss.adr = 0; ss.space = 0;
    dd.val = 0; dd.adr = 0; dd.space = 0;
}


//2. Юнит-тесты для проверки работы с оперативной памятью:

void test_mem(void) {
    Address a;
    Byte b0, b1, bres;
    Word w, wres;
    signed char signed_b0, signed_b1, signed_bres;
    signed short signed_w;

    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //пишем байт, читаем байт
    a = 0;
    b0 = 0x12;
    b_write(a, b0);
    bres = b_read(a);
    //тут полезно написать отладочную печать a, b0, bres
    print_log(LOG_TRACE, "a = %06o b0 = %hhx bres = %hhx", a, b0, bres);
    assert(b0 == bres);
    
    //пишем слово, читаем слово
    a = 2;        // другой адрес
    w = 0x3456;
    w_write(a, w, MEMSPACE);
    wres = w_read(a);
    //тут полезно написать отладочную печать a, w, wres
    print_log(LOG_TRACE, "a = %06o w = %04x wres = %04x", a, w, wres);
    assert(w == wres);
    
    //пишем 2 байта, читаем 1 слово
    a = 4;        // другой адрес
    w = 0xa1b2;
    //little-endian, младшие разряды по меньшему адресу
    b0 = 0xb2;
    b1 = 0xa1;    
    b_write(a, b0);
    b_write(a+1, b1);
    wres = w_read(a);
    //тут полезно написать отладочную печать a, w, wres
    print_log(LOG_TRACE, "a = %06o b1 = %02hhx b0 = %02hhx wres = %04x", a, b1, b0, wres);
    assert(w == wres);

    //еще тесты:

    //чтение и запись байта по нечетному адресу
    a = 1;
    b0 = 0x78;
    b_write(a, b0);
    bres = b_read(a);
    print_log(LOG_TRACE, "a = %06o b0 = %hhx bres = %hhx", a, b0, bres);
    assert(b0 == bres);

    //пишем слово, читаем побайтово (проверка Little-Endian)
    a = 6;
    w = 0xCDE1;
    w_write(a, w, MEMSPACE);
    b0 = b_read(a);     // должен быть младший байт: 0xE1
    b1 = b_read(a + 1); // должен быть старший байт: 0xCD
    print_log(LOG_TRACE, "a = %06o w = %04x b1 = %02hhx b0 = %02hhx", a, w, b1, b0);
    assert(b0 == 0xE1);
    assert(b1 == 0xCD);

    //проверка отрицательных чисел (старший бит равен 1):

    //пишем и читаем отрицательный байт
    a = 1; 
    signed_b0 = -123; 
    b_write(a, (Byte)signed_b0);
    signed_bres = (signed char)b_read(a);
    print_log(LOG_TRACE, "a = %06o signed_b0 = %d signed_bres = %d", a, signed_b0, signed_bres);
    assert(signed_b0 == signed_bres);

    //пишем отрицательное слово, читаем побайтово (проверка Little-Endian)
    a = 10;
    signed_w = -3678;    //0xF1A2
    w_write(a, (Word)signed_w, MEMSPACE);
    signed_b0 = (signed char)b_read(a);       //должен быть младший байт: 0xA2 (-94)
    signed_b1 = (signed char)b_read(a + 1);   //должен быт старший байт: 0xF1 (-15)
    print_log(LOG_TRACE, "a = %06o signed_w = %04x signed_b1 = %d, signed_b0 = %d", a, (Word)signed_w, signed_b1, signed_b0);
    assert(signed_b0 == (signed char)0xA2);
    assert(signed_b1 == (signed char)0xF1);

    /*
    //тесты, вызывающие падение программы:
    print_log(LOG_INFO, "Пишем слово по нечетному адресу");
    w_write(1, 0x1234); 

    print_log(LOG_INFO, "Читаем слово по нечетному адресу");
    w_read(3);
    */

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//3. Юнит-тесты для проверки работы с командами процессора:

//тест на распознавание команды MOV, ADD, HALT
void test_parse_mov(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    Command cmd = parse_cmd(0010604);

    assert(strcmp(cmd.name, "mov") == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на выполнение MOV по моде 0 в команде MOV R5, R3
 void test_mov(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 12;    // dd
    reg[5] = 34;    // ss
    Command cmd = parse_cmd(0010503);

    cmd.do_command();

    assert(reg[3] == 34);
    assert(reg[5] == 34);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на выполнение SOB в команде SOB R1, LOOP
void test_sob(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на выполнение CLR в команде CLR R4
void test_clr(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[4] = 1234;

    Command cmd = parse_cmd(0005004);

    cmd.do_command();

    assert(reg[4] == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//4. Юнит-тесты для проверки работы с модами адресации процессора:

//тест на чтение аргументов ss и dd в MOV R5, R3
void test_mode0(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    reg[3] = 12;    // dd
    reg[5] = 34;    // ss

    parse_cmd(0010503);

    assert(ss.val == 34);
    assert(ss.adr == 5);
    assert(dd.val == 12);
    assert(dd.adr == 3);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на чтение аргументов ss и dd в MOV (R5), R3
void test_mode1_toreg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на запись из регистра в память MOV R3, (R5)
void test_mode1_fromreg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент регистра MOV (R5)+, R3
void test_mode2_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент регистра R7 MOV #77, R3
void test_mode2_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент косвенной моды регистра MOV @(R5)+, R3
void test_mode3_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент абсолютного режима PC MOV @#400, R3
void test_mode3_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автодекремент MOV R3, -(R5)
void test_mode4(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автодекремент косвенной моды регистра MOV @-(R5), R3
void test_mode5(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на индексную адресацию регистра MOV 4(R5), R3 
void test_mode6_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на относительную адресацию через PC MOV 10(PC), R3
void test_mode6_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на индексную косвенную адресацию регистра: MOV @4(R5), R3
void test_mode7_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на индексную косвенную адресацию регистра: MOV @4(R5), R3
void test_mode7_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

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
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест в MOV на флаг Z = 1, остальные 0
void test_flags_mov_zero(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    flag_Z = 0; flag_N = 1; flag_V = 1;
    reg[5] = 0; 

    Command cmd = parse_cmd(0010503);

    cmd.do_command();

    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест в MOV на флаг N = 1, остальные 0
void test_flags_mov_negative(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    flag_Z = 1; flag_N = 0; flag_V = 1;
    reg[5] = 0177777;

    Command cmd = parse_cmd(0010503);

    cmd.do_command();

    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    
    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест в ADD на C = 1 и Z = 1, остальные 0
void test_flags_add_carry(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    flag_N = 1; flag_Z = 0; flag_V = 1; flag_C = 0;
    reg[5] = 0177777;
    reg[3] = 01;

    Command cmd = parse_cmd(0060503);

    cmd.do_command();

    assert(reg[3] == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест в ADD на V = 1 и N = 1, остальные 0
void test_flags_add_overflow(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    flag_N = 0; flag_Z = 1; flag_V = 0; flag_C = 1;
    reg[5] = 040000;
    reg[3] = 040000;

    Command cmd = parse_cmd(0060503);
    
    cmd.do_command();

    assert(flag_N == 1);
    assert(flag_V == 1);
    assert(flag_Z == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на безусловный переход BR
void test_br(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    PC = 01002; 
    flag_N = 1; flag_Z = 0; flag_V = 1; flag_C = 0;

    Command cmd = parse_cmd(0000400);
    
    assert(strcmp(cmd.name, "br") == 0);
    assert(xx == 0); 

    cmd.do_command();

    assert(PC == 01002);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 1);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на безусловный переход вперед
void test_br_forward(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    PC = 01002;

    Command cmd = parse_cmd(0000402);
    assert(xx == 2);

    cmd.do_command();

    assert(PC == 01006);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на безусловный переход назад
void test_br_backward(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    PC = 01006;

    Command cmd = parse_cmd(0000775);
    assert(xx == -3);

    cmd.do_command();

    assert(PC == 01000);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на условный переход по флагу нуля (Z = 1)
void test_beq(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup Z = 1
    flag_Z = 1;
    PC = 01002;

    Command cmd = parse_cmd(0001402);
    cmd.do_command();

    assert(PC == 01006);

    //setup Z = 0
    flag_Z = 0;
    PC = 01002;

    cmd.do_command();

    assert(PC == 01002);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на условный переход по флагу знака (N = 0)
void test_bpl(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup N = 0
    flag_N = 0;
    PC = 01002;

    Command cmd = parse_cmd(0100002);
    cmd.do_command();

    assert(PC == 01006);

    //setup N = 1
    flag_N = 1;
    PC = 01002;

    cmd.do_command();

    assert(PC == 01002);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на условный переход по флагу нуля (Z = 0)
void test_bne(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup Z = 0
    for (int i = 0; i < 8; i++) reg[i] = 0;
    flag_Z = 0;
    PC = 01002;

    Command cmd = parse_cmd(0001002);
    cmd.do_command();

    assert(PC == 01006);

    //setup Z = 1
    flag_Z = 1;
    PC = 01002;

    cmd.do_command();

    assert(PC == 01002);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на выставление отрицательного байта N в TSTb
void test_tst(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup TST
    byte_cmd = 0;
    flag_C = 1; flag_V = 1;
    dd.val = 0100000;
    dd.adr = 1; dd.space = REGSPACE;

    do_tst();
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //setup TSTb
    byte_cmd = 1;
    flag_N = 1;
    dd.val = 0;
    dd.adr = 1; dd.space = REGSPACE;

    do_tst();
    assert(flag_Z == 1);
    assert(flag_N == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на вызов подпрограмм JSR/RTS по регистру R2
void test_jsr_rts(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    SP = 0177700; 
    reg[2] = 12;
    
    //SUBA
    PC = 024616;
    r = 2;
    dd.adr = 046444; //адрес подпрограммы SUBA
    
    do_jsr();
    
    assert(reg[2] == 024616);
    assert(w_read(SP) == 12);
    assert(PC == 046444);
    
    //SUBB
    PC = 046454;
    r = 2;
    dd.adr = 046466; //адрес подпрограммы SUBB
    
    do_jsr();
    
    assert(reg[2] == 046454);
    assert(w_read(SP) == 024616);
    assert(PC == 046466);

    //возврат из SUBB
    do_rts();
    
    assert(PC == 046454);
    assert(reg[2] == 024616);
    
    //возврат из SUBA
    r = 2;
    do_rts();
    
    assert(PC == 024616);
    assert(reg[2] == 12);
    assert(SP == 0177700);

    //clean
    reset_cpu_state();
    
    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на проверку сдвига влево и сдвига вправо командой ASH
void test_ash(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup двиг влево
    reg[2] = 4;
    r = 2;
    ss.val = 1;
    do_ash();
    assert(reg[2] == 8);
    assert(flag_Z == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //setup сдвиг вправо
    reg[2] = 0177760; 
    r = 2;
    ss.val = 076;      
    do_ash();

    assert(reg[2] == 0177774); 
    assert(flag_N == 1);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на прибалвение переноса к байту командой ADCb
void test_adcb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[4] = 5;
    byte_cmd = 1;
    flag_C = 1;
    dd.val = 5;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_adcb();

    assert(reg[4] == 6);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASHC
void test_ashc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[2] = 0; reg[3] = 0100000;
    r = 2; ss.val = 1;

    do_ashc();

    assert(reg[2] == 1);
    assert(reg[3] == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASL со словом
void test_asl(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[1] = 0100000;
    byte_cmd = 0;
    dd.val = 0100000; dd.adr = 1; dd.space = REGSPACE;

    do_asl();

    assert(reg[1] == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);
    
    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASLb с байтом
void test_aslb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[4] = 0200;
    byte_cmd = 1;
    dd.val = 0200; dd.adr = 4; dd.space = REGSPACE;

    do_aslb();

    assert((reg[4] & 0xFF) == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды R со словом
void test_asr(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[2] = 0100001;
    byte_cmd = 0;
    dd.val = 0100001; dd.adr = 2; dd.space = REGSPACE;

    do_asr();

    assert(reg[2] == 0140000);
    assert(flag_C == 1);
    assert(flag_N == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASRb с байтом
void test_asrb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[5] = 0201;
    byte_cmd = 1;
    dd.val = 0201; dd.adr = 5; dd.space = REGSPACE;

    do_asrb();

    assert((reg[5] & 0xFF) == 0300);
    assert(flag_C == 1); // Младший бит ушел в C
    assert(flag_N == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест (комплексная верификация) работы условных ветвлений
void test_branches(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup for all
    xx = 4;

    //setup BCS, BCC, BLO, BHIS
    PC = 01000; 
    flag_C = 1;

    do_bcs();

    assert(PC == 01010);

    PC = 01000; 
    flag_C = 1;

    do_bcc();

    assert(PC == 01000);

    //setup BMI, BPL, BEQ, BNE
    PC = 01000; 
    flag_N = 1;

    do_bmi(); 

    assert(PC == 01010);

    PC = 01000; 
    flag_N = 1;

    do_bpl(); 
    
    assert(PC == 01000);

    PC = 01000; 
    flag_Z = 1;

    do_beq(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_Z = 1;

    do_bne(); 
    
    assert(PC == 01000);

    //setup BHI, BLOS
    PC = 01000; 
    flag_C = 0; 
    flag_Z = 0;

    do_bhi(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_C = 1; 
    flag_Z = 0;

    do_bhi(); 
    
    assert(PC == 01000);

    PC = 01000; 
    flag_C = 1; 
    flag_Z = 0;

    do_blos(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_C = 0; 
    flag_Z = 1;

    do_blos(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_C = 0; 
    flag_Z = 0;

    do_blos(); 
    
    assert(PC == 01000);

    //setup BGE, BLT, BGT, BLE
    PC = 01000; 
    flag_N = 1; 
    flag_V = 0;

    do_blt(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_N = 1; 
    flag_V = 1;

    do_bge(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_Z = 0; 
    flag_N = 1; 
    flag_V = 1;

    do_bgt(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_Z = 1; 
    flag_N = 1; 
    flag_V = 1;

    do_bgt(); 
    
    assert(PC == 01000);

    PC = 01000; 
    flag_Z = 1; 
    flag_N = 0; 
    flag_V = 0;

    do_ble(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_Z = 0; 
    flag_N = 1; 
    flag_V = 0;

    do_ble(); 
    
    assert(PC == 01010);

    //setup BVC, BVS

    PC = 01000; 
    flag_V = 1;

    do_bvs(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_V = 0;

    do_bvc(); 
    
    assert(PC == 01010);

    PC = 01000; 
    flag_V = 1;

    do_bvc(); 
    
    assert(PC == 01000);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу байтовых логических команд BICb, BISb, BITb
void test_bit_logic_bytes(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup for all
    flag_C = 1;

    //setup BISb
    reg[1] = 0005;
    ss.val = 0120;
    dd.val = 0005; 
    dd.adr = 1; 
    dd.space = REGSPACE;
    
    do_bisb();

    assert((reg[1] & 0xFF) == 0125);
    assert(flag_C == 1);
    assert(flag_Z == 0);

    //setup BICb
    ss.val = 0005;
    dd.val = 0125; 
    dd.adr = 1; 
    dd.space = REGSPACE;
    
    do_bic();

    assert((reg[1] & 0xFF) == 0120);
    assert(flag_C == 1);
    assert(flag_Z == 0);

    //setup BITb
    ss.val = 0020;
    dd.val = 0120; 
    dd.adr = 1; 
    dd.space = REGSPACE;
    
    do_bitb();

    assert((reg[1] & 0xFF) == 0120);
    assert(flag_Z == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}


//тест на работу команд очистки флагов CLC, CLV, CLZ, CLN, CCC
void test_clear_flags(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup for all
    PC = 01002; 

    //setup CLC
    flag_C = 1; 
    flag_V = 1; 
    flag_Z = 1; 
    flag_N = 1;
    w_write(01000, 0000241, MEMSPACE);

    do_clr_fl();

    assert(flag_C == 0);
    assert(flag_V == 1); 
    assert(flag_Z == 1); 
    assert(flag_N == 1);

    //setup CLV
    flag_C = 1; 
    flag_V = 1; 
    flag_Z = 1; 
    flag_N = 1;
    w_write(01000, 0000242, MEMSPACE);

    do_clr_fl();

    assert(flag_V == 0);
    assert(flag_C == 1); 
    assert(flag_Z == 1); 
    assert(flag_N == 1);

    //setup CLZ
    flag_C = 1; 
    flag_V = 1; 
    flag_Z = 1; 
    flag_N = 1;
    w_write(01000, 0000244, MEMSPACE);

    do_clr_fl();

    assert(flag_Z == 0);
    assert(flag_C == 1); 
    assert(flag_V == 1); 
    assert(flag_N == 1);

    //setup CLN
    flag_C = 1; 
    flag_V = 1; 
    flag_Z = 1; 
    flag_N = 1;
    w_write(01000, 0000250, MEMSPACE);

    do_clr_fl();

    assert(flag_N == 0);
    assert(flag_C == 1); 
    assert(flag_V == 1); 
    assert(flag_Z == 1);

    //setup CCC
    flag_C = 1; 
    flag_V = 1; 
    flag_Z = 1; 
    flag_N = 1;
    w_write(01000, 0000257, MEMSPACE);

    do_clr_fl();

    assert(flag_C == 0);
    assert(flag_V == 0);
    assert(flag_Z == 0);
    assert(flag_N == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}


//тест на работу команды сравнения байт CMPb
void test_cmpb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup на равные байты
    reg[2] = 055;
    ss.val = 055;
    dd.val = 055; 
    dd.adr = 2; 
    dd.space = REGSPACE;

    do_cmp();

    assert(reg[2] == 055);
    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //setup в SS меньшее в DD большее
    reg[2] = 020;
    ss.val = 010;
    dd.val = 020; 
    dd.adr = 2; 
    dd.space = REGSPACE;

    do_cmp();

    assert(reg[2] == 020);
    assert(flag_Z == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}
//тест на работу команды байтовой инверсии COMb
void test_comb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0125; 
    byte_cmd = 1;
    flag_C = 0;
    flag_V = 1;
    dd.val = 0125;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_comb();

    assert((reg[3] & 0xFF) == 0252); 
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды декремента DECb
void test_decb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[4] = 0200;
    byte_cmd = 1;
    flag_C = 1;
    dd.val = 0200;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_decb();

    assert((reg[4] & 0377) == 0177); 
    assert(flag_V == 1);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды инкремента INCb
void test_inc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[4] = 0177;
    byte_cmd = 1;
    flag_C = 1;
    dd.val = 0177;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_inc();

    assert((reg[4] & 0377) == 0200);
    assert(flag_V == 1);
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды безусловного перехода JMP
void test_jmp(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    PC = 01000;
    reg[2] = 004000;
    dd.adr = 004000;
    dd.val = 0;
    dd.space = MEMSPACE;

    do_jmp();

    assert(PC == 004000);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды смены знака NEGb
void test_negb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup положительное число
    reg[3] = 004; 
    byte_cmd = 1;
    flag_C = 0;
    dd.val = 004;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_negb();

    assert(reg[3] == 0177774); 
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_C == 1);
    assert(flag_V == 0);

    //setup ноль
    reg[3] = 0; 
    byte_cmd = 1;
    flag_C = 1;
    dd.val = 0;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_negb();

    assert(reg[3] == 0); 
    assert(flag_Z == 1);
    assert(flag_N == 0); 
    assert(flag_C == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу пустой команды NOP
void test_nop(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    PC = 01002;
    reg[1] = 42;
    flag_N = 1; 
    flag_C = 1; 
    flag_Z = 0; 
    flag_V = 0;
    w_write(01000, 0000240, MEMSPACE);

    do_clr_fl();

    assert(reg[1] == 42);
    assert(flag_N == 1);
    assert(flag_C == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу сброса командой RESET
void test_reset(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    PC = 01002;
    reg[5] = 77;
    flag_Z = 1; 
    flag_V = 1; 
    flag_N = 0; 
    flag_C = 0;

    do_reset();

    assert(reg[5] == 77);
    assert(flag_Z == 1);
    assert(flag_V == 1);
    assert(flag_N == 0);
    assert(flag_C == 0);

    //clear
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу циклического сдвига влево командой ROLb
void test_rolb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0200;
    byte_cmd = 1;
    flag_C = 0;
    dd.val = 0200; 
    dd.adr = 3; 
    dd.space = REGSPACE;

    do_rolb();

    assert((reg[3] & 0377) == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу циклического сдвига вправо командой ROLb
void test_rorb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 001;
    byte_cmd = 1;
    flag_C = 1;

    dd.val = 001; 
    dd.adr = 3; 
    dd.space = REGSPACE;

    do_rorb();

    assert((reg[3] & 0377) == 0200);
    assert(flag_C == 1);
    assert(flag_N == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды байтового вычитания переноса SBCb
void test_sbcb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup обычного вычитания переноса
    reg[3] = 005;
    byte_cmd = 1;
    flag_C = 1;
    dd.val = 005;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_sbcb();

    assert(reg[3] == 004); 
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //setup вычитания из нуля
    reg[3] = 000;
    byte_cmd = 1;
    flag_C = 1;
    dd.val = 000;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_sbcb();

    assert(reg[3] == 0177777); 
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_C == 1);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команд установки флагов
void test_set_flags(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup for all
    PC = 01002; 

    //setup SEC
    flag_C = 0; 
    flag_V = 0; 
    flag_Z = 0; 
    flag_N = 0;
    w_write(01000, 0000261, MEMSPACE);

    do_set_fl();

    assert(flag_C == 1);
    assert(flag_V == 0); 
    assert(flag_Z == 0); 
    assert(flag_N == 0);

    //setup SEV
    flag_C = 0; 
    flag_V = 0; 
    flag_Z = 0; 
    flag_N = 0;
    w_write(01000, 0000262, MEMSPACE);

    do_set_fl();

    assert(flag_V == 1);
    assert(flag_C == 0); 
    assert(flag_Z == 0); 
    assert(flag_N == 0);

    //setup SEZ
    flag_C = 0; 
    flag_V = 0; 
    flag_Z = 0; 
    flag_N = 0;
    w_write(01000, 0000264, MEMSPACE);

    do_set_fl();

    assert(flag_Z == 1);
    assert(flag_C == 0); 
    assert(flag_V == 0); 
    assert(flag_N == 0);

    //setup SEN
    flag_C = 0; 
    flag_V = 0; 
    flag_Z = 0; 
    flag_N = 0;
    w_write(01000, 0000270, MEMSPACE);

    do_set_fl();

    assert(flag_N == 1);
    assert(flag_C == 0); 
    assert(flag_V == 0); 
    assert(flag_Z == 0);

    //setup SCC
    flag_C = 0; 
    flag_V = 0; 
    flag_Z = 0; 
    flag_N = 0;
    w_write(01000, 0000277, MEMSPACE);

    do_set_fl();

    assert(flag_C == 1);
    assert(flag_V == 1);
    assert(flag_Z == 1);
    assert(flag_N == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на вычитание командой SUB
void test_sub(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup обычное вычитание
    reg[2] = 12;
    byte_cmd = 0;
    flag_C = 0;
    ss.val = 5;
    dd.val = 12; 
    dd.adr = 2; 
    dd.space = REGSPACE;

    do_sub();

    assert(reg[2] == 7); 
    assert(flag_Z == 0); 
    assert(flag_N == 0);
    assert(flag_C == 0); 
    assert(flag_V == 0);

    //setup из меньшего большее
    reg[2] = 5;
    byte_cmd = 0;
    flag_C = 0;
    ss.val = 15;
    dd.val = 5; 
    dd.adr = 2; 
    dd.space = REGSPACE;

    do_sub();

    assert(reg[2] == 0177766);
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_C == 1);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на перестановку байт командой SWAb
void test_swab(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    Word test_val = 012345;
    Word expected_low = test_val & 0xFF;
    Word expected_high = (test_val >> 8) & 0xFF;
    Word expected_res = (expected_low << 8) | expected_high;

    reg[2] = test_val; 
    byte_cmd = 0;
    flag_C = 1; 
    flag_V = 1;
    dd.val = test_val;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_swab();

    assert(reg[2] == expected_res);
    assert(flag_Z == (expected_high == 0 ? 1 : 0));
    assert(flag_N == ((expected_high >> 7) & 1));
    assert(flag_V == 0); 
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на знаковое расширение флага N командой SXT
void test_sxt(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup N = 1
    reg[2] = 0012345;
    byte_cmd = 0;
    flag_N = 1;
    flag_C = 1;
    dd.val = 0;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_sxt();

    assert(reg[2] == 0177777);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_C == 1);

    //setup N = 0
    reg[2] = 0012345;
    byte_cmd = 0;
    flag_N = 0;
    flag_C = 1;
    dd.val = 0;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_sxt();

    assert(reg[2] == 0000000);
    assert(flag_N == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на исключающее ИЛИ командой XOR
void test_xor(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[2] = 0012345; 
    reg[3] = 0005252;
    byte_cmd = 0;
    r = 2;
    flag_C = 1;
    dd.val = 0005252;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_xor();

    assert(reg[3] == 0017117);
    assert(flag_Z == 0); 
    assert(flag_N == 0);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //setup XOR одинаковых чисел
    reg[2] = 0012345;
    byte_cmd = 0;
    r = 2;
    flag_C = 1;
    dd.val = 0012345;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_xor();

    assert(reg[2] == 0000000);
    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на умножение командой MUL
void test_mul(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[2] = 0000200;
    r = 2;
    ss.val = 5;

    do_mul();

    assert(reg[2] == 0000000);
    assert(reg[3] == 0001200);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на деление командой DIV
void test_div(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    reg[2] = 0000000;
    reg[3] = 0020005;
    r = 2;
    ss.val = 2;

    do_div();

    assert(reg[2] == 0010002);
    assert(reg[3] == 0000001);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

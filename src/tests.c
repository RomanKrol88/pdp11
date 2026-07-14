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
extern int r, nn, xx;
extern Byte keyboard_rcsr;
extern Byte keyboard_rbuf;
extern Word rk11_rkds;
extern Word rk11_rker;
extern Word rk11_rkcs;
extern Word rk11_rkwc;
extern Word rk11_rkba;
extern Word rk11_rkda;

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
    TEST_ADC            = 30,
    TEST_ASHC           = 31,
    TEST_ASL            = 32,
    TEST_ASLB           = 33,
    TEST_ASR            = 34,
    TEST_ASRB           = 35,
    TEST_BIT_LOGIC      = 36,
    TEST_CLR_FL         = 37,
    TEST_CMP            = 38,
    TEST_COM            = 39,
    TEST_DEC            = 40,
    TEST_INC            = 41,
    TEST_JMP            = 42,
    TEST_NEG            = 43,
    TEST_NOP            = 44,
    TEST_RESET          = 45,
    TEST_ROL            = 46,
    TEST_ROR            = 47,
    TEST_SBC            = 48,
    TEST_SET_FL         = 49,
    TEST_SUB            = 50,
    TEST_SWAB           = 51,
    TEST_SXT            = 52,
    TEST_XOR            = 53,
    TEST_MUL            = 54,
    TEST_DIV            = 55,
    TEST_KEYBOARD       = 56,
    TEST_TIMER          = 57,
    TEST_INTERRUPT      = 58,
    TEST_KEYB_INT       = 59,
    TEST_SYS_TRAPS      = 60,
    TEST_RK11           = 61
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
    {TEST_ADC,              "test_adc",                 test_adc},
    {TEST_ASHC,             "test_ashc",                test_ashc},
    {TEST_ASL,              "test_asl",                 test_asl},
    {TEST_ASLB,             "test_aslb",                test_aslb},
    {TEST_ASR,              "test_asr",                 test_asr},
    {TEST_ASRB,             "test_asrb",                test_asrb},
    {TEST_BIT_LOGIC,        "test_bit_logic_bytes",     test_bit_logic_bytes},
    {TEST_CLR_FL,           "test_clr_fl",              test_clear_flags},
    {TEST_CMP,              "test_cmp",                 test_cmp},
    {TEST_COM ,             "test_com",                 test_com},
    {TEST_DEC,              "test_dec",                 test_dec},
    {TEST_INC,              "test_inc",                 test_inc},
    {TEST_JMP,              "test_jmp",                 test_jmp},
    {TEST_NEG,              "test_neg",                 test_neg},
    {TEST_NOP,              "test_nop",                 test_nop},
    {TEST_RESET,            "test_reset",               test_reset},
    {TEST_ROL,              "test_rol",                 test_rol},
    {TEST_ROR,              "test_ror",                 test_ror},
    {TEST_SBC,              "test_sbc",                 test_sbc},
    {TEST_SET_FL,           "test_set_fl",              test_set_flags},
    {TEST_SUB,              "test_sub",                 test_sub},
    {TEST_SWAB,             "test_swab",                test_swab},
    {TEST_SXT,              "test_sxt",                 test_sxt},
    {TEST_XOR,              "test_xor",                 test_xor},
    {TEST_MUL,              "test_mul",                 test_mul},
    {TEST_DIV,              "test_div",                 test_div},
    {TEST_KEYBOARD,         "test_keyboard",            test_keyboard},
    {TEST_TIMER,            "test_timer",               test_timer},
    {TEST_INTERRUPT,        "test_interrupt",           test_interrupt},
    {TEST_KEYB_INT,         "test_keyboard_interrupt",  test_keyboard_interrupt},
    {TEST_SYS_TRAPS,        "test_sys_traps",           test_sys_traps},
    {TEST_RK11,             "test_rk11_disk",           test_rk11_disk}
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
        case TEST_ADC           :   test_adc();                     break;
        case TEST_ASHC          :   test_ashc();                    break;
        case TEST_ASL           :   test_asl();                     break;
        case TEST_ASLB          :   test_aslb();                    break;
        case TEST_ASR           :   test_asr();                     break;
        case TEST_ASRB          :   test_asrb();                    break;
        case TEST_BIT_LOGIC     :   test_bit_logic_bytes();         break;
        case TEST_CLR_FL        :   test_clear_flags();             break;
        case TEST_CMP           :   test_cmp();                     break;
        case TEST_COM           :   test_com();                     break;
        case TEST_DEC           :   test_dec();                     break;
        case TEST_INC           :   test_inc();                     break;
        case TEST_JMP           :   test_jmp();                     break;
        case TEST_NEG           :   test_neg();                     break;
        case TEST_NOP           :   test_nop();                     break;
        case TEST_RESET         :   test_reset();                   break;
        case TEST_ROL           :   test_rol();                     break;
        case TEST_ROR           :   test_ror();                     break;
        case TEST_SBC           :   test_sbc();                     break;
        case TEST_SET_FL        :   test_set_flags();               break;
        case TEST_SUB           :   test_sub();                     break;
        case TEST_SWAB          :   test_swab();                    break;
        case TEST_SXT           :   test_sxt();                     break;
        case TEST_XOR           :   test_xor();                     break;
        case TEST_MUL           :   test_mul();                     break;
        case TEST_DIV           :   test_div();                     break;
        case TEST_KEYBOARD      :   test_keyboard();                break;
        case TEST_TIMER         :   test_timer();                   break;
        case TEST_INTERRUPT     :   test_interrupt();               break;
        case TEST_KEYB_INT      :   test_keyboard_interrupt();      break;
        case TEST_SYS_TRAPS     :   test_sys_traps();               break;
        case TEST_RK11          :   test_rk11_disk();               break;
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
    memset(ss.name, 0, sizeof(ss.name));
    dd.val = 0; dd.adr = 0; dd.space = 0;
    memset(dd.name, 0, sizeof(dd.name));
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
    a = 000000;
    b0 = 0x12;
    b_write(a, b0);
    bres = b_read(a);
    //тут полезно написать отладочную печать a, b0, bres
    print_log(LOG_TRACE, "a = %06o b0 = %hhx bres = %hhx", a, b0, bres);
    assert(b0 == bres);
    
    //пишем слово, читаем слово
    a = 000002;        // другой адрес
    w = 0x3456;
    w_write(a, w, MEMSPACE);
    wres = w_read(a);
    //тут полезно написать отладочную печать a, w, wres
    print_log(LOG_TRACE, "a = %06o w = %04x wres = %04x", a, w, wres);
    assert(w == wres);
    
    //пишем 2 байта, читаем 1 слово
    a = 000004;        // другой адрес
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
    a = 000001;
    b0 = 0x78;
    b_write(a, b0);
    bres = b_read(a);
    print_log(LOG_TRACE, "a = %06o b0 = %hhx bres = %hhx", a, b0, bres);
    assert(b0 == bres);

    //пишем слово, читаем побайтово (проверка Little-Endian)
    a = 000006;
    w = 0xCDE1;
    w_write(a, w, MEMSPACE);
    b0 = b_read(a);     // должен быть младший байт: 0xE1
    b1 = b_read(a + 1); // должен быть старший байт: 0xCD
    print_log(LOG_TRACE, "a = %06o w = %04x b1 = %02hhx b0 = %02hhx", a, w, b1, b0);
    assert(b0 == 0xE1);
    assert(b1 == 0xCD);

    //проверка отрицательных чисел (старший бит равен 1):

    //пишем и читаем отрицательный байт
    a = 000001; 
    signed_b0 = -123; 
    b_write(a, (Byte)signed_b0);
    signed_bres = (signed char)b_read(a);
    print_log(LOG_TRACE, "a = %06o signed_b0 = %d signed_bres = %d", a, signed_b0, signed_bres);
    assert(signed_b0 == signed_bres);

    //пишем отрицательное слово, читаем побайтово (проверка Little-Endian)
    a = 000012;
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
    w_write(000001, 0x1234); 

    print_log(LOG_INFO, "Читаем слово по нечетному адресу");
    w_read(000003);
    */

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//3. Юнит-тесты для проверки работы с командами процессора:

//тест на распознавание команды MOV, ADD, HALT
void test_parse_mov(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //MOV (опкод 0010604)
    Command cmd_mov = parse_cmd(0010604);
    assert(strcmp(cmd_mov.name, "mov") == 0);
    reset_cpu_state();

    //ADD (опкод 0060102)
    Command cmd_add = parse_cmd(0060102);
    assert(strcmp(cmd_add.name, "add") == 0);
    reset_cpu_state();

    //HALT (опкод 0000000)
    Command cmd_halt = parse_cmd(0000000);
    assert(strcmp(cmd_halt.name, "halt") == 0);
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на выполнение MOV по моде 0 в команде MOV R5, R3
 void test_mov(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //MOV
    reg[3] = 12;
    reg[5] = 34;
    
    byte_cmd = 0; 

    Command cmd_word = parse_cmd(0010503);

    cmd_word.do_command();

    assert(reg[3] == 34);
    assert(reg[5] == 34);
    assert(flag_Z == 0);
    assert(flag_N == 0);

    reset_cpu_state();

    //MOVb
    reg[3] = 0;
    reg[5] = 0x00F1;
    
    Command cmd_byte = parse_cmd(0110503);
    
    cmd_byte.do_command();

    assert(reg[3] == 0xFFF1); 
    assert(reg[5] == 0x00F1); 
    assert(flag_Z == 0);
    assert(flag_N == 1);

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
    assert(flag_N == 0);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //setup 2
    reg[1] = 1;       //последняя итерация цикла
    PC = 001020;

    cmd.do_command();

    assert(reg[1] == 0);
    assert(PC == 001020);
    assert(flag_N == 0);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на выполнение CLR в команде CLR R4
void test_clr(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup CLR
    reg[4] = 1234;

    Command cmd = parse_cmd(0005004);

    cmd.do_command();

    assert(reg[4] == 0);
    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    //setup CLRb
    reg[4] = 5678; 

    Command cmd_byte = parse_cmd(0105004);
    cmd_byte.do_command();

    assert(reg[4] == 0);
    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

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
    assert(ss.space == REGSPACE);
    assert(strcmp(ss.name, "R5") == 0);
    assert(dd.val == 12);
    assert(dd.adr == 3);
    assert(dd.space == REGSPACE);
    assert(strcmp(dd.name, "R3") == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на чтение аргументов ss и dd в MOV (R5), R3
void test_mode1_toreg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    // setup
    reg[3] = 12;
    reg[5] = 000200;
    w_write(000200, 34, MEMSPACE);

    Command cmd = parse_cmd(0011503);

    assert(ss.val == 34);
    assert(ss.adr == 000200);
    assert(ss.space == MEMSPACE);
    assert(strcmp(ss.name, "(R5)") == 0);
    assert(dd.val == 12);
    assert(dd.adr == 3);
    assert(dd.space == REGSPACE);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 34);
    assert(reg[5] == 000200);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на запись из регистра в память MOV R3, (R5)
void test_mode1_fromreg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    //setup
    reg[3] = 75;
    reg[5] = 000400;
    w_write(000400, 0, MEMSPACE);

    Command cmd = parse_cmd(0010315);

    assert(ss.val == 75);
    assert(ss.adr == 3);
    assert(ss.space == REGSPACE);
    assert(strcmp(ss.name, "R3") == 0);

    assert(dd.adr == 000400);
    assert(dd.space == MEMSPACE);
    assert(strcmp(dd.name, "(R5)") == 0);

    cmd.do_command();

    assert(w_read(000400) == 75);
    assert(reg[3] == 75);
    assert(reg[5] == 000400);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент регистра MOV (R5)+, R3
void test_mode2_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 000200;
    w_write(000200, 55, MEMSPACE);

    Command cmd = parse_cmd(0012503);

    assert(ss.val == 55);
    assert(ss.adr == 000200);
    assert(ss.space == MEMSPACE);
    assert(strcmp(ss.name, "(R5)+") == 0);

    assert(dd.val == 0);
    assert(dd.adr == 3);
    assert(dd.space == REGSPACE);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 55);
    assert(reg[5] == 000202);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент регистра R7 MOV #77, R3
void test_mode2_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    PC = 002000;
    w_write(002000, 77, MEMSPACE);

    Command cmd = parse_cmd(0012703);

    assert(ss.val == 77);
    assert(ss.adr == 002000);
    assert(ss.space == MEMSPACE);
    assert(strcmp(ss.name, "#115") == 0);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 77);
    assert(PC == 002002);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент косвенной моды регистра MOV @(R5)+, R3
void test_mode3_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 000200;
    w_write(000200, 000400, MEMSPACE);
    w_write(000400, 85, MEMSPACE);

    Command cmd = parse_cmd(0013503);

    assert(ss.val == 85);
    assert(ss.adr == 000400);
    assert(ss.space == MEMSPACE);
    assert(strcmp(ss.name, "@(R5)+") == 0);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 85);
    assert(reg[5] == 000202);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автоинкремент абсолютного режима PC MOV @#400, R3
void test_mode3_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    PC = 002000;
    w_write(002000, 000400, MEMSPACE);
    w_write(000400, 99, MEMSPACE);

    Command cmd = parse_cmd(0013703);

    assert(ss.val == 99);
    assert(ss.adr == 000400);
    assert(ss.space == MEMSPACE);
    assert(strncmp(ss.name, "@#", 2) == 0);
    assert(strstr(ss.name, "400") != NULL);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 99);
    assert(PC == 002002);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автодекремент MOV R3, -(R5)
void test_mode4(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 88;
    reg[5] = 000402;
    w_write(000400, 0, MEMSPACE);

    Command cmd = parse_cmd(0010345);

    assert(ss.val == 88);
    assert(ss.adr == 3);
    assert(ss.space == REGSPACE);
    assert(strcmp(ss.name, "R3") == 0);
    assert(dd.adr == 000400);
    assert(dd.space == MEMSPACE);
    assert(strcmp(dd.name, "-(R5)") == 0);

    cmd.do_command();

    assert(w_read(000400) == 88);
    assert(reg[5] == 000400);
    assert(reg[3] == 88);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на автодекремент косвенной моды регистра MOV @-(R5), R3
void test_mode5(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 000402;
    
    w_write(000400, 000600, MEMSPACE); 
    w_write(000600, 95, MEMSPACE);

    Command cmd = parse_cmd(0015503);

    assert(ss.val == 95);
    assert(ss.adr == 000600);
    assert(ss.space == MEMSPACE);
    assert(strcmp(ss.name, "@-(R5)") == 0);
    assert(dd.val == 0);
    assert(dd.adr == 3);
    assert(dd.space == REGSPACE);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 95);
    assert(reg[5] == 000400);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на индексную адресацию регистра MOV 4(R5), R3 
void test_mode6_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 000200;
    
    PC = 002000;
    w_write(002000, 4, MEMSPACE);
    
    w_write(000204, 66, MEMSPACE); 

    Command cmd = parse_cmd(0016503);

    assert(ss.val == 66);
    assert(ss.adr == 000204);
    assert(ss.space == MEMSPACE);
    assert(strcmp(ss.name, "4(R5)") == 0);
    assert(PC == 02002);
     assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 66);
    assert(reg[5] == 000200);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на относительную адресацию через PC MOV 10(PC), R3
void test_mode6_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    PC = 002000;
    w_write(002000, 010, MEMSPACE);

    w_write(002012, 123, MEMSPACE);

    Command cmd = parse_cmd(0016703);

    assert(ss.val == 123);
    assert(ss.adr == 002012);
    assert(ss.space == MEMSPACE);
    assert(strstr(ss.name, "2012") != NULL);
    assert(PC == 002002);

    cmd.do_command();

    assert(reg[3] == 123);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на индексную косвенную адресацию регистра: MOV @4(R5), R3
void test_mode7_reg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    reg[5] = 000200;
    
    PC = 002000;
    w_write(002000, 4, MEMSPACE);
    w_write(000204, 000600, MEMSPACE); 
    w_write(000600, 77, MEMSPACE); 

    Command cmd = parse_cmd(0017503);

    assert(ss.val == 77);
    assert(ss.adr == 000600);
    assert(ss.space == MEMSPACE);
    assert(ss.name[0] == '@');
    assert(strstr(ss.name, "(R5)") != NULL);
    assert(PC == 002002);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 77);
    assert(reg[5] == 000200);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на индексную косвенную адресацию регистра: MOV @4(R5), R3
void test_mode7_pc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[3] = 0;
    PC = 002000;
    w_write(002000, 8, MEMSPACE);
    w_write(002012, 000700, MEMSPACE);
    w_write(000700, 150, MEMSPACE);

    Command cmd = parse_cmd(0017703);

    assert(ss.val == 150);
    assert(ss.adr == 000700);
    assert(ss.space == MEMSPACE);
    assert(strncmp(ss.name, "@#", 2) == 0);
    assert(strstr(ss.name, "700") != NULL);
    assert(PC == 002002);
    assert(strcmp(dd.name, "R3") == 0);

    cmd.do_command();

    assert(reg[3] == 150);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

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
    reg[5] = 0xFFFF;

    Command cmd = parse_cmd(0010503);

    cmd.do_command();

    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);
    
    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест в ADD на C = 1 и Z = 1, остальные 0
void test_flags_add_carry(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    flag_N = 1; flag_Z = 0; flag_V = 1; flag_C = 0;
    reg[5] = 0xFFFF;
    reg[3] = 1;

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
    reg[5] = 16384;
    reg[3] = 16384;

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
    PC = 001002;

    Command cmd = parse_cmd(0000400);
    
    assert(strcmp(cmd.name, "br") == 0);
    assert(xx == 0); 

    cmd.do_command();

    assert(PC == 001002);
 
    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на безусловный переход вперед
void test_br_forward(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    PC = 001002;

    Command cmd = parse_cmd(0000402);
    assert(xx == 2);

    cmd.do_command();

    assert(PC == 001006);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на безусловный переход назад
void test_br_backward(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup
    PC = 001006;

    Command cmd = parse_cmd(0000775);
    assert(xx == -3);

    cmd.do_command();

    assert(PC == 001000);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на условный переход по флагу нуля (Z = 1)
void test_beq(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup Z = 1
    flag_Z = 1;
    flag_N = 1; flag_V = 0; flag_C = 1;
    PC = 001002;

    Command cmd = parse_cmd(0001402);
    cmd.do_command();

    assert(PC == 001006);
    assert(flag_Z == 1);
    assert(flag_N == 1);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //setup Z = 0
    flag_Z = 0;
    PC = 001002;

    cmd.do_command();

    assert(PC == 01002);
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на условный переход по флагу знака (N = 0)
void test_bpl(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);
    
    //setup N = 0
    flag_N = 0;
    flag_Z = 1; flag_V = 0; flag_C = 1;
    PC = 001002;

    Command cmd = parse_cmd(0100002);
    cmd.do_command();

    assert(PC == 001006);
    assert(flag_N == 0);
    assert(flag_Z == 1);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //setup N = 1
    flag_N = 1;
    PC = 001002;

    cmd.do_command();

    assert(PC == 001002);
    assert(flag_N == 1);
    assert(flag_Z == 1);
    assert(flag_V == 0);
    assert(flag_C == 1);

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
    flag_N = 1; flag_V = 1; flag_C = 0;
    PC = 001002;

    Command cmd = parse_cmd(0001002);
    cmd.do_command();

    assert(PC == 001006);
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_V == 1);
    assert(flag_C == 0);

    //setup Z = 1
    flag_Z = 1;
    PC = 001002;

    cmd.do_command();

    assert(PC == 01002);
    assert(flag_Z == 1);
    assert(flag_N == 1);
    assert(flag_V == 1);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на выставление отрицательного байта N в TSTb
void test_tst(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup TST
    byte_cmd = 0;
    flag_C = 1; flag_V = 1; flag_N = 0; flag_Z = 1;
    dd.val = 0x8000;
    dd.adr = 1; 
    dd.space = REGSPACE;

    do_tst();

    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //setup TSTb
    byte_cmd = 1;
    flag_N = 1; flag_Z = 0; flag_V = 1; flag_C = 1;
    dd.val = 0;
    dd.adr = 1; 
    dd.space = REGSPACE;

    do_tst();

    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на вызов подпрограмм JSR/RTS по регистру R2
void test_jsr_rts(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    SP = 0177700; 
    reg[2] = 12;
    flag_N = 0; flag_Z = 0; flag_V = 0; flag_C = 0;
    
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
    assert(flag_N == 0); 
    assert(flag_Z == 0); 
    assert(flag_V == 0); 
    assert(flag_C == 0);

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
    assert(flag_N == 0); 
    assert(flag_Z == 0); 
    assert(flag_V == 0); 
    assert(flag_C == 0);

    //clean
    reset_cpu_state();
    
    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на проверку сдвига влево и сдвига вправо командой ASH
void test_ash(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);

    //setup cдвиг влево
    reg[2] = 2;
    r = 2;
    dd.val = 2;
    
    do_ash();
    
    assert(reg[2] == 8);
    assert(flag_N == 0);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //setup сдвиг вправо
    reg[3] = 16;
    r = 3;
    dd.val = 62;
    
    do_ash();
    
    assert(reg[3] == 4);
    assert(flag_N == 0);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на прибалвение переноса к байту командой ADCb
void test_adc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup ADCb
    byte_cmd = 1;
    flag_C = 1; flag_N = 1; flag_Z = 1; flag_V = 1;
    reg[4] = 5;
    dd.val = 5;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_adc();

    assert(reg[4] == 6);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //setup ADC
    byte_cmd = 0;
    flag_C = 1;
    flag_N = 1; flag_Z = 0; flag_V = 1;
    
    reg[4] = 0xFFFF;
    dd.val = 0xFFFF;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_adc();

    assert(reg[4] == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASHC
void test_ashc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[2] = 0; 
    reg[3] = 4;
    r = 2; 
    dd.val = 62;
    flag_N = 1; flag_Z = 1; flag_V = 1; flag_C = 1;

    do_ashc();

    assert(reg[2] == 0); 
    assert(reg[3] == 1);
    assert(flag_N == 0);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASL со словом
void test_asl(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[1] = 0x8000;
    byte_cmd = 0;
    dd.val = 0x8000; 
    dd.adr = 1; 
    dd.space = REGSPACE;
    flag_N = 1; flag_Z = 0; flag_V = 0; flag_C = 0;

    do_asl();

    assert(reg[1] == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);
    assert(flag_N == 0);
    assert(flag_V == 1);
    
    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASLb с байтом
void test_aslb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[4] = 0x80;
    byte_cmd = 1;
    dd.val = 0x80; 
    dd.adr = 4; 
    dd.space = REGSPACE;
    flag_N = 1; flag_Z = 0; flag_V = 0; flag_C = 0;

    do_asl();

    assert((reg[4] & 0xFF) == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);
    assert(flag_N == 0);
    assert(flag_V == 1); 

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASR со словом
void test_asr(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[2] = 0x8001;
    byte_cmd = 0;
    dd.val = 0x8001; 
    dd.adr = 2; 
    dd.space = REGSPACE;
    flag_N = 0; flag_Z = 1; flag_V = 1; flag_C = 0;

    do_asr();

    assert(reg[2] == 0xC000);
    assert(flag_C == 1);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE,"Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды ASRb с байтом
void test_asrb(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    reg[5] = 0x81;
    byte_cmd = 1;
    dd.val = 0x81; 
    dd.adr = 5; 
    dd.space = REGSPACE;
    flag_N = 0; flag_Z = 1; flag_V = 1; flag_C = 0;

    do_asr();

    assert((reg[5] & 0xFF) == 0xC0);
    assert(flag_C == 1);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);

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
    PC = 001000; 
    flag_C = 1; flag_N = 0; flag_Z = 0; flag_V = 0;

    do_bcs();

    assert(PC == 001010);
    assert(flag_C == 1 && flag_N == 0 && flag_Z == 0 && flag_V == 0);

    PC = 001000; 
    flag_C = 1; flag_N = 0; flag_Z = 0; flag_V = 0;

    do_bcc();

    assert(PC == 001000);
    assert(flag_C == 1 && flag_N == 0 && flag_Z == 0 && flag_V == 0);

    //setup BMI, BPL, BEQ, BNE
    PC = 001000; 
    flag_C = 0; flag_N = 1; flag_Z = 0; flag_V = 0;

    do_bmi(); 

    assert(PC == 001010);
    assert(flag_C == 0 && flag_N == 1 && flag_Z == 0 && flag_V == 0);

    PC = 001000; 
    flag_C = 0; flag_N = 0; flag_Z = 1; flag_V = 0;

    do_bpl(); 
    
    assert(PC == 001010);
    assert(flag_C == 0 && flag_N == 0 && flag_Z == 1 && flag_V == 0);

    PC = 001000; 
    flag_C = 0; flag_N = 0; flag_Z = 1; flag_V = 0;

    do_beq(); 
    
    assert(PC == 001010);
    assert(flag_C == 0 && flag_N == 0 && flag_Z == 1 && flag_V == 0);

    PC = 001000; 
    flag_C = 0; flag_N = 0; flag_Z = 1; flag_V = 0;

    do_bne(); 
    
    assert(PC == 001000);
    assert(flag_C == 0 && flag_N == 0 && flag_Z == 1 && flag_V == 0);

    //setup BHI, BLOS
    PC = 001000; 
    flag_C = 0; flag_Z = 0; flag_N = 0; flag_V = 0;

    do_bhi(); 
    
    assert(PC == 001010);
    assert(flag_C == 0 && flag_Z == 0);

    PC = 001000; 
    flag_C = 1; flag_Z = 0; flag_N = 0; flag_V = 0;

    do_bhi(); 
    
    assert(PC == 001000);
    assert(flag_C == 1 && flag_Z == 0);

    PC = 001000;
    flag_C = 1; flag_Z = 0; flag_N = 0; flag_V = 0; 

    do_blos(); 
    
    assert(PC == 001010);
    assert(flag_C == 1 && flag_Z == 0);

    PC = 001000; 
    flag_C = 0; flag_Z = 1; flag_N = 0; flag_V = 0;

    do_blos(); 
    
    assert(PC == 001010);
    assert(flag_C == 0 && flag_Z == 1);

    PC = 001000; 
    flag_C = 0; flag_Z = 0; flag_N = 0; flag_V = 0;

    do_blos(); 
    
    assert(PC == 001000);
    assert(flag_C == 0 && flag_Z == 0);

    //setup BGE, BLT, BGT, BLE
    PC = 001000; 
    flag_N = 1; flag_V = 0; flag_Z = 0; flag_C = 0;

    do_blt(); 
    
    assert(PC == 001010);
    assert(flag_N == 1 && flag_V == 0);

    PC = 001000; 
    flag_N = 1; flag_V = 1; flag_Z = 0; flag_C = 0;

    do_bge(); 
    
    assert(PC == 001010);
    assert(flag_N == 1 && flag_V == 1);

    PC = 001000; 
    flag_Z = 0; flag_N = 1; flag_V = 1; flag_C = 0;

    do_bgt(); 
    
    assert(PC == 001010);
    assert(flag_Z == 0 && flag_N == 1 && flag_V == 1);

    PC = 001000; 
    flag_Z = 1; flag_N = 1; flag_V = 1; flag_C = 0;

    do_bgt(); 
    
    assert(PC == 001000);
    assert(flag_Z == 1 && flag_N == 1 && flag_V == 1);

    PC = 001000; 
    flag_Z = 1; flag_N = 0; flag_V = 0; flag_C = 0;

    do_ble(); 
    
    assert(PC == 001010);
    assert(flag_Z == 1 && flag_N == 0 && flag_V == 0);

    PC = 001000; 
    flag_Z = 0; flag_N = 1; flag_V = 0; flag_C = 0;

    do_ble(); 
    
    assert(PC == 001010);
    assert(flag_Z == 0 && flag_N == 1 && flag_V == 0);

    //setup BVC, BVS

    PC = 001000; 
    flag_V = 1; flag_N = 0; flag_Z = 0; flag_C = 0;

    do_bvs(); 
    
    assert(PC == 001010);
    assert(flag_V == 1);

    PC = 001000; 
    flag_V = 0; flag_N = 0; flag_Z = 0; flag_C = 0;

    do_bvc(); 
    
    assert(PC == 001010);
    assert(flag_V == 0);

    PC = 001000; 
    flag_V = 1; flag_N = 0; flag_Z = 0; flag_C = 0;

    do_bvc(); 
    
    assert(PC == 001000);
    assert(flag_V == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу логических команд BIC, BIS, BIT
void test_bit_logic_bytes(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup for BICb, BISb, BITb
    byte_cmd = 1;
    flag_C = 1;

    //setup BISb
    reg[1] = 0x05;
    ss.val = 0x50;
    dd.val = 0x05; 
    dd.adr = 1; 
    dd.space = REGSPACE;
    
    do_bis();

    assert((reg[1] & 0xFF) == 0x55);
    assert(flag_C == 1);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0); 

    //setup BICb
    ss.val = 0x05;
    dd.val = 0x55;
    dd.adr = 1; 
    dd.space = REGSPACE;
    
    do_bic();

    assert((reg[1] & 0xFF) == 0x50);
    assert(flag_C == 1);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //setup BITb
    reg[1] = 0x50; 
    ss.val = 0x10; 
    dd.val = 0x50; 
    dd.adr = 1; 
    dd.space = REGSPACE;
    
    do_bit();

    assert((reg[1] & 0xFF) == 0x50); 
    assert(flag_Z == 0); 
    assert(flag_C == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    //setup for BIC, BIS, BIT
    byte_cmd = 0;
    flag_C = 0;

    //setup BIS
    reg[3] = 0x00FF;
    ss.val = 0x8000; 
    dd.val = 0x00FF;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_bis();

    assert(reg[3] == 0x80FF);
    assert(flag_C == 0);
    assert(flag_Z == 0);
    assert(flag_N == 1); 
    assert(flag_V == 0);

    //setup BIC
    ss.val = 0x80FF; 
    dd.val = 0x80FF;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_bic();

    assert(reg[3] == 0); 
    assert(flag_Z == 1); 
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //setup BIT
    reg[3] = 0x80FF;
    ss.val = 0x8000;
    dd.val = 0x80FF;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_bit();

    assert(reg[3] == 0x80FF);
    assert(flag_Z == 0); 
    assert(flag_N == 1);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команд очистки флагов CLC, CLV, CLZ, CLN, CCC
void test_clear_flags(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup for all
    PC = 001002; 

    //setup CLC
    flag_C = 1; flag_V = 1; flag_Z = 1; flag_N = 1;
    w_write(001000, 0x00A1, MEMSPACE);

    do_clr_fl();

    assert(flag_C == 0);
    assert(flag_V == 1); 
    assert(flag_Z == 1); 
    assert(flag_N == 1);

    //setup CLV
    flag_C = 1; flag_V = 1; flag_Z = 1; flag_N = 1;
    w_write(001000, 0x00A2, MEMSPACE);

    do_clr_fl();

    assert(flag_V == 0);
    assert(flag_C == 1); 
    assert(flag_Z == 1); 
    assert(flag_N == 1);

    //setup CLZ
    flag_C = 1; flag_V = 1; flag_Z = 1; flag_N = 1;
    w_write(001000, 0x00A4, MEMSPACE);

    do_clr_fl();

    assert(flag_Z == 0);
    assert(flag_C == 1); 
    assert(flag_V == 1); 
    assert(flag_N == 1);

    //setup CLN
    flag_C = 1; flag_V = 1; flag_Z = 1; flag_N = 1;
    w_write(001000, 0x00A8, MEMSPACE);

    do_clr_fl();

    assert(flag_N == 0);
    assert(flag_C == 1); 
    assert(flag_V == 1); 
    assert(flag_Z == 1);

    //setup CCC
    flag_C = 1; flag_V = 1; flag_Z = 1; flag_N = 1;
    w_write(01000, 0x00AF, MEMSPACE);

    do_clr_fl();

    assert(flag_C == 0);
    assert(flag_V == 0);
    assert(flag_Z == 0);
    assert(flag_N == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды сравнения байт CMP
void test_cmp(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup CMP
    byte_cmd = 0;
    flag_N = 1; flag_Z = 0; flag_V = 1; flag_C = 1;

    reg[2] = 45;
    ss.val = 45;
    dd.val = 45; 
    dd.adr = 2; 
    dd.space = REGSPACE;

    do_cmp();

    assert(reg[2] == 45);
    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    //setup CMPb
    byte_cmd = 1;
    flag_N = 0; flag_Z = 1; flag_V = 1; flag_C = 0;

    reg[2] = 16;
    ss.val = 8;
    dd.val = 16; 
    dd.adr = 2; 
    dd.space = REGSPACE;

    do_cmp();

    assert(reg[2] == 16);
    assert(flag_Z == 0);
    assert(flag_C == 1);
    assert(flag_N == 1);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды инверсии COM
void test_com(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup COMb
    byte_cmd = 1;
    flag_C = 0; flag_V = 1; flag_N = 0; flag_Z = 1;
    
    reg[3] = 0x55; 
    dd.val = 0x55;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_com();

    assert((reg[3] & 0xFF) == 0xAA); 
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    //setup COM
    byte_cmd = 0;
    flag_C = 0; flag_V = 1; flag_N = 1; flag_Z = 0;
    
    reg[3] = 0;
    dd.val = 0;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_com();

    assert(reg[3] == 0xFFFF);
    assert(flag_Z == 0); 
    assert(flag_N == 1);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды декремента DEC
void test_dec(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup DECb
    byte_cmd = 1;
    flag_C = 1;
    flag_N = 1; flag_Z = 1; flag_V = 0;
    
    reg[4] = 0x80; 
    dd.val = 0x80;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_dec();

    assert((reg[4] & 0xFF) == 0x7F); 
    assert(flag_V == 1);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    //setup DEC
    byte_cmd = 0;
    flag_C = 0;
    flag_N = 0; flag_Z = 1; flag_V = 0;
    
    reg[4] = 0x8000; 
    dd.val = 0x8000;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_dec();

    assert(reg[4] == 0x7FFF);
    assert(flag_V == 1);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды инкремента INCb
void test_inc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup INCb
    byte_cmd = 1;
    flag_C = 1;
    flag_N = 0; flag_Z = 1; flag_V = 0;
    reg[4] = 0x7F; 
    dd.val = 0x7F;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_inc();

    assert((reg[4] & 0xFF) == 0x80); 
    assert(flag_V == 1);
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    //setup INC
    byte_cmd = 0;
    flag_C = 0;
    flag_N = 1; flag_Z = 1; flag_V = 0;
    reg[4] = 0x7FFF; 
    dd.val = 0x7FFF;
    dd.adr = 4;
    dd.space = REGSPACE;

    do_inc();

    assert(reg[4] == 0x8000);
    assert(flag_V == 1);
    assert(flag_Z == 0); 
    assert(flag_N == 1);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды безусловного перехода JMP
void test_jmp(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    PC = 001000;
    reg[2] = 004000;
    dd.adr = 004000;
    dd.val = 0;
    dd.space = MEMSPACE;
    flag_Z = 1; flag_N = 1; flag_C = 1; flag_V = 1;

    do_jmp();

    assert(PC == 004000);
    assert(flag_Z == 1);
    assert(flag_N == 1);
    assert(flag_C == 1);
    assert(flag_V == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды смены знака NEGb
void test_neg(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup NEGb положительное число
    byte_cmd = 1;
    reg[3] = 4; 
    flag_C = 0; flag_V = 1; flag_N = 0; flag_Z = 1;
    dd.val = 4;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_neg();

    assert(reg[3] == 0xFFFC); 
    assert(flag_Z == 0);
    assert(flag_N == 1);
    assert(flag_C == 1);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    //setup NEG ноль
    byte_cmd = 0;
    reg[3] = 0; 
    flag_C = 1; flag_V = 1; flag_N = 1; flag_Z = 0;
    dd.val = 0;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_neg();

    assert(reg[3] == 0); 
    assert(flag_Z == 1);
    assert(flag_N == 0); 
    assert(flag_C == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    //проверка знакового переполнения
    byte_cmd = 0;
    flag_C = 0; flag_V = 0; flag_N = 0; flag_Z = 1;
    
    reg[3] = 0x8000; 
    dd.val = 0x8000;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_neg();

    assert(reg[3] == 0x8000); 
    assert(flag_V == 1);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу пустой команды NOP
void test_nop(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup
    PC = 001002;
    reg[1] = 42;
    flag_N = 1; flag_C = 1; flag_Z = 0; flag_V = 0;
    w_write(001000, 0x00A0, MEMSPACE);

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
    PC = 001002;
    reg[5] = 77; 
    flag_Z = 1; flag_V = 1; flag_N = 0; flag_C = 0;

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

//тест на работу циклического сдвига влево командой ROL
void test_rol(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup ROLb
    byte_cmd = 1;
    flag_C = 0;
    flag_N = 1; flag_Z = 0; flag_V = 1;
    
    reg[3] = 0x80;
    dd.val = 0x80; 
    dd.adr = 3; 
    dd.space = REGSPACE;

    do_rol();

    assert((reg[3] & 0xFF) == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);
    assert(flag_N == 0);
    assert(flag_V == 1);

    //clean
    reset_cpu_state();

    //setup ROL
    byte_cmd = 0;
    flag_C = 1;
    flag_N = 0; flag_Z = 1; flag_V = 1;
    
    reg[3] = 0x0000;
    dd.val = 0x0000;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_rol();

    assert(reg[3] == 1);
    assert(flag_Z == 0);
    assert(flag_C == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу циклического сдвига вправо командой ROL
void test_ror(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup RORb
    byte_cmd = 1;
    flag_C = 1;
    flag_N = 0; flag_Z = 1; flag_V = 1;
    
    reg[3] = 1;
    dd.val = 1; 
    dd.adr = 3; 
    dd.space = REGSPACE;

    do_ror();

    assert((reg[3] & 0xFF) == 0x80);
    assert(flag_C == 1);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    //setup ROR
    byte_cmd = 0;
    flag_C = 0;
    flag_N = 1; flag_Z = 0; flag_V = 1;
    
    reg[3] = 0x8000;
    dd.val = 0x8000;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_ror();

    assert(reg[3] == 0x4000);
    assert(flag_C == 0);
    assert(flag_N == 0);
    assert(flag_Z == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команды вычитания переноса SBC
void test_sbc(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup SBCb
    byte_cmd = 1;
    flag_C = 1;
    flag_N = 1; flag_Z = 1; flag_V = 1;
    
    reg[3] = 5;
    dd.val = 5;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_sbc();

    assert((reg[3] & 0xFF) == 4);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    //setup SBC
    byte_cmd = 0;
    flag_C = 1;
    flag_N = 0; flag_Z = 1; flag_V = 1;
    
    reg[3] = 0;
    dd.val = 0;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_sbc();

    assert(reg[3] == 0xFFFF);
    assert(flag_Z == 0); 
    assert(flag_N == 1);
    assert(flag_C == 1);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    //проверка знакового переполнения (V = 1)
    byte_cmd = 0;
    flag_C = 1;
    flag_V = 0; flag_N = 0; flag_Z = 1;
    
    reg[3] = 0x8000; 
    dd.val = 0x8000;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_sbc();

    assert(reg[3] == 0x7FFF); 
    assert(flag_V == 1);
    assert(flag_N == 0);
    assert(flag_Z == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на работу команд установки флагов
void test_set_flags(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup for all
    PC = 001002; 

    //setup SEC
    flag_C = 0; flag_V = 0; flag_Z = 0; flag_N = 0;
    w_write(001000, 0x00B1, MEMSPACE);

    do_set_fl();

    assert(flag_C == 1);
    assert(flag_V == 0); 
    assert(flag_Z == 0); 
    assert(flag_N == 0);

    //setup SEV
    flag_C = 0; flag_V = 0; flag_Z = 0; flag_N = 0;
    w_write(001000, 0x00B2, MEMSPACE);

    do_set_fl();

    assert(flag_V == 1);
    assert(flag_C == 0); 
    assert(flag_Z == 0); 
    assert(flag_N == 0);

    //setup SEZ
    flag_C = 0; flag_V = 0; flag_Z = 0; flag_N = 0;
    w_write(001000, 0x00B4, MEMSPACE);

    do_set_fl();

    assert(flag_Z == 1);
    assert(flag_C == 0); 
    assert(flag_V == 0); 
    assert(flag_N == 0);

    //setup SEN
    flag_C = 0; flag_V = 0; flag_Z = 0; flag_N = 0;
    w_write(001000, 0x00B8, MEMSPACE);

    do_set_fl();

    assert(flag_N == 1);
    assert(flag_C == 0); 
    assert(flag_V == 0); 
    assert(flag_Z == 0);

    //setup SCC
    flag_C = 0; flag_V = 0; flag_Z = 0; flag_N = 0;
    w_write(001000, 0x00BF, MEMSPACE);

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

    //обычное вычитание (из большего меньшее)
    byte_cmd = 0;
    flag_N = 1; flag_Z = 1; flag_V = 1; flag_C = 1;

    reg[2] = 12;
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

    //clean
    reset_cpu_state();

    //из меньшего большее
    byte_cmd = 0;
    flag_N = 0; flag_Z = 1; flag_V = 1; flag_C = 0;

    reg[2] = 5;
    ss.val = 15;
    dd.val = 5; 
    dd.adr = 2; 
    dd.space = REGSPACE;

    do_sub();

    assert(reg[2] == 0xFFF6);
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
    Word test_val = 0x14E5;
    Word expected_low = test_val & 0xFF;
    Word expected_high = (test_val >> 8) & 0xFF;
    Word expected_res = (expected_low << 8) | expected_high;

    reg[2] = test_val; 
    byte_cmd = 0;
    flag_Z = 1; flag_N = 1; flag_V = 1; flag_C = 1;
    dd.val = test_val;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_swab();

    assert(reg[2] == expected_res);
    assert(flag_Z == (expected_high == 0));
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
    reg[2] = 0x14E5;
    byte_cmd = 0;
    flag_N = 1; flag_C = 1; flag_V = 1;
    dd.val = 0;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_sxt();

    assert(reg[2] == 0xFFFF);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    //setup N = 0
    reg[2] = 0x14E5;
    byte_cmd = 0;
    flag_N = 0; flag_C = 1; flag_V = 1;
    dd.val = 0;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_sxt();

    assert(reg[2] == 0);
    assert(flag_N == 0);
    assert(flag_Z == 1);
    assert(flag_C == 1);
    assert(flag_V == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на исключающее ИЛИ командой XOR
void test_xor(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    //setup разных чисел
    byte_cmd = 0;
    r = 2;
    flag_C = 1; flag_V = 1; flag_N = 1; flag_Z = 1;
    
    reg[2] = 0x14E5; 
    reg[3] = 0x0A9A;
    dd.val = 0x0A9A;
    dd.adr = 3;
    dd.space = REGSPACE;

    do_xor();

    assert(reg[3] == 0x1E7F);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    //setup одинаковых чисел
    byte_cmd = 0;
    r = 2;
    flag_C = 1; flag_V = 1; flag_N = 1; flag_Z = 0;
    
    reg[2] = 0x14E5;
    dd.val = 0x14E5;
    dd.adr = 2;
    dd.space = REGSPACE;

    do_xor();

    assert(reg[2] == 0);
    assert(flag_Z == 1);
    assert(flag_N == 0);
    assert(flag_V == 0);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

// тест на умножение командой MUL (все аппаратные варианты DEC PDP-11)
void test_mul(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);

    // ===================================================================
    // КЕЙС 1: Чётный регистр R2 (Результат 32-битный, раскладывается по R2 и R3)
    // ===================================================================
    reset_cpu_state();
    reg[2] = 10;
    r = 2;
    dd.val = 64;
    flag_Z = 1; flag_N = 1; flag_V = 1; flag_C = 1; // Забиваем флаги мусором

    do_mul();

    assert(reg[2] == 0);      // Старшие 16 бит результата
    assert(reg[3] == 640);    // Младшие 16 бит результата
    assert(flag_Z == 0);      // Результат не ноль
    assert(flag_N == 0);      // Результат положительный
    assert(flag_V == 0);      // V всегда 0 для MUL
    assert(flag_C == 0);      // Число 640 влезает в 16 бит знака, переноса нет

    // ===================================================================
    // КЕЙС 2: Нечётный регистр R3 (Результат только 16-битный, строго в R3)
    // ===================================================================
    reset_cpu_state();
    reg[3] = 5;
    r = 3;
    dd.val = 20;

    do_mul();

    assert(reg[3] == 100);    // Результат строго в R3
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_C == 0);

    // ===================================================================
    // КЕЙС 3: Знаковая математика (Умножение отрицательных чисел: -2 * -5 = +10)
    // -2 в 16-битной сетке = 0177776 (0xFFFF). -5 = 0177773 (0xFFFB)
    // ===================================================================
    reset_cpu_state();
    reg[1] = 0177776; // -2 (Нечётный регистр R1)
    r = 1;
    dd.val = 0177773; // -5

    do_mul();

    assert(reg[1] == 10);     // Результат должен стать чистой положительной 10
    assert(flag_Z == 0);
    assert(flag_N == 0);      // Флаг N обязан быть 0 (результат положительный!)
    assert(flag_C == 0);

    // ===================================================================
    // КЕЙС 4: Большое знаковое произведение с вылетом за 16 бит (Проверка флага C)
    // 1000 * 200 = 60000. В знаковой 16-битной сетке short это число вылетает за 32767
    // и превращается в отрицательное. Но флаг C должен взвестись!
    // ===================================================================
    reset_cpu_state();
    reg[0] = 1000; // Чётный регистр R0
    r = 0;
    dd.val = 200;

    do_mul();

    // 60000 в hex — это 0xEA60. Старшая часть (R0) = 0x0003 (3), Младшая (R1) = 0x0D40 (006500 восьмеричное)
    assert(reg[0] == 3);        
    assert(reg[1] == 0006500); 
    assert(flag_Z == 0);
    assert(flag_C == 1);      // Вылет за пределы 16 бит, Carry взведен

    // ===================================================================
    // КЕЙС 5: Проверка флага знака N для НЕЧЁТНОГО регистра (Смотрит на 15-й бит)
    // Умножаем 1000 * -50. Результат -50000. В 32-битной сетке это отрицательное число.
    // Но младшие 16 бит от -50000 — это число +15536 (0x3CB0), у которого 15-й бит равен 0!
    // По канону DEC, для нечётного регистра flag_N обязан стать равным 0!
    // ===================================================================
    reset_cpu_state();
    reg[5] = 1000; // Нечётный регистр R5
    r = 5;
    dd.val = 0177716; // -50 (восьмеричное)

    do_mul();

    assert(reg[5] == 0036260); // Младшие 16 бит от -50000 (в hex 0x3CB0 = 15536)
    assert(flag_N == 0);       // Магия кремния DEC: flag_N равен 0, хотя само произведение отрицательное!
    assert(flag_C == 1);       // Вылет за пределы short диапазона

    //clean
    reset_cpu_state();
    
    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест на деление командой DIV
void test_div(void) {
    print_log(LOG_TRACE,"Testing function <%s> ...", __FUNCTION__);

    reg[2] = 0;
    reg[3] = 2000;
    r = 2;
    dd.val = 2;
    flag_Z = 1; flag_N = 1; flag_V = 1; flag_C = 1;

    do_div();

    assert(reg[2] == 1000);
    assert(reg[3] == 0);
    assert(flag_Z == 0);
    assert(flag_N == 0);
    assert(flag_V == 0);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест для проверки ввода символов с клавиатуры
void test_keyboard(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);
    
    reset_cpu_state();
    
    //setup b_read
    keyboard_rcsr = 0;
    keyboard_rbuf = 0;
    Byte rcsr_init = b_read(0177560); //RCSR
    assert((rcsr_init & 0200) == 0);

    //cимулируем «нажатие» клавиши: кладём в буфер символ 'A' и взводим Ready-флаг (0200)
    keyboard_rbuf = 'A';
    keyboard_rcsr |= 0200;
    Byte rcsr_ready = b_read(0177560);
    assert((rcsr_ready & 0200) != 0);

    //читаем символ из RBUF
    Byte read_symbol = b_read(0177562);
    assert(read_symbol == 'A');

    //АППАРАТНАЯ ПРОВЕРКА: после чтения из RBUF флаг в RCSR обязан автоматически сброситься
    Byte rcsr_after_read = b_read(0177560);
    assert((rcsr_after_read & 0200) == 0);

    //setup w_read
    keyboard_rbuf = 'Z';
    keyboard_rcsr |= 0200;

    Word w_rcsr = w_read(0177560);
    assert((w_rcsr & 0200) != 0);

    Word w_rbuf = w_read(0177562);
    assert((w_rbuf & 0xFF) == 'Z');

    Word w_rcsr_after = w_read(0177560);
    assert((w_rcsr_after & 0200) == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест для проверки таймера
void test_timer(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);
    
    reset_cpu_state();
    
    //setup b_read
    timer_lks = 0;
    Byte lks_init = b_read(0177546); //LKS
    assert((lks_init & 000200) == 0);

    //симулируем выполнение 999 инструкций — таймер должен молчать
    for (int i = 0; i < 999; i++) {
        timer_tick();
    }
    Byte lks_999 = b_read(0177546);
    assert((lks_999 & 0200) == 0);

    //1000-й
    timer_tick();
    
    //LKS аппаратно зажёгся 7-й бит готовности (000200)
    Byte lks_tick = b_read(0177546);
    assert((lks_tick & 000200) != 0);

    //АППАРАТНАЯ ПРОВЕРКА: так как на прошлом шаге мы вызвали b_read(LKS),
    //флаг готовности обязан автоматически погаснуть! Проверяем повторным чтением:
    Byte lks_after_read = b_read(0177546);
    assert((lks_after_read & 0200) == 0);

    //setup w_read
    for (int i = 0; i < 1000; i++) {
        timer_tick();
    }
    
    //флаг готовности взведен
    Word w_lks = w_read(0177546);
    assert((w_lks & 0200) != 0);

    //после чтения w_read флаг обязан автоматически сброситься в ноль
    Word w_lks_after = w_read(0177546);
    assert((w_lks_after & 0200) == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест для проверки аппаратных прерываний TRAP 0100
void test_interrupt(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);
    
    reset_cpu_state();
    
    //setup
    SP = 003000;
    PC = 001000;
    w_write(000100, 002000, MEMSPACE);
    w_write(000102, 000000, MEMSPACE);
    w_write(002000, 0000002, MEMSPACE);
    flag_N = 1; flag_Z = 0; flag_V = 1; flag_C = 0;
    timer_lks = 0100;

    //cимулируем выполнение 1000 инструкций, чтобы таймер сделал тик
    for (int i = 0; i < 1000; i++) {
        timer_tick();
    }
    
    //проверка флага таймера готовности
    assert(timer_lks == 0300);

    //вызываем обработчик прерываний
    interrupts();

    assert(PC == 002000);          //PC теперь указывает на вектор прерывания 002000
    assert(timer_lks == 0100);     //7-й бит готовности в LKS автоматически сбросился
    assert(SP == 002774);          //стек вырос вниз на два слова (-4 байта)

    //проверка что лежит в стеке
    Word saved_pc = w_read(SP);
    assert(saved_pc == 001000);
    Word saved_psw = w_read(SP + 2);
    assert((saved_psw & 010) != 0);
    assert((saved_psw & 002) != 0);

    assert(flag_N == 0 && flag_Z == 0 && flag_V == 0 && flag_C == 0);

    //RTI
    Word current_opcode = w_read(PC);
    assert(current_opcode == 0000002);
    PC += 2; 

    do_rti();

    assert(PC == 001000);
    assert(SP == 003000);
    assert(flag_N == 1);
    assert(flag_Z == 0);
    assert(flag_V == 1);
    assert(flag_C == 0);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест для проверки аппаратных прерываний клавиатуры TRAP 0060
void test_keyboard_interrupt(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);
    
    reset_cpu_state();
    
    //setup
    SP = 003000; 
    PC = 001000; 

    w_write(000060, 004000, MEMSPACE);
    w_write(000062, 000000, MEMSPACE);
    w_write(004000, 0000002, MEMSPACE);

    //проверяем пассивный режим: прилетела клавиша 'B', но прерывания клавиатуры ЗАПРЕЩЕНЫ (IE = 0)
    keyboard_rcsr = 000000; // IE = 0
    keyboard_rbuf = 'B';
    keyboard_rcsr |= 000200; // Ready = 1

    interrupts();
    assert(PC == 001000); // PC не изменился

    //активный режим: разрешаем прерывания клавиатуры (IE = 0100)
    keyboard_rcsr = 000100; // Включаем 6-й бит
    
    //имитация нажатия клавиши
    keyboard_rcsr |= 000200; 
    assert(keyboard_rcsr == 000300);

    interrupts();

    assert(PC == 004000);
    assert(keyboard_rcsr == 000100);\
    assert(SP == 002774);

    //RTI
    PC += 2;
    do_rti();

    assert(PC == 001000);
    assert(SP == 003000);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

//тест для проверки программных прерываний по векторам 0030 (EMT) и 0034 (TRAP)
void test_sys_traps(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);
    
    reset_cpu_state();
    
    SP = 003000; 
    
    //setup EMT
    PC = 001000; 
    w_write(000030, 005000, MEMSPACE);
    w_write(000032, 000000, MEMSPACE);
    w_write(005000, 0000002, MEMSPACE);
    flag_Z = 1;

    PC += 2;
    do_emt();

    assert(PC == 005000);
    assert(SP == 002774);
    assert(flag_Z == 0);

    PC += 2;
    do_rti();
    
    assert(PC == 001002);
    assert(SP == 003000);
    assert(flag_Z == 1);

    //clean
    reset_cpu_state();

    //setup TRAP
    SP = 003000;
    PC = 001100;

    w_write(000034, 006000, MEMSPACE);
    w_write(000036, 000000, MEMSPACE);
    w_write(006000, 0000002, MEMSPACE);
    flag_C = 1;

    PC += 2;
    do_trap();

    assert(PC == 006000);
    assert(SP == 002774);
    assert(flag_C == 0);

    PC += 2;
    do_rti();

    assert(PC == 001102);
    assert(SP == 003000);
    assert(flag_C == 1);

    //clean
    reset_cpu_state();

    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}

void test_rk11_disk(void) {
    print_log(LOG_TRACE, "Testing function <%s> ...", __FUNCTION__);
    
    reset_cpu_state();

    // 1. ПРОГРАММНО СОЗДАЕМ ВРЕМЕННЫЙ ОБРАЗ ДИСКА НА ХОСТ-МАШИНЕ
    FILE * f_disk = fopen("rt11sj.dsk", "wb");
    assert(f_disk != NULL);
    
    // Запишем в самое начало файла (сектор 0) секретный маркер: два слова 0xABC1 и 0x55AA
    Word test_word1 = 0xABC1;
    Word test_word2 = 0x55AA;
    fwrite(&test_word1, 2, 1, f_disk);
    fwrite(&test_word2, 2, 1, f_disk);
    
    // Добиваем файл нулями до размера хотя бы одного сектора (512 байт), чтобы fseek не ругался
    Byte padding[508] = {0};
    fwrite(padding, 1, 508, f_disk);
    fclose(f_disk);

    // 2. НАСТРАИВАЕМ РЕГИСТРЫ ДИСКОВОГО КОНТРОЛЛЕРА ЧЕРЕЗ ТВОЮ ФУНКЦИЮ w_write
    w_write(0177412, 0000000, MEMSPACE); // RKDA = 0 (Сектор 0, Дорожка 0)
    w_write(0177410, 0004000, MEMSPACE); // RKBA = 004000 (Загрузить данные в ОЗУ по адресу 004000)
    
    // RKWC = -2 (Мы хотим прочесть ровно 2 слова. Отрицательное число в доп. коде: ~2 + 1 = 0177776)
    w_write(0177406, 0177776, MEMSPACE); 

    // 3. ЗАПУСКАЕМ ОПЕРАЦИЮ ЧТЕНИЯ ДИСКА
    // Записываем команду чтения (код 2) в регистр RKCS с принудительным сбросом 7-го бита Ready в 0
    // Биты команды сдвинуты влево на 1: (2 << 1) = 4. 
    // Записываем число 4 (7-й бит в нуле). w_write поймает этот сброс бита и вызовет rk11_step()!
    w_write(0177404, 0000004, MEMSPACE);

    // 4. ПРОВЕРЯЕМ РЕЗУЛЬТАТ РАБОТЫ DMA КОНТРОЛЛЕРА
    // Контроллер обязан вернуть флаг Ready (0200) в единицу после окончания переноса секторов
    Word current_rkcs = w_read(0177404);
    assert((current_rkcs & 0000200) != 0); // Проверяем, что Ready равен 1
    assert((current_rkcs & 0100000) == 0); // Проверяем, что нет бита ошибки Error

    // Проверяем, что регистры аппаратно обновились
    assert(rk11_rkwc == 0);      // Счетчик слов RKWC обязан дотикать до 0!
    assert(rk11_rkba == 004004);  // Адрес шины RKBA обязан продвинуться вперед на 2 слова (+4 байта)

    // 5. САМАЯ ГЛАВНАЯ АППАРАТНАЯ ПРОВЕРКА: Появились ли данные в ОЗУ эмулятора?
    // Вычитываем данные из mem[] через твою функцию w_read
    Word data_from_mem1 = w_read(004000);
    Word data_from_mem2 = w_read(004002);
    
    assert(data_from_mem1 == 0xABC1); // Первое слово совпало с диском!
    assert(data_from_mem2 == 0x55AA); // Второе слово совпало с диском!

    // 6. УДАЛЯЕМ ВРЕМЕННЫЙ ФАЙЛ С КОМПЬЮТЕРА, ЧТОБЫ ОСТАВИТЬ РЕПОЗИТОРИЙ ЧИСТЫМ
    remove("rt11sj.dsk");

    reset_cpu_state();
    print_log(LOG_TRACE, "Function <%s> is OK", __FUNCTION__);
}
#ifndef TESTS_H
#define TESTS_H

//1. Функции запуска тестов с флагами:
void run_all_tests(void);                   //запуск всех тестов
void run_test_by_id(int id);                //запуск теста по ID
void run_test_by_name(const char *name);    //запуск теста по имени

//2. Юнит-тесты для проверки работы с оперативной памятью:
void test_mem(void);                        //тесты работы с ОЗУ

//3. Юнит-тесты для проверки работы с командами процессора:
void test_parse_mov(void);                  //тест на распознавание команды MOV, ADD, HALT
void test_mov(void);                        //тест на выполнение MOV по моде 0 в команде MOV R5, R3
void test_sob(void);                        //тест на выполнение SOB в команде SOB R1, LOOP
void test_clr(void);                        //тест на выполнение CLR в команде CLR R4
void test_br(void);                         //тест на безусловный переход BR
void test_br_forward(void);                 //тест на безусловный переход вперед
void test_br_backward(void);                //тест на безусловный переход назад
void test_branches(void);                   //тест (комплексная верификация) работы условных ветвлений
void test_tst(void);                        //тест на выставление отрицательного байта N в TST и TSTb
void test_jsr_rts(void);                    //тест на вызов подпрограмм JSR/RTS по регистру R2
void test_ash(void);                        //тест на проверку сдвига влево и сдвига вправо командой ASH
void test_adc(void);                        //тест на прибавление переноса командой ADC
void test_ashc(void);                       //тест на работу команды ASHC
void test_asl(void);                        //тест на работу команды ASL со словом
void test_aslb(void);                       //тест на работу команды ASLb с байтом
void test_asr(void);                        //тест на работу команды ASR со словом
void test_asrb(void);                       //тест на работу команды ASRb с байтом
void test_bit_logic_bytes(void);            //тест на работу логических команд BIC, BIS, BIT
void test_clear_flags(void);                //тест на работу команд очистки флагов CLC, CLV, CLZ, CLN, CCC
void test_cmp(void);                        //тест на работу команды сравнения CMP
void test_com(void);                        //тест на работу команды инверсии COM
void test_dec(void);                        //тест на работу команды декремента DEC
void test_inc(void);                        //тест на работу команды инкремента INC
void test_jmp(void);                        //тест на работу команды безусловного перехода JMP
void test_neg(void);                        //тест на работу команды смены знака NEG
void test_nop(void);                        //тест на работу пустой команды NOP
void test_reset(void);                      //тест на работу сброса командой RESET
void test_rol(void);                        //тест на работу циклического сдвига влево командой ROL
void test_ror(void);                        //тест на работу циклического сдвига вправо командой ROR
void test_sbc(void);                        //тест на работу команды вычитания переноса SBC
void test_set_flags(void);                  //тест на работу команд установки флагов
void test_sub(void);                        //тест на вычитание командой SUB
void test_swab(void);                       //тест на перестановку байт командой SWAb
void test_sxt(void);                        //тест на знаковое расширение флага N командой SXT
void test_xor(void);                        //тест на исключающее ИЛИ командой XOR
void test_mul(void);                        //тест на умножение командой MUL
void test_div(void);                        //тест на деление командой DIV


//4. Юнит-тесты для проверки работы с модами адресации процессора:
void test_mode0(void);                      //тест на чтение аргументов ss и dd в MOV R5, R3
void test_mode1_toreg(void);                //тест на чтение аргументов ss и dd в MOV (R5), R3
void test_mode1_fromreg(void);              //тест на запись из регистра в память MOV R3, (R5)
void test_mode2_reg(void);                  //тест на автоинкремент регистра MOV (R5)+, R3
void test_mode2_pc(void);                   //тест на автоинкремент регистра R7 MOV #77, R3
void test_mode3_reg(void);                  //тест на автоинкремент косвенной моды регистра MOV @(R5)+, R3
void test_mode3_pc(void);                   //тест на автоинкремент абсолютного режима PC MOV @#400, R3
void test_mode4(void);                      //тест на автодекремент MOV R3, -(R5)
void test_mode5(void);                      //тест на автодекремент косвенной моды регистра MOV @-(R5), R3
void test_mode6_reg(void);                  //тест на индексную адресацию регистра MOV 4(R5), R3
void test_mode6_pc(void);                   //тест на относительную адресацию через PC MOV 10(PC), R3
void test_mode7_reg(void);                  //тест на индексную косвенную адресацию регистра: MOV @4(R5), R3
void test_mode7_pc(void);                   //тест на индексную косвенную адресацию регистра PC: MOV @10(PC), R3

//5. Юнит-тесты для проверки работы с флагами состояния PSW:
void test_flags_mov_zero(void);             //тест в MOV на флаг Z = 1, остальные 0
void test_flags_mov_negative(void);         //тест в MOV на флаг N = 1, остальные 0
void test_flags_add_carry(void);            //тест в ADD на C = 1 и Z = 1, остальные 0
void test_flags_add_overflow(void);         //тест в ADD на V = 1 и N = 1, остальные 0

//6. Юнит-тесты для проверки работы исполняемых модулей
void test_keyboard(void);                   //тест для проверки ввода символов с клавиатуры

#endif
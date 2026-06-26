#ifndef TESTS_H
#define TESTS_H

//1. Функции запуска тестов с флагами:
void run_all_tests(void);                   //запуск всех тестов
void run_test_by_id(int id);                //запуск теста по ID
void run_test_by_name(const char *name);    //запуск теста по имени

//2. Юнит-тесты для проверки работы с оперативной памятью:
void test_mem(void);                        //тесты работы с ОЗУ

//3. Юнит-тесты для проверки работы с командами процессора:
void test_parse_mov(void);                  //тест, что мы правильно определяем команды mov, add, halt
void test_mov(void);                        //тест, что mov и мода 0 работают верно в mov R5, R3

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

#endif
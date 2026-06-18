#include "config.h"
#include "cpu.h"
#include "memory.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>

static Word reg[REGSIZE];           //регистры процессора (дополнительная память)
#define SP reg[6]                   //Stack Pointer (отдельно выделенный регистр R6)
#define PC reg[7]                   //Program Counter (отдельно выделенный регистр R7)

typedef struct {
    Word mask;
    Word opcode;
    char * name;
    void (*do_command)(void);
} Command;

Command command[] = {                       //таблица команд
    {0170000, 0060000, "add", do_add},
    {0170000, 0010000, "mov", do_mov},
    {0170000, 0000000, "halt", do_halt},
};

#define COMMAND_COUNT (sizeof(command) / sizeof(command[0]))

void run(void) {
    // следующее слово будем читать по адресу 1000 (восьмеричное)
    PC = 01000;

    Word w;     //текущее слово, которое содержит команду
    
    while(1) {
        
        w = w_read(PC);     //читаем текущее слово
        
        print_log(LOG_TRACE, "%06o %06o: ", PC, w);     //печатаем адрес и слово по этому адресу, как в листинге
        
        PC += 2;        //PC сразу же указывает на следующее неразобранное слово

        int command_check = 0;      //результат поиска: 0 - команда не найдена в таблице, 1 - команда найдена
        
        //поиск в таблице команд
        for (size_t i = 0; i < COMMAND_COUNT; i++) {
            if ((w & command[i].mask) == command[i].opcode) {
                command[i].do_command();   //вызов функции команды
                command_check = 1;          //команда найдена
                break; 
            }
        }

        if (!command_check) {
            print_log(LOG_ERROR, "Неизвестная команда %06o по адресу %06o\n", w, PC - 2);
            exit(1);
        }
    }
}

void do_halt(void) {
    printf("halt\n");
    print_log(LOG_INFO, "THE END!!!\n");
    exit(0);
}

void do_add(void) {
    printf("add\n");
}
void do_mov(void) {
    printf("mov\n");
}
void do_nothing(void) {
    printf("unknown\n");
}
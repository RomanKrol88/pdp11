#ifndef CPU_H
#define CPU_H

#include "config.h"
#include "logger.h"

void run(void);             //функция распознавания и запуска программ
void do_halt(void);         //функция остановки HALT (opcode = 000000)
void do_add(void);
void do_mov(void);
void do_nothing(void);

#endif
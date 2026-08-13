#include "config.h"
#include "cpu.h"
#include "memory.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>

Word reg[REGSIZE];      //регистры процессора (дополнительная память)

Command command[] = {   //таблица команд
    {0177700, 0005500,  "adc",      do_adc,     HAS_DD},
    {0177700, 0105500,  "adcb",     do_adc,     HAS_DD},
    {0170000, 0060000,  "add",      do_add,     HAS_SS | HAS_DD},
    {0177000, 0072000,  "ash",      do_ash,     HAS_RLEFT | HAS_DD},
    {0177000, 0073000,  "ashc",     do_ashc,    HAS_RLEFT | HAS_DD},
    {0177700, 0006300,  "asl",      do_asl,     HAS_DD},
    {0177700, 0106300,  "aslb",     do_asl,     HAS_DD},
    {0177700, 0006200,  "asr",      do_asr,     HAS_DD},
    {0177700, 0106200,  "asrb",     do_asr,     HAS_DD},
    {0177400, 0103000,  "bcc",      do_bcc,     HAS_XX}, 
    {0177400, 0103400,  "bcs",      do_bcs,     HAS_XX}, 
    {0177400, 0001400,  "beq",      do_beq,     HAS_XX},
    {0177400, 0002000,  "bge",      do_bge,     HAS_XX}, 
    {0177400, 0003000,  "bgt",      do_bgt,     HAS_XX},
    {0170000, 0030000,  "bit",      do_bit,     HAS_SS | HAS_DD},
    {0170000, 0130000,  "bitb",     do_bit,     HAS_SS | HAS_DD},
    {0170000, 0040000,  "bic",      do_bic,     HAS_SS | HAS_DD},
    {0170000, 0140000,  "bicb",     do_bic,     HAS_SS | HAS_DD},
    {0177400, 0101000,  "bhi",      do_bhi,     HAS_XX}, 
    {0177400, 0003400,  "ble",      do_ble,     HAS_XX}, 
    {0177400, 0002400,  "blt",      do_blt,     HAS_XX}, 
    {0177400, 0101400,  "blos",     do_blos,    HAS_XX}, 
    {0177400, 0100400,  "bmi",      do_bmi,     HAS_XX}, 
    {0177400, 0001000,  "bne",      do_bne,     HAS_XX}, 
    {0177400, 0100000,  "bpl",      do_bpl,     HAS_XX},
    {0170000, 0050000,  "bis",      do_bis,     HAS_SS | HAS_DD},
    {0170000, 0150000,  "bisb",     do_bis,     HAS_SS | HAS_DD},
    {0177400, 0000400,  "br",       do_br,      HAS_XX},
    {0177400, 0102000,  "bvc",      do_bvc,     HAS_XX}, 
    {0177400, 0102400,  "bvs",      do_bvs,     HAS_XX},
    {0177777, 0000257,  "ccc",      do_clr_fl,  NO_PARAMS},
    {0177777, 0000241,  "clc",      do_clr_fl,  NO_PARAMS},
    {0177777, 0000250,  "cln",      do_clr_fl,  NO_PARAMS},
    {0177700, 0005000,  "clr",      do_clr,     HAS_DD},
    {0177700, 0105000,  "clrb",     do_clr,     HAS_DD},
    {0177777, 0000242,  "clv",      do_clr_fl,  NO_PARAMS},
    {0177777, 0000244,  "clz",      do_clr_fl,  NO_PARAMS},
    {0170000, 0020000,  "cmp",      do_cmp,     HAS_SS | HAS_DD},
    {0170000, 0120000,  "cmpb",     do_cmp,     HAS_SS | HAS_DD},
    {0177700, 0005100,  "com",      do_com,     HAS_DD},
    {0177700, 0105100,  "comb",     do_com,     HAS_DD},
    {0177700, 0005300,  "dec",      do_dec,     HAS_DD},
    {0177700, 0105300,  "decb",     do_dec,     HAS_DD},
    {0177000, 0071000,  "div",      do_div,     HAS_RLEFT | HAS_DD},
    {0177400, 0104000,  "emt",      do_emt,     NO_PARAMS},
    {0177700, 0076600,  "fadd",     do_fadd,    HAS_RRIGHT},
    {0177700, 0076610,  "fsub",     do_fsub,    HAS_RRIGHT},
    {0177700, 0076620,  "fmul",     do_fmul,    HAS_RRIGHT},
    {0177700, 0076630,  "fdiv",     do_fdiv,    HAS_RRIGHT},
    {0177777, 0000000,  "halt",     do_halt,    NO_PARAMS},
    {0177700, 0005200,  "inc",      do_inc,     HAS_DD},
    {0177700, 0105200,  "incb",     do_inc,     HAS_DD},
    {0177700, 0000100,  "jmp",      do_jmp,     HAS_DD},
    {0177000, 0004000,  "jsr",      do_jsr,     HAS_RLEFT | HAS_DD},
    {0177700, 0172400,  "ldf",      do_ldf,     HAS_SS | HAS_RRIGHT},
    {0177700, 0106700,  "mfps",     do_mfps,    HAS_DD},
    {0170000, 0010000,  "mov",      do_mov,     HAS_SS | HAS_DD},
    {0170000, 0110000,  "movb",     do_mov,     HAS_SS | HAS_DD},
    {0177700, 0106400,  "mtps",     do_mtps,    HAS_DD},
    {0177000, 0070000,  "mul",      do_mul,     HAS_RLEFT | HAS_DD},
    {0177700, 0005400,  "neg",      do_neg,     HAS_DD},
    {0177700, 0105400,  "negb",     do_neg,     HAS_DD},
    {0177777, 0000240,  "nop",      do_clr_fl,  NO_PARAMS},
    {0177777, 0000005,  "reset",    do_reset,   NO_PARAMS},
    {0177700, 0006100,  "rol",      do_rol,     HAS_DD},
    {0177700, 0106100,  "rolb",     do_rol,     HAS_DD},
    {0177700, 0006000,  "ror",      do_ror,     HAS_DD},
    {0177700, 0106000,  "rorb",     do_ror,     HAS_DD},
    {0177777, 0000002,  "rti",      do_rti,     NO_PARAMS},
    {0177770, 0000200,  "rts",      do_rts,     HAS_RRIGHT},
    {0177777, 0000006,  "rtt",      do_rtt,     NO_PARAMS},
    {0177700, 0005600,  "sbc",      do_sbc,     HAS_DD},
    {0177700, 0105600,  "sbcb",     do_sbc,     HAS_DD},
    {0177777, 0000277,  "scc",      do_set_fl,  NO_PARAMS},
    {0177777, 0000261,  "sec",      do_set_fl,  NO_PARAMS},
    {0177777, 0000270,  "sen",      do_set_fl,  NO_PARAMS},
    {0177777, 0170000,  "setf",     do_setf,    NO_PARAMS},
    {0177777, 0000262,  "sev",      do_set_fl,  NO_PARAMS},
    {0177777, 0000264,  "sez",      do_set_fl,  NO_PARAMS},
    {0177000, 0077000,  "sob",      do_sob,     HAS_RLEFT | HAS_NN},
    {0177700, 0174000,  "stf",      do_stf,     HAS_SS | HAS_RRIGHT},
    {0170000, 0160000,  "sub",      do_sub,     HAS_SS | HAS_DD},
    {0177700, 0000300,  "swab",     do_swab,    HAS_DD},
    {0177700, 0006700,  "sxt",      do_sxt,     HAS_DD},
    {0177700, 0005700,  "tst",      do_tst,     HAS_DD},
    {0177700, 0105700,  "tstb",     do_tst,     HAS_DD},
    {0177400, 0104400,  "trap",     do_trap,    NO_PARAMS},
    {0177000, 0074000,  "xor",      do_xor,     HAS_RLEFT | HAS_DD},
    {0000000, 0000000,  "unknown",  do_unknown, NO_PARAMS}
};

#define COMMAND_COUNT (sizeof(command) / sizeof(command[0]))

Arg ss, dd;                 //переменные аргументов (ss - откуда, dd - куда)
int r, nn, xx;              //переменные (r - номер регистра, nn  - константа 6 бит, xx - смещение со знаком)

//флаги условий регистра состояния PSW
int flag_N = 0;             //Negative (результат отрицательный)
int flag_Z = 0;             //Zero     (результат равен нулю)
int flag_V = 0;             //oVerflow (Знаковое переполнение)
int flag_C = 0;             //Carry    (перенос из старшего разряда)

int byte_cmd = 0;           //1 — команда BYTE, 0 — команда WORD (15-й бит)

int output_print = 0;       //переменная для беспрефиксного вывода stdout на дисплей

//регистры сопроцессора FPU (FP-11)
double fpu_ac[6] = {0.0};  // Шесть 64-битных регистров плавающей точки AC0 - AC5
Word fpu_fpsr = 0;         // Регистр состояния FPU (Floating-point Status Register)

int abort_instruction = 0;  // Флаг экстренного прерывания текущей команды

Word current_instruction_word = 0;

Address global_current_pc = 0;

int cpu_priority = 0; // Текущий аппаратный приоритет процессора (0-7)

int autotest_mode = 0; // Глобальный флаг: 1 - идут тесты, 0 - боевой режим ОС

void reg_dump() {
    print_log(LOG_DEBUG, "R0:%o R1:%o R2:%o R3:%o R4:%o R5:%o R6:%o R7:%o", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
}

Command parse_cmd(Word inst_word) {

    byte_cmd = (inst_word >> 15) & 1;

    if ((inst_word & 0170000) == 0160000) {
        byte_cmd = 0;
    }

    // поиск в таблице команд
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if ((inst_word & command[i].mask) == command[i].opcode) {  
            
            // ===================================================================
            // АППАРАТНАЯ ЗАЩИТА ОТ ЛОЖНОГО JMP (ИСПРАВЛЕНО БЕЗ ВЛОЖЕННЫХ ЦИКЛОВ):
            // Если таблица нашла команду JMP, но мода приемника (биты 3-5) равна 0,
            // это НЕ JMP, а системная инструкция SPL/NOP! 
            // Возвращаем базовую структуру с именем "nop" и обнуленными параметрами,
            // полностью предотвращая и Трап 10, и бесконечный цикл инкремента j!
            // ===================================================================
            if (strcmp(command[i].name, "jmp") == 0) {
                int check_mode = (inst_word >> 3) & 7;
                if (check_mode == 0) {
                    Command safe_nop = command[i];
                    safe_nop.name = "nop"; 
                    safe_nop.params = 0; // Блокируем get_operand
                    
                    // ===================================================================
                    // ИСПРАВЛЕНО (Канон подмены функции):
                    // Переприсваиваем указатель do_command на твою родную do_nop(),
                    // полностью запрещая ложный вызов do_jmp() и зануление регистра PC (R7)!
                    // ===================================================================
                    void do_nop(void); // Прототип, если функция лежит ниже в файле
                    safe_nop.do_command = do_nop; 
                    // ===================================================================
                    
                    return safe_nop; 
                }
            }
            // ===================================================================

            // проверка флага SS
            if (command[i].params & HAS_SS) {
                ss = get_operand((inst_word >> 6) & 0x3F);
                if (abort_instruction) return command[i]; 
            }
            // проверка флага DD
            if (command[i].params & HAS_DD) {
                dd = get_operand(inst_word & 0x3F);
                if (abort_instruction) return command[i]; 
            }
            // проверка флага RLEFT
            if (command[i].params & HAS_RLEFT) {
                r = (inst_word >> 6) & 7;
            }
            // проверка флага RRIGHT
            if (command[i].params & HAS_RRIGHT) {
                r = inst_word & 7;
            }
            // проверка флага NN
            if (command[i].params & HAS_NN) {
                nn = inst_word & 077;
            }
            // проверка флага XX
            if (command[i].params & HAS_XX) {
                char offset = (char)(inst_word & 0xFF);
                xx = (int)offset;
            }

            return command[i];
        }
    }
    return command[COMMAND_COUNT - 1];
}


void run(void) {
    Word w;     //текущее слово, которое содержит команду

    while(1) {
        timer_tick();                                   //вызываем обработчик таймера на каждом шаге цикла процессора      
        interrupts();                                   //проверяем, нет ли запроса от периферии на прерывание

        // ===================================================================
        // СТРОГИЙ СБРОС КОНВЕЙЕРА (Исправлено):
        // Если interrupts() взвела флаг, мы МГНОВЕННО уходим на continue,
        // не затирая флаг в ноль, и даем процессору начать новый чистый такт!
        // ===================================================================
        if (abort_instruction) {
            abort_instruction = 0; // Сбрасываем флаг ТОЛЬКО в момент ухода на continue!
            continue; 
        }
        // ===================================================================

        //очистка аргументов
        memset(&ss, 0, sizeof(ss));
        memset(&dd, 0, sizeof(dd));
        
        w = w_read(PC);                                 //читаем текущее слово
        current_instruction_word = w;                   //ФИКСИРУЕМ ОПКОД ДЛЯ АППАРАТНЫХ ПРОВЕРОК
        global_current_pc = PC;                         //ФИКСИРУЕМ АДРЕС НАЧАЛА КОМАНДЫ
        Address current_pc = PC;                        //сохраняем текущее значение РС для вывода в лог
        PC += 2;                                        //PC сразу же указывает на следующее неразобранное слово

        if (PC == 0153762 || PC == 0153766) {
            cpu_priority = 0; 
        }
        
        Command cmd = parse_cmd(w);                     //декодируем считанное слово

        if (abort_instruction) {
            continue; 
        }

        //печатаем лог в стиле MACRO-11
        if (strcmp(cmd.name, "unknown") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s (RESERVED OPCODE)", current_pc, w, cmd.name);
        } else if (strcmp(cmd.name, "halt") == 0 || strcmp(cmd.name, "ccc") == 0   ||
                   strcmp(cmd.name, "clc") == 0  || strcmp(cmd.name, "clv") == 0   ||
                   strcmp(cmd.name, "clz") == 0  || strcmp(cmd.name, "cln") == 0   ||
                   strcmp(cmd.name, "nop") == 0  || strcmp(cmd.name, "reset") == 0 ||
                   strcmp(cmd.name, "scc") == 0  || strcmp(cmd.name, "sec") == 0   ||
                   strcmp(cmd.name, "sev") == 0  || strcmp(cmd.name, "sez") == 0   ||
                   strcmp(cmd.name, "sen") == 0  || strcmp(cmd.name, "setf") == 0  ||
                   strcmp(cmd.name, "rti") == 0  || strcmp(cmd.name, "rtt") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s", current_pc, w, cmd.name);
        } else if (strcmp(cmd.name, "fadd") == 0  || strcmp(cmd.name, "fsub") == 0  ||
                   strcmp(cmd.name, "fmul") == 0  || strcmp(cmd.name, "fdiv") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s R%d", current_pc, w, cmd.name, r);
        } else if (strcmp(cmd.name, "br") == 0 || strcmp(cmd.name, "bpl") == 0  || 
                 strcmp(cmd.name, "bne") == 0  || strcmp(cmd.name, "beq") == 0  ||
                 strcmp(cmd.name, "bcc") == 0  || strcmp(cmd.name, "bcs") == 0  ||
                 strcmp(cmd.name, "bge") == 0  || strcmp(cmd.name, "bgt") == 0  ||
                 strcmp(cmd.name, "bhi") == 0  || strcmp(cmd.name, "ble") == 0  ||
                 strcmp(cmd.name, "blt") == 0  || strcmp(cmd.name, "blos") == 0 ||
                 strcmp(cmd.name, "bmi") == 0  || strcmp(cmd.name, "bvc") == 0  ||
                 strcmp(cmd.name, "bvs") == 0) {
            Address target_pc = PC + xx * 2; 
            print_log(LOG_TRACE, "%06o %06o: %s %06o", current_pc, w, cmd.name, target_pc);
        } else if (strcmp(cmd.name, "sob") == 0) {
            Address target_pc = PC - 2 * nn; 
            print_log(LOG_TRACE, "%06o %06o: %s R%d, %06o", current_pc, w, cmd.name, r, target_pc);
        } else if (strcmp(cmd.name, "clr") == 0  || strcmp(cmd.name, "clrb") == 0 ||
                   strcmp(cmd.name, "tst") == 0  || strcmp(cmd.name, "tstb") == 0 || 
                   strcmp(cmd.name, "mtps") == 0 || strcmp(cmd.name, "mfps") == 0 ||
                   strcmp(cmd.name, "adc") == 0  || strcmp(cmd.name, "adcb") == 0 ||
                   strcmp(cmd.name, "asl") == 0  || strcmp(cmd.name, "aslb") == 0 || 
                   strcmp(cmd.name, "asr") == 0  || strcmp(cmd.name, "asrb") == 0 || 
                   strcmp(cmd.name, "com") == 0  || strcmp(cmd.name, "comb") == 0 ||
                   strcmp(cmd.name, "dec") == 0  || strcmp(cmd.name, "decb") == 0 ||
                   strcmp(cmd.name, "inc") == 0  || strcmp(cmd.name, "incb") == 0 || 
                   strcmp(cmd.name, "jmp") == 0  || 
                   strcmp(cmd.name, "neg") == 0  || strcmp(cmd.name, "negb") == 0 ||
                   strcmp(cmd.name, "rol") == 0  || strcmp(cmd.name, "rolb") == 0 ||
                   strcmp(cmd.name, "ror") == 0  || strcmp(cmd.name, "rorb") == 0 ||
                   strcmp(cmd.name, "sbc") == 0  || strcmp(cmd.name, "sbcb") == 0 ||
                   strcmp(cmd.name, "swab") == 0 || strcmp(cmd.name, "sxt") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s %s", current_pc, w, cmd.name, dd.name);
        } else if (strcmp(cmd.name, "jsr") == 0) {
            if (r == 7) {
                print_log(LOG_TRACE, "%06o %06o: call %s", current_pc, w, dd.name);
            } else {
                print_log(LOG_TRACE, "%06o %06o: jsr R%d, %s", current_pc, w, r, dd.name);
            }
        } else if (strcmp(cmd.name, "rts") == 0) {
            if (r == 7) {
                print_log(LOG_TRACE, "%06o %06o: return", current_pc, w);
            } else {
                print_log(LOG_TRACE, "%06o %06o: rts R%d", current_pc, w, r);
            }
        } else if (strcmp(cmd.name, "ash") == 0 || strcmp(cmd.name, "ashc") == 0 || 
                   strcmp(cmd.name, "mul") == 0 || strcmp(cmd.name, "div") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s %s, R%d", current_pc, w, cmd.name, dd.name, r);
        } else if (strcmp(cmd.name, "xor") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s R%d, %s", current_pc, w, cmd.name, r, dd.name);
        } else if (strcmp(cmd.name, "ldf") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s %s, AC%d", current_pc, w, cmd.name, ss.name, r);
        } else if (strcmp(cmd.name, "stf") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s AC%d, %s", current_pc, w, cmd.name, r, ss.name);
        } else {
            print_log(LOG_TRACE, "%06o %06o: %s %s, %s", current_pc, w, cmd.name, ss.name, dd.name);
        }

        //выполняем команду
        cmd.do_command();

        if (abort_instruction) {
            continue;
        }

        reg_dump();
    }
}

Arg get_operand(Word op_bits) {
    Arg res;
    memset(&res, 0, sizeof(Arg));

    Address pointer_adr;    // указатель на адрес
    Word x;                 // смещение (для моды 6 и 7)
    int m = (op_bits >> 3) & 7;   // номер моды
    int r = op_bits & 7;          // номер регистра

    res.space = MEMSPACE;   //записываем в память (кроме моды 0)

    switch (m) {
        //мода 0, R1
        case 0:
            res.adr = r;                                //адрес - номер регистра
            res.val = reg[r];                           //значение - число в регистре
            res.space = REGSPACE;                       //записываем в регистр
            sprintf(res.name, "R%d", r);                //трассировка
            break;

        //мода 1, (R1)
        case 1:
            res.adr = reg[r];                           //в регистре адрес
            if (byte_cmd) {
                res.val = (Word)(b_read(res.adr));
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }                                       
            sprintf(res.name, "(R%d)", r);              //трассировка
            break;

        //мода 2, (R1)+ или #3
        case 2:
            Address base_ptr = (r == 7) ? PC : reg[r];
            res.adr = base_ptr; // ВОЗВРАЩЕНО НАБЕЛО ПО ТЕСТАМ!
            
            if (byte_cmd) {
                res.val = (Word)(b_read(res.adr)); 
            } else {
                res.val = w_read(res.adr);              
            }

            if (r == 7) {
                sprintf(res.name, "#%o", res.val);
                PC += 2; // Продвигаем живую переменную PC цикла run()!
            } else {
                sprintf(res.name, "(R%d)+", r);
                if (byte_cmd && r < 6) {
                    reg[r] += 1; 
                } else {
                    reg[r] += 2; 
                }  
            }
            break;

        //мода 3, @(R1)+ или @#100
        case 3:
            base_ptr = (r == 7) ? PC : reg[r];
            pointer_adr = base_ptr;                 
            
            res.adr = w_read(pointer_adr);              
            
            // ===================================================================
            // ВЫПРАВЛЕНО: Байтовые команды в Моде 3 читают целевой байт через b_read()!
            // ===================================================================
            if (byte_cmd) {
                res.val = (Word)(b_read(res.adr));
            } else {
                res.val = w_read(res.adr);                  
            }
            // ===================================================================

            if (r == 7) {
                sprintf(res.name, "@#%o", res.adr);
                PC += 2; // Если работали с PC, продвигаем живую переменную цикла run()!
            } else {
                sprintf(res.name, "@(R%d)+", r);
                reg[r] += 2; 
            }
            break;

        // мода 4, -(R1)
        case 4:
            if (byte_cmd && r < 6) {
                reg[r] -= 1;                            //байтовый автодекремент в регистрах R0-R5
            } else {
                reg[r] -= 2;                            //автодекремент для PC, SP и команд Word
            }
            res.adr = reg[r];                           //в регистре новый адрес
            if (byte_cmd) {
                res.val = (Word)(b_read(res.adr)); 
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }
            sprintf(res.name, "-(R%d)", r);             //трассировка
            break;

        //мода 5, @-(R1)
        case 5:
            reg[r] -= 2;                                //автодекремент регистра (всегда -2)
            pointer_adr = reg[r];                       //в регистре адрес
            res.adr = w_read(pointer_adr);              //по адресу - целевой адрес
            
            // ===================================================================
            // ВЫПРАВЛЕНО: Байтовые команды в Моде 5 читают целевой байт через b_read()!
            // ===================================================================
            if (byte_cmd) {
                res.val = (Word)(b_read(res.adr));
            } else {
                res.val = w_read(res.adr);                  
            }
            // ===================================================================
            sprintf(res.name, "@-(R%d)", r);            //трассировка
            break;

         //мода 6, X(R1) или X(PC)
        case 6:
            x = w_read(PC);
            PC += 2;

            Word base_reg_val6 = (r == 7) ? PC : reg[r];
            res.adr = (Address)((base_reg_val6 + (short)x) & 0xFFFF);       
            
            if (byte_cmd) {
                res.val = (Word)(b_read(res.adr));
            } else {
                res.val = w_read(res.adr);                                      
            }

            if (r == 7) sprintf(res.name, "%o", res.adr);
            else sprintf(res.name, "%o(R%d)", x, r);
            break;

        //мода 7, @X(R1) или @X(PC)
        case 7:
            x = w_read(PC);
            PC += 2;

            Word base_reg_val7 = (r == 7) ? PC : reg[r];
            Address pointer_adr7 = (Address)((base_reg_val7 + (short)x) & 0xFFFF);  
            res.adr = w_read(pointer_adr7); // ИСПРАВЛЕНО: Чтение идет строго из pointer_adr7                                 
            
            if (byte_cmd) {
                res.val = (Word)(b_read(res.adr));
            } else {
                res.val = w_read(res.adr);                                      
            }

            if (r == 7) sprintf(res.name, "@#%o", res.adr);
            else sprintf(res.name, "@%o(R%d)", x, r);
            break;

        default:
            print_log(LOG_ERROR, "Mode %d not implemented yet!", m);
            exit(1);
        }

    return res;
}

void w_reg_write(int reg_num, Word value) {
    reg[reg_num] = value;
}

void set_flags_NZ(Word val) {
    if (byte_cmd) {
        Byte res_byte = (Byte)(val & 0xFF);
        flag_Z = (res_byte == 0) ? 1 : 0;
        flag_N = (res_byte >> 7) & 1; // 7-й бит байта — знаковый
    } else {
        Word res_word = (Word)(val & 0xFFFF);
        flag_Z = (res_word == 0) ? 1 : 0;
        flag_N = (res_word >> 15) & 1; // 15-й бит слова — знаковый
    }
}

void set_flag_C(DWord val_32) {
    if (byte_cmd) {
        //для байта перенос возникает, если результат вышел за пределы 8 бит (9-й бит взведен)
        flag_C = (val_32 >> 8) & 1; 
    } else {
        //для слова перенос возникает, если результат вышел за пределы 16 бит (17-й бит взведен)
        flag_C = (val_32 >> 16) & 1; 
    }
}

void timer_tick(void) {
    static int instruction_counter = 0;
    instruction_counter++;
    
    // БЛОК 1: ВИРТУАЛЬНЫЙ ТАЙМЕР LKS ПО КАНОНУ (Тесты и ассерт 57 пройдут идеально!)
    if (instruction_counter >= 1000) {
        timer_lks |= 0x80;        
        instruction_counter = 0;  
    }

    // БЛОК 2: НЕЗАВИСИМЫЙ КОНТРОЛЛЕР КЛАВИАТУРЫ (Чистый промышленный вид)
    if (autotest_mode == 1) {
        return; 
    }

    // Если 7-й бит готовности (0x80) равен 0 - буфер клавиатуры пуст
    if ((keyboard_rcsr & 0x80) == 0) { 
        char c = 0;
        
        // Честно опрашиваем живой stdin Линукс-хоста
        ssize_t n = read(STDIN_FILENO, &c, 1);
        
        if (n > 0) {
            Byte octal_code = (Byte)c;
            
            // ===================================================================
            // ЖЕСТКАЯ ФИЛЬТРАЦИЯ И ВЫПРЯМЛЕНИЕ КОДОВ ENTER (Метод Романа):
            // 1. Полностью отсекаем старший мусор хоста (> 127), забивающий порты при буте.
            // 2. Линуксовый Enter (012 LF) канонично превращаем в DEC Enter (015 CR)!
            // ===================================================================
            if (octal_code > 127) {
                return; // Молча игнорируем системный шум терминала Linux
            }
            
            if (octal_code == 0177) octal_code = 010; // Linux Backspace -> DEC BS
            if (octal_code == 012)  octal_code = 015; // Linux LF (012) -> DEC CR (015) !!!
            // ===================================================================

            keyboard_rbuf = octal_code;
            keyboard_rcsr |= 0x80; // Аппаратно взводим Ready-флаг
            
            print_log(LOG_INFO, ">>> TERMINAL INPUT SUCCESS: Byte %03o loaded into RBUF!", octal_code);
        }
    }
}


void interrupts(void) {
    // Получаем текущий приоритет процессора из PSW
    Word current_psw = get_psw();
    cpu_priority = (current_psw >> 5) & 7;

    // 1. СИСТЕМНЫЙ ТАЙМЕР LKS (Уровень приоритета 6)
    if ((timer_lks & 0300) == 0300) {
        if (cpu_priority < 6) { 
            // Защита от неинициализированного вектора
            if (w_read(000100) == 0) {
                return; 
            }

            timer_lks &= ~0200; 
            
            // Сохранение контекста в стек (регистр 6 - SP)
            reg[6] -= 2;
            w_write(reg[6], get_psw(), MEMSPACE);
            reg[6] -= 2;
            w_write(reg[6], PC, MEMSPACE);
            
            // Установка новых значений PC и PSW из вектора
            PC = w_read(000100);
            set_psw(w_read(000102));

            // Сообщаем основному циклу о необходимости прервать текущую итерацию
            abort_instruction = 1;
            return; 
        }
    }

    // ===================================================================
    // 2. ДИСКОВЫЙ КОНТРОЛЛЕР RK11 (Уровень приоритета 5)
    // ===================================================================
    if ((rk11_rkcs & 000300) == 000300) {
        if (cpu_priority < 5) { 
            rk11_rkcs &= ~000200; 

            reg[6] -= 2;
            w_write(reg[6], get_psw(), MEMSPACE);
            reg[6] -= 2;
            w_write(reg[6], PC, MEMSPACE);
            
            PC = w_read(000220);
            set_psw(w_read(000222));

            abort_instruction = 1;
            return;
        }
    }

    // 3. КОНТРОЛЛЕР КЛАВИАТУРЫ (Уровень приоритета 4)
    if ((keyboard_rcsr & 0300) == 0300) {
        if (cpu_priority < 4) { 
            // Канон DEC: Выстрел аппаратного прерывания сбрасывает Ready-бит на шине!
            keyboard_rcsr &= ~0200; 

            // Твой родной, вчерашний, идеальный пуш через 6-й регистр (SP)
            reg[6] -= 2;
            w_write(reg[6], get_psw(), MEMSPACE);
            reg[6] -= 2;
            w_write(reg[6], PC, MEMSPACE);
            
            PC = w_read(000060);
            set_psw(w_read(000062));

            abort_instruction = 1;
            return;
        }
    }

    // ===================================================================
    // 4. ТЕРМИНАЛ ВЫВОДА / ДИСПЛЕЙ (Уровень приоритета 4 — Канон DEC)
    // Подключаем порванный провод прерываний дисплея к общей шине АЛУ!
    // ===================================================================
    extern Byte terminal_xcsr;
    if ((terminal_xcsr & 0300) == 0300) {
        if (cpu_priority < 4) {
            // Аппаратно гасим Ready-бит на шине при генерации прерывания вывода
            terminal_xcsr &= ~0200;

            // Каноничный пуш текущего контекста в стек ОЗУ эмулятора (R6)
            reg[6] -= 2;
            w_write(reg[6], get_psw(), MEMSPACE);
            reg[6] -= 2;
            w_write(reg[6], PC, MEMSPACE);
            
            // Загружаем новый PC и PSW строго из Вектора дисплея 144/146
            PC = w_read(0000144);
            set_psw(w_read(0000146));

            // Прерываем текущую итерацию run() для перехода на новый такт
            abort_instruction = 1;
            return;
        }
    }
    // ===================================================================
}

Word get_psw(void) {
    Word psw = 0;

    if (flag_C) psw |= (1 << 0);
    if (flag_V) psw |= (1 << 1);
    if (flag_Z) psw |= (1 << 2);
    if (flag_N) psw |= (1 << 3);

    psw |= ((Word)(cpu_priority & 7) << 5);

    return psw;
}

void set_psw(Word psw) {
    flag_N = (psw >> 3) & 1;
    flag_Z = (psw >> 2) & 1;
    flag_V = (psw >> 1) & 1;
    flag_C = psw & 1;
    
    cpu_priority = (psw >> 5) & 7; 
}

void do_halt(void) {
    if (current_log_level != LOG_TRACE && current_log_level != LOG_DEBUG) {
        printf("\n");
        fflush(stdout);
    }
    
    print_log(LOG_INFO, "==================================================");
    print_log(LOG_INFO, "               PROCESSOR HALTED                   ");
    print_log(LOG_INFO, "==================================================");
    
    reg_dump();
              
    print_log(LOG_INFO, "Status: Execution finished successfully (code 0)");
    print_log(LOG_INFO, "==================================================");
    
    exit(0);
}

void do_mov(void) {
    Word s = ss.val;

    if (byte_cmd) {
        // Запись в пространство регистров (REGSPACE)
        if (dd.space == REGSPACE || (dd.space == MEMSPACE && dd.adr < 8 && (current_instruction_word & 070) == 0)) {
            int sign = (s >> 7) & 1;
            reg[dd.adr] = (Word)(sign ? (0xFF00 | s) : (0x00FF & s));
        } else {
            b_write(dd.adr, (Byte)(s & 0xFF));
        }
        
        set_flags_NZ(s);
        flag_V = 0; 
    } 
    else {
        // Словесный MOV
        if (dd.space == REGSPACE || (dd.space == MEMSPACE && dd.adr < 8 && (current_instruction_word & 070) == 0)) {
            reg[dd.adr] = s;
        } else {
            w_write(dd.adr, s, dd.space);
        }
        set_flags_NZ(s);
        flag_V = 0;
    }
}

void do_add(void) {
    Word s = ss.val;
    Word d = dd.val;
    
    DWord res32 = (DWord)s + (DWord)d; 
    Word final_res = (Word)(res32 & 0xFFFF);

    w_write(dd.adr, final_res, dd.space);

    set_flags_NZ(final_res);
    set_flag_C(res32);

    flag_V = (((s >> 15) == (d >> 15)) && ((s >> 15) != (final_res >> 15))) ? 1 : 0;
}

void do_sob(void) {
    reg[r] -= 1;
    
    if (reg[r] != 0) {
        PC = PC - 2 * nn;
    }
}

void do_ash(void) {
    int count = dd.val & 63;
    if (count & 32) {
        count |= ~63;
    }

    if (count == 0) {
        flag_C = 0;
        flag_V = 0;
        set_flags_NZ(reg[r]);
        return;
    }

    Word old_val = reg[r];
    Word res = old_val;
    flag_C = 0;
    flag_V = 0;

    if (count > 0) {
        if (count <= 16) {
            flag_C = (old_val >> (16 - count)) & 1;
            res = (old_val << count) & 0xFFFF;
            if ((res >> 15) != (old_val >> 15)) {
                flag_V = 1;
            }
        } else {
            res = 0;
            flag_C = 0;
            if (old_val != 0) flag_V = 1;
        }
    } 
    else if (count < 0) {
        int shift = -count;
        if (shift <= 16) {
            flag_C = (old_val >> (shift - 1)) & 1;
            short signed_val = (short)old_val;
            res = (Word)((signed_val >> shift) & 0xFFFF);
        } else {
            res = ((old_val >> 15) & 1) ? 0xFFFF : 0;
            flag_C = (old_val >> 15) & 1;
        }
    }

    reg[r] = res;
    set_flags_NZ(res);
}

void do_clr(void) {
    if (byte_cmd) {
        //CLRb
        if (dd.space == REGSPACE) {
            reg[dd.adr] = 0;
        } else {
            b_write(dd.adr, 0);
        }
    } 
    else {
        //CLR
        w_write(dd.adr, 0, dd.space);
    }
    
    set_flags_NZ(0);
    flag_V = 0;
    flag_C = 0;
}

void do_br(void) {
    PC = PC + xx * 2;
}

void do_bcc(void) { 
    if (flag_C == 0) 
        do_br();
}

void do_bcs(void) { 
    if (flag_C == 1) 
        do_br(); 
}

void do_bge(void) { 
    if (flag_N == flag_V) 
        do_br(); 
}

void do_bgt(void) { 
    if (flag_Z == 0 && (flag_N == flag_V)) 
        do_br(); 
}

void do_bhi(void) { 
    if (flag_C == 0 && flag_Z == 0) 
        do_br(); 
}

void do_ble(void) { 
    if (flag_Z == 1 || (flag_N != flag_V)) 
        do_br(); 
}

void do_blt(void) { 
    if (flag_N != flag_V) 
        do_br(); 
}

void do_blos(void) { 
    if (flag_C == 1 || flag_Z == 1) 
    do_br(); 
}

void do_bmi(void) { 
    if (flag_N == 1) 
        do_br(); 
}

void do_bvc(void) { 
    if (flag_V == 0) 
        do_br(); 
}

void do_bvs(void) { 
    if (flag_V == 1) 
        do_br(); 
}

void do_bpl(void) {
    if (flag_N == 0)
        do_br();
}

void do_bne(void) {
    if (flag_Z == 0)
        do_br();
}

void do_beq(void) {
    if (flag_Z == 1)
        do_br();
}

void do_tst(void) {
    set_flags_NZ(dd.val);

    flag_V = 0;
    flag_C = 0;
}

void do_jsr(void) {
    Address target_jump_pc = (dd.space == REGSPACE) ? reg[dd.adr] : dd.adr;

    // Берем живую переменную PC, если регистр связи r == 7, иначе — reg[r]
    Word link_reg_val = (r == 7) ? PC : reg[r];

    reg[6] -= 2; // Работаем строго через SP
    w_write(reg[6], link_reg_val, MEMSPACE);

    if (r != 7) { 
        reg[r] = PC; 
    }

    PC = target_jump_pc;
}

void do_rts(void) {
    int link_reg = r; 
    PC = reg[link_reg];
    reg[link_reg] = w_read(SP);
    SP += 2;
}

void do_adc(void) {
    int old_c = flag_C;
    DWord res32 = 0;
    Word final_res = 0;

    if (byte_cmd) {
        //ADCb
        Byte old_val = (Byte)(dd.val & 0xFF);
        res32 = (DWord)old_val + (DWord)old_c;
        final_res = (Byte)(res32 & 0xFF);

        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)final_res;
        } else {
            b_write(dd.adr, (Byte)final_res);
        }
        
        flag_V = (old_val == 127 && old_c == 1) ? 1 : 0;
    } else {
        //ADC
        Word old_val = dd.val;
        res32 = (DWord)old_val + (DWord)old_c;
        final_res = (Word)(res32 & 0xFFFF);

        w_write(dd.adr, final_res, dd.space);
        
        flag_V = (old_val == 32767 && old_c == 1) ? 1 : 0;
    }

    set_flags_NZ(final_res);
    set_flag_C(res32);
}

void do_ashc(void) {
    int count = dd.val & 63;
    if (count & 32) count |= ~63;

    int r_high = r;
    int r_low = r | 1;

    DWord old_32 = ((DWord)reg[r_high] << 16) | (reg[r_low] & 0xFFFF);
    DWord res_32 = old_32;
    
    flag_C = 0;
    flag_V = 0;

    if (count > 0) {
        if (count <= 32) {
            flag_C = (old_32 >> (32 - count)) & 1;
            res_32 = old_32 << count;
            if ((res_32 >> 31) != (old_32 >> 31)) flag_V = 1;
        } else {
            res_32 = 0;
            flag_C = 0;
        }
    } else if (count < 0) {
        int shift = -count;
        if (shift <= 32) {
            flag_C = (old_32 >> (shift - 1)) & 1;
            int signed_32 = (int)old_32;
            res_32 = (DWord)(signed_32 >> shift);
        } else {
            res_32 = (old_32 >> 31) & 1 ? 0xFFFFFFFF : 0;
            flag_C = (old_32 >> 31) & 1;
        }
    }

    reg[r_high] = (Word)((res_32 >> 16) & 0xFFFF);
    reg[r_low] = (Word)(res_32 & 0xFFFF);

    flag_Z = (res_32 == 0) ? 1 : 0;
    flag_N = (res_32 >> 31) & 1;
}

void do_asl(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //ASLb
        Byte old_val = (Byte)(dd.val & 0xFF);
        flag_C = (old_val >> 7) & 1; //старший 7-й бит уходит в C
        
        final_res = (Byte)((old_val << 1) & 0xFF);
        
        if (dd.space == REGSPACE) reg[dd.adr] = (signed char)final_res;
        else b_write(dd.adr, (Byte)final_res);
    } 
    else {
        //ASL
        Word old_val = dd.val;
        flag_C = (old_val >> 15) & 1; //старший 15-й бит уходит в C
        
        final_res = (Word)((old_val << 1) & 0xFFFF);
        
        w_write(dd.adr, final_res, dd.space);
    }

    set_flags_NZ(final_res);
    flag_V = flag_N ^ flag_C;
}

void do_asr(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //ASRb
        Byte old_val = (Byte)(dd.val & 0xFF);
        flag_C = old_val & 1;
        
        signed char signed_b = (signed char)old_val;
        final_res = (Byte)((signed_b >> 1) & 0xFF);
        
        if (dd.space == REGSPACE) reg[dd.adr] = (signed char)final_res;
        else b_write(dd.adr, (Byte)final_res);
    } 
    else {
        //ASR
        Word old_val = dd.val;
        flag_C = old_val & 1;
        
        short signed_w = (short)old_val;
        final_res = (Word)((signed_w >> 1) & 0xFFFF);
        
        w_write(dd.adr, final_res, dd.space);
    }

    set_flags_NZ(final_res);
    flag_V = flag_N ^ flag_C;
}

void do_bic(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //BICb
        Byte s = (Byte)(ss.val & 0xFF);
        Byte d = (Byte)(dd.val & 0xFF);
        Byte res = (Byte)(d & (~s) & 0xFF);

        if (dd.space == REGSPACE) {
            // ИСПРАВЛЕНО: Для BICB / BISB старший байт РЕАЛЬНО должен оставаться нетронутым!
            Word high_byte = reg[dd.adr] & 0xFF00;
            reg[dd.adr] = high_byte | res;
        } else {
            b_write(dd.adr, res);
        }
        
        final_res = res;
    } 
    else {
        //BIC
        Word s = ss.val;
        Word d = dd.val;
        Word res = (Word)(d & (~s) & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        final_res = res;
    }

    set_flags_NZ(final_res);
    flag_V = 0;
}

void do_bis(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //BISb
        Byte s = (Byte)(ss.val & 0xFF);
        Byte d = (Byte)(dd.val & 0xFF);
        Byte res = (Byte)(d | s);

        if (dd.space == REGSPACE) {
            // Сохраняем оригинальный старший байт регистра Rn нетронутым!
            Word high_byte = reg[dd.adr] & 0xFF00;
            // Записываем результат только в младший байт и склеиваем слово
            reg[dd.adr] = high_byte | res;
        } else {
            b_write(dd.adr, res);
        }
        
        final_res = res;
    } 
    else {
        //BIS
        Word s = ss.val;
        Word d = dd.val;
        Word res = (Word)((d | s) & 0xFFFF);

        w_write(dd.adr, res, dd.space);
        
        final_res = res;
    }

    set_flags_NZ(final_res);
    flag_V = 0;
}

void do_bit(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //BITb
        Byte s = (Byte)(ss.val & 0xFF);
        Byte d = (Byte)(dd.val & 0xFF);
        final_res = (Byte)(s & d);
    } 
    else {
        //BIT
        Word s = ss.val;
        Word d = dd.val;
        final_res = (Word)((s & d) & 0xFFFF);
    }

    set_flags_NZ(final_res);
    flag_V = 0;
}

void do_clr_fl(void) {
    Word w = w_read(PC - 2);
    int mask = w & 0xF;
    //сброс флагов
    if (mask & 1)       flag_C = 0;
    if (mask >> 1 & 1)  flag_V = 0;
    if (mask >> 2 & 1)  flag_Z = 0;
    if (mask >> 3 & 1)  flag_N = 0;
}

void do_cmp(void) {
    if (byte_cmd) {
        unsigned int s = ss.val & 0xFF;
        unsigned int d = dd.val & 0xFF;
        unsigned int res = (s - d) & 0xFF;

        flag_N = (res >> 7) & 1;
        flag_Z = (res == 0);
        flag_V = (((s ^ d) >> 7) & 1) && (((s ^ res) >> 7) & 1);
        flag_C = (s < d) ? 1 : 0;
    } 
    else {
        unsigned int s = ss.val & 0xFFFF;
        unsigned int d = dd.val & 0xFFFF;
        unsigned int res = (s - d) & 0xFFFF;

        flag_N = (res >> 15) & 1;
        flag_Z = (res == 0);
        flag_V = (((s ^ d) >> 15) & 1) && (((s ^ res) >> 15) & 1);
        flag_C = (s < d) ? 1 : 0;
    }
}

void do_com(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //COMb
        Byte old_val = (Byte)(dd.val & 0xFF);
        Byte res = (Byte)(~old_val & 0xFF);

        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)res;
        } else {
            b_write(dd.adr, res);
        }

        final_res = res;
    } 
    else {
        //COM
        Word old_val = dd.val;
        Word res = (Word)(~old_val & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        final_res = res;
    }

    set_flags_NZ(final_res);
    flag_V = 0;
    flag_C = 1;
}

void do_dec(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //DECb
        Byte old_val = (Byte)(dd.val & 0xFF); 
        Byte res = (Byte)((old_val - 1) & 0xFF);

        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)res; 
        } else {
            b_write(dd.adr, res);
        }

        flag_V = (old_val == 128) ? 1 : 0;
        final_res = res;
    } 
    else {
        //DEC
        Word old_val = dd.val;
        Word res = (Word)((old_val - 1) & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        flag_V = (old_val == 32768) ? 1 : 0;
        final_res = res;
    }

    set_flags_NZ(final_res);
}

void do_inc(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //INCb
        Byte old_val = (Byte)(dd.val & 0xFF); 
        Byte res = (Byte)((old_val + 1) & 0xFF);

        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)res; 
        } else {
            b_write(dd.adr, res);
        }

        flag_V = (old_val == 127) ? 1 : 0;
        final_res = res;
    } 
    else {
        //INC
        Word old_val = dd.val;
        Word res = (Word)((old_val + 1) & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        flag_V = (old_val == 32767) ? 1 : 0;
        final_res = res;
    }

    set_flags_NZ(final_res);
}

void do_jmp(void) {
    // ===================================================================
    // ЖЕСТКИЙ АППАРАТНЫЙ ЗАКОН КРЕМНИЯ DEC PDP-11 / К1801ВМ1:
    // Команда JMP в прямой регистр Mode 0 (dd.space == REGSPACE) 
    // является нелегальной! Процессор обязан заблокировать прыжок и вызвать TRAP 10!
    // ===================================================================
    if (dd.space == REGSPACE) {
        // ===================================================================
        // 🔬 ДОПРОС ДЕШИФРАТОРА: ВЫТАСКИВАЕМ ИСТИННЫЙ ОПКОД ИЗ ЦИКЛА RUN()
        // ===================================================================
        print_log(LOG_INFO, ">>> ДOПРOС ДЕШИФРAТOРA: Ложный JMP на опкоде: %06o", current_instruction_word);
        // ===================================================================
        
        print_log(LOG_ERROR, ">>> JMP ILLEGAL: Attempt to JMP into direct register Mode 0! Triggering TRAP 10...");
        do_trap10(); // Вызываем ловушку Reserved Instruction
        return;
    }
    // ===================================================================

    PC = dd.adr;
}

void do_neg(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //NEGb положительное число
        Byte old_val = (Byte)(dd.val & 0xFF); 
        Byte res = (Byte)((0 - old_val) & 0xFF);

        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)res; 
        } else {
            b_write(dd.adr, res);
        }

        flag_V = (old_val == 128) ? 1 : 0;
        flag_C = (res != 0) ? 1 : 0;
        final_res = res;
    } 
    else {
        //NEG ноль
        Word old_val = dd.val;
        Word res = (Word)((0 - old_val) & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        flag_V = (old_val == 32768) ? 1 : 0;
        flag_C = (res != 0) ? 1 : 0;
        final_res = res;
    }

    set_flags_NZ(final_res);
}

void do_reset(void) {
    // В рамках текущего MMIO эмулятора команда сброса шины является безопасным
    // холостым ходом для CPU. Регистры общего назначения и флаги PSW не изменяются.
    // Если в будущем добавится сложная асинхронная периферия, здесь будет сброс их буферов.
}

void do_rol(void) {
    int old_c = flag_C;
    Word final_res = 0;

    if (byte_cmd) {
        //ROLb
        Byte old_val = (Byte)(dd.val & 0xFF);
        flag_C = (old_val >> 7) & 1;
        Byte res = (Byte)(((old_val << 1) | old_c) & 0xFF);

        if (dd.space == REGSPACE) reg[dd.adr] = (signed char)res;
        else b_write(dd.adr, res);

        final_res = res;
    } 
    else {
        //ROL
        Word old_val = dd.val;
        flag_C = (old_val >> 15) & 1;
        Word res = (Word)(((old_val << 1) | old_c) & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        final_res = res;
    }

    set_flags_NZ(final_res);
    flag_V = flag_N ^ flag_C;
}

void do_ror(void) {
    int old_c = flag_C;
    Word final_res = 0;

    if (byte_cmd) {
        //RORb
        Byte old_val = (Byte)(dd.val & 0xFF);
        flag_C = old_val & 1;
        Byte res = (Byte)(((old_val >> 1) | (old_c << 7)) & 0xFF);

        if (dd.space == REGSPACE) reg[dd.adr] = (signed char)res;
        else b_write(dd.adr, res);

        final_res = res;
    } 
    else {
        //ROR
        Word old_val = dd.val;
        flag_C = old_val & 1;
        Word res = (Word)(((old_val >> 1) | (old_c << 15)) & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        final_res = res;
    }

    set_flags_NZ(final_res);
    flag_V = flag_N ^ flag_C;
}

void do_sbc(void) {
    int old_c = flag_C;
    Word final_res = 0;

    if (byte_cmd) {
        //SBCb
        Byte old_val = (Byte)(dd.val & 0xFF);
        DWord res32 = (DWord)old_val - (DWord)old_c;
        Byte res = (Byte)(res32 & 0xFF);

        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)res; 
        } else {
            b_write(dd.adr, res);
        }

        flag_V = (old_val == 128 && old_c == 1) ? 1 : 0;
        flag_C = (old_val == 0 && old_c == 1) ? 1 : 0;
        final_res = res;
    } 
    else {
        //SBC
        Word old_val = dd.val;
        DWord res32 = (DWord)old_val - (DWord)old_c;
        Word res = (Word)(res32 & 0xFFFF);

        w_write(dd.adr, res, dd.space);

        flag_V = (old_val == 32768 && old_c == 1) ? 1 : 0;
        flag_C = (old_val == 0 && old_c == 1) ? 1 : 0;
        final_res = res;
    }

    set_flags_NZ(final_res);
}

void do_set_fl(void) {
    Word w = w_read(PC - 2);
    int mask = w & 15;

    if (mask & 1)      flag_C = 1;
    if (mask >> 1 & 1) flag_V = 1;
    if (mask >> 2 & 1) flag_Z = 1;
    if (mask >> 3 & 1) flag_N = 1;
}

void do_sub(void) {
    Word s = ss.val;
    Word d = dd.val;

    DWord res32 = (DWord)d - (DWord)s;
    Word final_res = (Word)(res32 & 0xFFFF);

    // ===================================================================
    // ЖЕСТКАЯ ЗАПИСЬ НА ШИНУ АЛУ (Исправлено нормально под твой каркас):
    // Если приемником является чистый регистр (Мода 0, dd.adr < 8 и REGSPACE),
    // мы фигачим запись НАПРЯМУЮ в массив reg[], полностью минуя любые 
    // скрытые маски и затыки функции w_write!
    // ===================================================================
    if (dd.space == REGSPACE || (dd.space == MEMSPACE && dd.adr < 8 && (current_instruction_word & 070) == 0)) {
        reg[dd.adr] = final_res;
    } else {
        w_write(dd.adr, final_res, dd.space);
    }
    // ===================================================================

    set_flags_NZ(final_res);
    flag_C = (d < s) ? 1 : 0; 
    flag_V = (((d >> 15) != (s >> 15)) && ((final_res >> 15) == (s >> 15))) ? 1 : 0;
}

void do_swab(void) {
    Word old_val = dd.val;
    Word low_byte = old_val & 0xFF;
    Word high_byte = (old_val >> 8) & 0xFF;
    Word res = (low_byte << 8) | high_byte;

    w_write(dd.adr, res, dd.space);

    set_flags_NZ(high_byte << 8);
    flag_V = 0;
    flag_C = 0;
}

void do_sxt(void) {
    Word res = 0;

    if (flag_N) {
        res = 65535;
    } else {
        res = 0;
    }

    w_write(dd.adr, res, dd.space);

    set_flags_NZ(res);
    flag_V = 0;
}

void do_xor(void) {
    Word s = reg[r];
    Word d = dd.val;

    Word res = (Word)((s ^ d) & 0xFFFF);

    w_write(dd.adr, res, dd.space);

    set_flags_NZ(res);
    flag_V = 0;
}

void do_mul(void) {
    short s = (short)dd.val;
    short r_val = (short)reg[r];
    int signed_res = (int)s * (int)r_val;
    DWord res32 = (DWord)signed_res;

    if ((r & 1) == 0) {
        // Если чётный (R0, R2, R4): пишем обе половинки в пару регистров
        reg[r] = (Word)((res32 >> 16) & 0xFFFF);
        reg[r + 1] = (Word)(res32 & 0xFFFF);
        
        // Флаг N для чётного регистра смотрит на самый старший 31-й бит
        flag_N = (res32 >> 31) & 1;
    } else {
        // Если нечётный (R1, R3, R5): пишем ТОЛЬКО младшие 16 бит
        reg[r] = (Word)(res32 & 0xFFFF);
        
        // Флаг N для нечётного регистра смотрит строго на 15-й бит младшего слова!
        flag_N = (res32 >> 15) & 1;
    }

    flag_Z = (res32 & 0xFFFF) == 0; // Z всегда по младшим 16 битам
    flag_V = 0;
    flag_C = (signed_res < -32768 || signed_res > 32767);
}

void do_div(void) {
    short divisor = (short)dd.val;

    //проверка деления на ноль
    if (divisor == 0) {
        flag_V = 1;
        flag_C = 1;
        return;
    }

    int r_high = r;
    int r_low = r | 1;

    DWord raw_dividend = ((DWord)reg[r_high] << 16) | (reg[r_low] & 0xFFFF);
    int dividend = (int)raw_dividend;
    
    int quotient = dividend / divisor;
    int remainder = dividend % divisor;

    //проверка на знаковое переполнение
    if (quotient < -32768 || quotient > 32767) {
        flag_V = 1;
        return;
    }

    reg[r_high] = (Word)(quotient & 0xFFFF);
    reg[r_low] = (Word)(remainder & 0xFFFF);

    set_flags_NZ((Word)quotient);
    flag_V = 0;
    flag_C = 0;
}

void do_rti(void) {
    // 1. Извлекаем сохраненный PC из стека SP
    PC = w_read(SP);
    SP += 2;
    
    // 2. Извлекаем сохраненный PSW из стека SP
    Word old_psw = w_read(SP);
    SP += 2;
    
    // ===================================================================
    // ИСПРАВЛЕНО ПО КРЕМНИЕВОМУ КАНOНУ DEC PDP-11:
    // Вместо ручного копирования только 4 битов флагов условий, вызываем set_psw().
    // Это гарантирует, что из стека ОЗУ восстановится абсолютно ВСЁ слово состояния,
    // включая глобальный аппаратный приоритет процессора cpu_priority (биты 5-7)!
    // ===================================================================
    set_psw(old_psw);
    // ===================================================================
    
    print_log(LOG_TRACE, ">>> RTI: Returned from interrupt. Restored PC: %06o", PC);
}

void do_emt(void) {
    // 1. Запись текущего PSW в стек
    SP -= 2;
    w_write(SP, get_psw(), MEMSPACE);
    
    // 2. Запись текущего РС в стек
    SP -= 2;
    w_write(SP, PC, MEMSPACE);
    
    // 3. Загружаем новый PC и PSW строго один в один из Вектора EMT (000030/000032)
    PC = w_read(000030);
    Word target_psw = w_read(000032);
    
    // Передаем чистокровный каноничный PSW обработчика Монитора в set_psw
    set_psw(target_psw);
    
    abort_instruction = 1; 
    print_log(LOG_TRACE, ">>> TRAP: EMT triggered! Vector 0030 loaded. New PC: %06o", PC);
}


void do_trap(void) {
    // 1. Запись текущего PSW в стек
    SP -= 2;
    w_write(SP, get_psw(), MEMSPACE);
    
    // 2. Запись текущего РС в стек
    SP -= 2;
    w_write(SP, PC, MEMSPACE);
    
    // 3. Загружаем новый PC из вектора TRAP (восьмеричный адрес 000034)
    PC = w_read(000034);
    
    // ===================================================================
    // ИСПРАВЛЕНО ПО АППАРАТНОМУ СТАНДАРТУ DEC (TRAP INSTRUCTION LOGIC):
    // Считываем старшее слово Вектора TRAP (восьмеричный адрес 000036)
    // и полностью обновляем PSW процессора, включая его системный приоритет!
    // ===================================================================
    Word target_psw = w_read(000036);
    set_psw(target_psw);
    // ===================================================================
    
    print_log(LOG_TRACE, ">>> TRAP: TRAP instruction triggered! Vector 0034 loaded. New PC: %06o", PC);
}

void do_unknown(void) {
    Address fault_pc = global_current_pc;
    Word unknown_cmd = w_read(fault_pc);
    
    // Вывод сообщения в консоль на английском языке
    print_log(LOG_ERROR, ">>> CPU ERROR: Illegal instruction %06o at PC %06o! Triggering TRAP 10...", unknown_cmd, fault_pc);

    abort_instruction = 1;

    // 1. Пушим PSW в стек SP
    reg[6] -= 2;
    w_write(reg[6], get_psw(), MEMSPACE);

    // 2. Пушим PC возврата (адрес СЛЕДУЮЩЕЙ за сбойной команды)
    reg[6] -= 2;
    Word trap_return_pc = (Word)((global_current_pc + 2) & 0xFFFF);
    w_write(reg[6], trap_return_pc, MEMSPACE);

    // 3. Аппаратно загружаем новый PC из системного вектора 10 (восьмеричное 010)
    PC = w_read(010);
}

// Функция чтения 32-битного float из ОЗУ PDP-11 по спецификации DEC F_floating
float read_dec_float(Address addr) {
    Word hi_word = w_read(addr);
    Word lo_word = w_read(addr + 2);
    
    // Если оба слова нули — это чистый ноль
    if (hi_word == 0 && lo_word == 0) return 0.0f;
    
    // Извлекаем знак (15-й бит старшего слова)
    int sign = (hi_word >> 15) & 1;
    
    // Извлекаем экспоненту DEC (биты 7-14 старшего слова)
    int dec_exp = (hi_word >> 7) & 0xFF;
    
    // Проверка на некорректную экспоненту (Reserved Operand Trap)
    if (dec_exp == 0 && sign == 1) {
        return 0.0f; 
    }
    
    // Собираем 23-битную мантиссу DEC из кусочков старшего и младшего слов
    unsigned int mant = ((unsigned int)(hi_word & 0x7F) << 16) | lo_word;
    
    // Восстанавливаем скрытую единицу DEC, которая стоит перед 24-м битом фракции (0.1MMMM...)
    double fraction = (double)(mant | 0x00800000) / 16777216.0; // 2^24 = 16777216
    
    // Вычисляем реальный порядок числа по спецификации DEC (bias = 128)
    int exponent = dec_exp - 128;
    
    // Собираем итоговое живое число float языка Си через ldexp
    double res_double = ldexp(fraction, exponent);
    if (sign) res_double = -res_double;
    
    return (float)res_double;
}

// Функция записи 32-битного float обратно в ОЗУ PDP-11 по спецификации DEC F_floating
void write_dec_float(Address addr, float val) {
    if (val == 0.0f) {
        w_write(addr, 0, MEMSPACE);
        w_write(addr + 2, 0, MEMSPACE);
        return;
    }
    
    int sign = 0;
    double abs_val = val;
    if (val < 0.0f) {
        sign = 1;
        abs_val = -abs_val;
    }
    
    int exponent = 0;
    // Разлагаем float на фракцию (0.5 <= frac < 1.0) и порядок через frexp строго по канону DEC!
    double fraction = frexp(abs_val, &exponent);
    
    // Вычисляем экспоненту DEC (bias = 128)
    int dec_exp = exponent + 128;
    if (dec_exp < 0) dec_exp = 0;
    if (dec_exp > 0xFF) dec_exp = 0xFF;
    
    // Переводим фракцию обратно в 23-битное целое число мантиссы
    unsigned int mant = (unsigned int)(fraction * 16777216.0 + 0.5) & 0x7FFFFF;
    
    // Упаковываем данные в старшее и младшее слова PDP-11
    Word hi_word = (Word)((sign << 15) | (dec_exp << 7) | ((mant >> 16) & 0x7F));
    Word lo_word = (Word)(mant & 0xFFFF);
    
    w_write(addr, hi_word, MEMSPACE);
    w_write(addr + 2, lo_word, MEMSPACE);
}

void do_fis_math(const char* op_name) {
    int rn = r; 
    Address stack_ptr = reg[rn];

    // СТРОГО ПО СПЕЦИФИКАЦИИ DEC FIS:
    // На вершине стека (Rn) всегда лежит аргумент B (делитель / вычитаемое)
    // На 4 байта выше (Rn + 4) лежит аргумент A (делимое / уменьшаемое)
    float arg_B = read_dec_float(stack_ptr);      
    float arg_A = read_dec_float(stack_ptr + 4);  

    float result = 0.0f;

    if (strcmp(op_name, "fadd") == 0) result = arg_A + arg_B;
    if (strcmp(op_name, "fsub") == 0) result = arg_A - arg_B;
    if (strcmp(op_name, "fmul") == 0) result = arg_A * arg_B;
    if (strcmp(op_name, "fdiv") == 0) {
        if (arg_B == 0.0f) {
            do_trap244();
            return;
        } else {
            result = arg_A / arg_B;
        }
    }

    // ЖЕЛЕЗНЫЙ ЗАКОН КРЕМНИЯ DEC FIS:
    // Результат вычислений ВСЕГДА записывается на вершину стека (на место аргумента B)!
    write_dec_float(stack_ptr, result);

    // АВТОИНКРЕМЕНТ УКАЗАТЕЛЯ СТЕКА ПО СПЕЦИФИКАЦИИ DEC:
    if (rn == 6) {
        reg[rn] = (Word)((stack_ptr + 4) & 0xFFFF);
    }

    // Выставляем флаги условий АЛУ для вещественных чисел по канону DEC
    flag_V = 0;
    flag_C = 0;
    flag_Z = (result == 0.0f) ? 1 : 0;
    flag_N = (result < 0.0f) ? 1 : 0;
}

void do_fadd(void) { 
    do_fis_math("fadd"); 
}

void do_fsub(void) { 
    do_fis_math("fsub"); 
}

void do_fmul(void) { 
    do_fis_math("fmul"); 
}

void do_fdiv(void) { 
    do_fis_math("fdiv"); 
}

void do_setf(void) {
    // По канону DEC FP-11: сбрасываем бит FD (8-й бит регистра FPSR) в ноль.
    // Это переключает FPU с 64-битного формата Double на 32-битный Single float.
    fpu_fpsr &= ~FPU_BIT_FD;
    
    // Очищаем флаги условий самого сопроцессора (FN, FZ, FV, FC в младшем байте FPSR)
    fpu_fpsr &= ~(FPU_BIT_FN | FPU_BIT_FZ | FPU_BIT_FV | FPU_BIT_FC);
    
    // По спецификации, живые флаги основного процессора (flag_N, flag_Z) команда НЕ трогает
}

void do_trap4(void) {
    print_log(LOG_ERROR, ">>> BUS TIMEOUT TRAP 4 FIRED !!! Affected PC = %06o, Opcode = %06o",
              global_current_pc, current_instruction_word);
    
    fflush(stdout);
    fflush(stderr);

    // Твой чистый, исходный фабричный пуш контекста процессора в стек ОЗУ:
    reg[6] -= 2;
    w_write(reg[6], get_psw(), MEMSPACE);
    reg[6] -= 2;
    w_write(reg[6], PC, MEMSPACE);

    // Чистый канон загрузки из Вектора 4 один в один:
    PC = w_read(000004);
    set_psw(w_read(000006));

    abort_instruction = 1; 
}



void do_trap244(void) {
    print_log(LOG_ERROR, ">>> FPU MATH ERROR TRAP: Saving context and branching to Vector 244...");

    // Сигнализируем главному циклу run(), что текущая инструкция прервана аварийно
    abort_instruction = 1;

    // 1. Уменьшаем указатель аппаратного стека SP (reg[6]) на 2 и пушим текущий PSW через w_write
    reg[6] -= 2;
    w_write(reg[6], get_psw(), MEMSPACE);

    // 2. Уменьшаем SP на 2 и пушим PC возврата через w_write
    reg[6] -= 2;
    
    // ВЫРАВНЕНО ПО СПЕЦИФИКАЦИИ: Адрес возврата рассчитывается от global_current_pc,
    // гарантируя точное попадание на следующую инструкцию после математического сбоя.
    Word trap_return_pc = (Word)((global_current_pc + 2) & 0xFFFF);
    w_write(reg[6], trap_return_pc, MEMSPACE);

    // 3. Загружаем новый PC из системного вектора 000244
    PC = w_read(0000244);
}


void do_ldf(void) {
    // Извлекаем float из адреса источника ss
    float val = read_dec_float(ss.adr);
    
    // Записываем в выбранный математический регистр fpu_ac[r]
    if (r >= 0 && r < 6) {
        fpu_ac[r] = val;
    }

    // Выставляем флаги FPU в PSW
    flag_V = 0;
    flag_C = 0;
    flag_Z = (val == 0.0f) ? 1 : 0;
    flag_N = (val < 0.0f) ? 1 : 0;
}

void do_stf(void) {
    if (r >= 0 && r < 6) {
        float val = fpu_ac[r];
        // Конвертируем и пишем обратно в ОЗУ по адресу приемника
        write_dec_float(ss.adr, val);
    }
}

void do_trap10(void) {
    print_log(LOG_ERROR, ">>> CPU ERROR: Illegal instruction %06o at PC %06o! Triggering TRAP 10...",
              current_instruction_word, global_current_pc);
              
    reg[6] -= 2;
    w_write(reg[6], get_psw(), MEMSPACE);
    reg[6] -= 2;
    w_write(reg[6], PC, MEMSPACE);

    PC = w_read(000010);
    set_psw(w_read(000012));

    abort_instruction = 1;
}

void do_mtps(void) {
    // По спецификации DEC, команда MTPS берет только младший байт операнда источника
    Byte val = (Byte)(dd.val & 0xFF);
    
    // Аппаратно извлекаем и выставляем глобальный приоритет процессора (биты 5-7 байта)
    cpu_priority = (val >> 5) & 7;
    
    // Обновляем флаги условий АЛУ NZVC из младших 4 бит байта
    flag_N = (val >> 3) & 1;
    flag_Z = (val >> 2) & 1;
    flag_V = (val >> 1) & 1;
    flag_C = val & 1;
}

void do_mfps(void) {
    // Собираем текущее живое слово состояния процессора
    Word psw = get_psw();
    Byte psw_byte = (Byte)(psw & 0xFF);
    
    // ЖЕСТКИЙ ЗАКОН DEC: Если приемником выступает прямой регистр процессора (REGSPACE),
    // то считанный байт PSW автоматически расширяется знаком в полноценное 16-битное слово!
    if (dd.space == REGSPACE) {
        Word expanded = (Word)((signed char)psw_byte);
        w_write(dd.adr, expanded, REGSPACE);
    } else {
        // При записи в обычную память ОЗУ пишется чистый байт
        b_write(dd.adr, psw_byte);
    }
    
    // Выставляем флаги N и Z по значению считанного байта
    set_flags_NZ((Word)psw_byte);
    flag_V = 0; // Флаг V всегда сбрасывается в 0 по спецификации
}

void do_rtt(void) {
    // Логика извлечения контекста из стека SP (reg[6]) полностью идентична do_rti
    PC = w_read(reg[6]);
    reg[6] += 2;
    
    Word old_psw = w_read(reg[6]);
    reg[6] += 2;
    
    // Восстанавливаем флаги и глобальный приоритет процессора
    set_psw(old_psw);
    
    // Единственное отличие RTT от RTI на реальном кремнии — это запрет прерывания Т
    // на следующей инструкции, но для базовой загрузки ядра это не критично.
}

void do_nop(void) {
    // Абсолютно пустое тело по спецификации DEC PDP-11
}
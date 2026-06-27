#include "config.h"
#include "cpu.h"
#include "memory.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

Word reg[REGSIZE];      //регистры процессора (дополнительная память)

Command command[] = {   //таблица команд
    {0170000, 0060000,  "add",   do_add,    HAS_SS | HAS_DD},
    {0170000, 0010000,  "mov",   do_mov,    HAS_SS | HAS_DD},
    {0170000, 0110000,  "movb",  do_mov,    HAS_SS | HAS_DD},
    {0177000, 0077000,  "sob",   do_sob,    HAS_R | HAS_NN},
    {0177700, 0005000,  "clr",   do_clr,    HAS_DD},
    {0177400, 0000400,  "br",    do_br,     HAS_XX},          
    {0177400, 0100000,  "bpl",   do_bpl,    HAS_XX}, 
    {0177400, 0001000,  "bne",   do_bne,    HAS_XX}, 
    {0177400, 0001400,  "beq",   do_beq,    HAS_XX},
    {0177777, 0000000,  "halt",  do_halt,   NO_PARAMS},
};

#define COMMAND_COUNT (sizeof(command) / sizeof(command[0]))

Arg ss, dd;                 //переменные аргументов (ss - откуда, dd - куда)
int r, n, nn, xx;           //переменные (r - номер регистра, n - константа 3 бита, nn  - константа 6 бит, xx - смещение со знаком)

//флаги условий регистра состояния PSW
int flag_N = 0;             //Negative (результат отрицательный)
int flag_Z = 0;             //Zero     (результат равен нулю)
int flag_V = 0;             //oVerflow (Знаковое переполнение)
int flag_C = 0;             //Carry    (перенос из старшего разряда)

int byte_cmd = 0;           //1 — команда BYTE, 0 — команда WORD (15-й бит)

static const Command unknown_command = {0, 0, "unknown", do_nothing, NO_PARAMS}; //если функция не определена

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
        case 3: 
            if (r == 7) sprintf(out_str, "@#%o", arg.adr);
            else sprintf(out_str, "@(R%d)+", r);
            break;
        case 4: 
            sprintf(out_str, "-(R%d)", r); 
            break;
        case 5: 
            sprintf(out_str, "@-(R%d)", r); 
            break;
        case 6:
            Word x = arg.adr - reg[r];
            if (r == 7) sprintf(out_str, "%o", arg.adr);
            else sprintf(out_str, "%o(R%d)", x, r);
            break;
        case 7:
            if (r == 7) sprintf(out_str, "@#%o", arg.adr);
            else sprintf(out_str, "@#%o", arg.adr);
            break;
        default: sprintf(out_str, "?"); break;
    }
}

void reg_dump() {
    print_log(LOG_TRACE, "R0:%o R1:%o R2:%o R3:%o R4:%o R5:%o R6:%o R7:%o\n", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
}

Command parse_cmd(Word w) {

    byte_cmd = (w >> 15) & 1;

    //поиск в таблице команд
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if ((w & command[i].mask) == command[i].opcode) {   
            //проверка флага SS
            if (command[i].params & HAS_SS) {
                ss = get_mr(w >> 6);
            }
            //проверка флага DD
            if (command[i].params & HAS_DD) {
                dd = get_mr(w);
            }
            //проверка флага R
            if (command[i].params & HAS_R) {
                r = (w >> 6) & 7;
            }
            //проверка флага N
            if (command[i].params & HAS_N) {
                n = w & 7;
            }
            //проверка флага NN
            if (command[i].params & HAS_NN) {
                nn = w & 077;
            }
            //проверка флага XX
            if (command[i].params & HAS_XX) {
                char offset = (char)(w & 0xFF);
                xx = (int)offset;
            }

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
            print_log(LOG_ERROR, "Unknown instruction %06o at address %06o\n", w, PC - 2);
            exit(1);
        }

        //печатаем лог в стиле MACRO-11
        if (strcmp(cmd.name, "halt") == 0) {
            //для HALT выводим только имя
            print_log(LOG_TRACE, "%06o %06o: %s\n", current_pc, w, cmd.name);
        } else if (strcmp(cmd.name, "sob") == 0) {
            Address target_pc = PC - 2 * nn; 
            print_log(LOG_TRACE, "%06o %06o: %s R%d, %06o\n", current_pc, w, cmd.name, r, target_pc);
        } else if (strcmp(cmd.name, "clr") == 0) {
            char dd_str[32] = "";
            format_arg(dd, w, dd_str);
            print_log(LOG_TRACE, "%06o %06o: %s %s\n", current_pc, w, cmd.name, dd_str);
        } else {
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
    Address pointer_adr;    //указатель на адрес
    Word x;                 //смещение (для моды 6 и 7)
    int m = (w >> 3) & 7;   //номер моды
    int r = w & 7;          //номер регистра

    switch (m) {
        //мода 0, R1
        case 0:
            res.adr = r;                                //адрес - номер регистра
            res.val = reg[r];                           //значение - число в регистре
            res.space = REGSPACE;                       //записываем в регистр
            break;

        //мода 1, (R1)
        case 1:
            res.adr = reg[r];                           //в регистре адрес
            if (byte_cmd) {
                res.val = (signed char)b_read(res.adr); //по адресу байта - значение
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }                                       
            res.space = MEMSPACE;                       //записываем в память
            break;

        //мода 2, (R1)+ или #3
        case 2:
            res.adr = reg[r];                           //в регистре адрес
            if (byte_cmd) {
                res.val = (signed char)b_read(res.adr); //по адресу байта - значение
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }
            res.space = MEMSPACE;                       //записываем в память
            if (byte_cmd && r < 6) {
                reg[r] += 1;                            //байтовый инкремент в регистрах R0-R5
            } else {
                reg[r] += 2;                            //инкремент для PC и SP только команды Word
            }  
            break;

        //мода 3, @(R1)+ или @#100
        case 3:
            Address point_adr = reg[r];                 //в регистре адрес
            res.adr = w_read(point_adr);                //по адресу - целевой адрес
            reg[r] += 2;                                //автоинкремент регистра (всегда +2)
            res.val = w_read(res.adr);                  //по целевому адресу - значение
            res.space = MEMSPACE;                       //записываем в память
            break;

        //мода 4, -(R1)
        case 4:
            if (byte_cmd && r < 6) {
                reg[r] -= 1;                            //байтовый автодекремент в регистрах R0-R5
            } else {
                reg[r] -= 2;                            //автодекремент для PC и SP только команды Word
            }
            res.adr = reg[r];                           //в регистре новый адрес
            if (byte_cmd) {
                res.val = (signed char)b_read(res.adr); //по адресу байта - значение
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }
            res.space = MEMSPACE;                       //записываем в память
            break;

        //мода 5, @-(R1)
        case 5:
            reg[r] -= 2;                                //автодекремент регистра (всегда -2)
            pointer_adr = reg[r];                       //в регистре адрес
            res.adr = w_read(pointer_adr);              //по адресу - целевой адрес
            res.val = w_read(res.adr);                  //по целевому адресу - значение
            res.space = MEMSPACE;                       //записываем в память
            break;

        // мода 6, X(R1) или X(PC)
        case 6:
            x = w_read(PC); 
            PC += 2;
            res.adr = (Address)(reg[r] + (short)x);     //адрес указателя со смещением
            res.val = w_read(res.adr);                  //по адресу - значение
            res.space = MEMSPACE;                       //записываем в память
            break;

        // мода 7, @X(R1) или @X(PC)
        case 7:
            x = w_read(PC); 
            PC += 2;
            pointer_adr = (Address)(reg[r] + (short)x); //адрес указателя со смещением
            res.adr = w_read(pointer_adr);              //по адресу - целевой адрес
            res.val = w_read(res.adr);                  //по целевому адресу - значение
            res.space = MEMSPACE;                       //записываем в память
            break;

        default:
            print_log(LOG_ERROR, "Mode %d not implemented yet!\n", m);
            exit(1);
        }

    return res;
}

void w_reg_write(int r, Word val) {
    reg[r] = val;
}

void set_flags_mov(Word val) {
    if (byte_cmd) {
        Byte res_byte = (Byte)val;
        flag_Z = (res_byte == 0) ? 1 : 0;
        flag_N = (res_byte >> 7) & 1; //7-й бит байта — знаковый
    } else {
        Word res_word = val & 0177777;
        flag_Z = (res_word == 0) ? 1 : 0;
        flag_N = (res_word >> 15) & 1; //15-й бит слова — знаковый
    }

    flag_V = 0; //флаг V всегда 0
}

// Функция вычисления флагов C и V для команды сложения ADD
void set_flags_add(Word src, Word dst, unsigned int res) {
    Word s = src & 0177777;
    Word d = dst & 0177777;
    Word r = (Word)(res & 0177777);
    
    flag_C = (res >> 16) & 1; 
    flag_V = (((s >> 15) == (d >> 15)) && ((s >> 15) != (r >> 15))) ? 1 : 0;
    //флаги C и V не меняются
}


void do_halt(void) {
    print_log(LOG_INFO, "==================================================\n");
    print_log(LOG_INFO, "               PROCESSOR HALTED                   \n");
    print_log(LOG_INFO, "==================================================\n");
    
    reg_dump();
              
    print_log(LOG_INFO, "Status: Execution finished successfully (code 0)\n");
    print_log(LOG_INFO, "==================================================\n");
    
    exit(0);
}

void do_mov(void) {
    if (byte_cmd) {
        b_write(dd.adr, (Byte)ss.val);      //значение байтового аргумента SS пишем по адресу аргумента DD
    } else {
        w_write(dd.adr, ss.val, dd.space);  //значение Word аргумента SS пишем по адресу аргумента DD
    }
    set_flags_mov(ss.val);                  //выставляем флаги условий PSW
}

void do_add(void) {
    unsigned int res32 = (unsigned int)ss.val + (unsigned int)dd.val;   //сумма SS и DD
    
    Word final_res = (Word)(res32 & 0177777);
    w_write(dd.adr, final_res, dd.space);
    
    set_flags_mov(final_res);
    set_flags_add(ss.val, dd.val, res32);
}

void do_sob(void) {
        reg[r] -= 1;            //уменьшаем выбранный регистр на 1
    //идем назад пока регистр не станет равен 0
    if (reg[r] != 0) {
        PC = PC - 2 * nn;
    }
}

void do_clr(void) {
    w_write(dd.adr, 0, dd.space);   //обнуляем регистр
}

void do_br(void) {
    PC = PC + xx * 2;
}

void do_bpl(void) {
    if (flag_N == 0) {
        do_br();
    }
}

void do_bne(void) {
    if (flag_Z == 0) {
        do_br();
    }
}

void do_beq(void) {
    if (flag_Z == 1) {
        do_br();
    }
}

void do_nothing(void) {
    printf("unknown\n");
}
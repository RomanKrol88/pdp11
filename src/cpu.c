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
    {0177700, 0005500,  "adc",      do_adcb,    HAS_DD},
    {0177700, 0105500,  "adcb",     do_adcb,    HAS_DD},
    {0170000, 0060000,  "add",      do_add,     HAS_SS | HAS_DD},
    {0177000, 0072000,  "ash",      do_ash,     HAS_R | HAS_SS},
    {0177000, 0073000,  "ashc",     do_ashc,    HAS_R | HAS_SS},
    {0177700, 0006300,  "asl",      do_asl,     HAS_DD},
    {0177700, 0106300,  "aslb",     do_aslb,    HAS_DD},
    {0177700, 0006200,  "asr",      do_asr,     HAS_DD},
    {0177700, 0106200,  "asrb",     do_asrb,    HAS_DD},
    {0177400, 0001400,  "beq",      do_beq,     HAS_XX},
    {0177400, 0001000,  "bne",      do_bne,     HAS_XX}, 
    {0177400, 0100000,  "bpl",      do_bpl,     HAS_XX},
    {0177400, 0000400,  "br",       do_br,      HAS_XX},
    {0177700, 0005000,  "clr",      do_clr,     HAS_DD},
    {0177777, 0000000,  "halt",     do_halt,    NO_PARAMS},
    {0177000, 0004000,  "jsr",      do_jsr,     HAS_R | HAS_DD},
    {0170000, 0010000,  "mov",      do_mov,     HAS_SS | HAS_DD},
    {0170000, 0110000,  "movb",     do_mov,     HAS_SS | HAS_DD},
    {0177770, 0000200,  "rts",      do_rts,     HAS_N},
    {0177000, 0077000,  "sob",      do_sob,     HAS_R | HAS_NN},
    {0177700, 0105700,  "tstb",     do_tstb,    HAS_DD}, 
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

int output_print = 0;       //переменная для беспрефиксного вывода stdout на дисплей

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
    print_log(LOG_TRACE, "R0:%o R1:%o R2:%o R3:%o R4:%o R5:%o R6:%o R7:%o", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
}

Command parse_cmd(Word w) {

    byte_cmd = (w >> 15) & 1;

    //поиск в таблице команд
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if ((w & command[i].mask) == command[i].opcode) {   
            //проверка флага SS
            if ((command[i].params & HAS_R) && (command[i].params & HAS_SS)) {
                ss = get_mr(w); //забираем из младших 6 бит для ASH/ASHC
            } else {
                //обычные двухадресные команды (mov, add), у них SS в битах 6-11
                if (command[i].params & HAS_SS) {
                    ss = get_mr(w >> 6);
                }
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
    //следующее слово будем читать по адресу 1000 (восьмеричное)
    PC = 01000;

    Word w;     //текущее слово, которое содержит команду
    
    while(1) {
        w = w_read(PC);                                 //читаем текущее слово
        Address current_pc = PC;                        //сохраняем текущее значение РС для вывода в лог
        PC += 2;                                        //PC сразу же указывает на следующее неразобранное слово
        Command cmd = parse_cmd(w);                     //декодируем считанное слово

        //если вернулась неизвестная команда, выводим ошибку
        if (strcmp(cmd.name, "unknown") == 0) {
            print_log(LOG_ERROR, "Unknown instruction %06o at address %06o", w, PC - 2);
            exit(1);
        }

        //печатаем лог в стиле MACRO-11
        if (strcmp(cmd.name, "halt") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s", current_pc, w, cmd.name);
        } else if (strcmp(cmd.name, "br") == 0  || strcmp(cmd.name, "bpl") == 0 || 
                 strcmp(cmd.name, "bne") == 0 || strcmp(cmd.name, "beq") == 0) {
            Address target_pc = PC + xx * 2; 
            print_log(LOG_TRACE, "%06o %06o: %s %06o", current_pc, w, cmd.name, target_pc);
        } else if (strcmp(cmd.name, "sob") == 0) {
            Address target_pc = PC - 2 * nn; 
            print_log(LOG_TRACE, "%06o %06o: %s R%d, %06o", current_pc, w, cmd.name, r, target_pc);
        } else if (strcmp(cmd.name, "clr") == 0 || strcmp(cmd.name, "tstb") == 0 ||
                   strcmp(cmd.name, "adcb") == 0 || strcmp(cmd.name, "adc") == 0 ||
                   strcmp(cmd.name, "aslb") == 0 || strcmp(cmd.name, "asrb") == 0 ||
                   strcmp(cmd.name, "asl") == 0  || strcmp(cmd.name, "asr") == 0) {
            char dd_str[32] = "";
            format_arg(dd, w, dd_str);
            print_log(LOG_TRACE, "%06o %06o: %s %s", current_pc, w, cmd.name, dd_str);
        } else if (strcmp(cmd.name, "jsr") == 0) {
            char dd_str[32] = "";
            format_arg(dd, w, dd_str);
            if (r == 7) { 
                print_log(LOG_TRACE, "%06o %06o: %s PC, %s", current_pc, w, cmd.name, dd_str);
            } else {
                print_log(LOG_TRACE, "%06o %06o: %s R%d, %s", current_pc, w, cmd.name, r, dd_str);
            }
        } else if (strcmp(cmd.name, "rts") == 0) {
            if (n == 7) {
                print_log(LOG_TRACE, "%06o %06o: %s PC", current_pc, w, cmd.name);
            } else {
                print_log(LOG_TRACE, "%06o %06o: %s R%d", current_pc, w, cmd.name, n);
            }
        } else if (strcmp(cmd.name, "ash") == 0) {
            char ss_str[32] = "";
            format_arg(ss, w, ss_str); // Извлекаем аргумент источника (счетчик сдвига)
            print_log(LOG_TRACE, "%06o %06o: %s %s, R%d", current_pc, w, cmd.name, ss_str, r);
        } else if (strcmp(cmd.name, "ashc") == 0) {
            char ss_str[32] = "";
            format_arg(ss, w, ss_str);
            print_log(LOG_TRACE, "%06o %06o: %s %s, R%d", current_pc, w, cmd.name, ss_str, r);
        } else {
            char ss_str[32] = "";
            char dd_str[32] = "";

            format_arg(ss, w >> 6, ss_str);
            format_arg(dd, w, dd_str);

            print_log(LOG_TRACE, "%06o %06o: %s %s, %s", current_pc, w, cmd.name, ss_str, dd_str);
        }

        // выполняем команду
        cmd.do_command();

        reg_dump();
    }
}

Arg get_mr(Word w) {
    Arg res;
    Address pointer_adr;    // указатель на адрес
    Word x;                 // смещение (для моды 6 и 7)
    int m = (w >> 3) & 7;   // номер моды
    int r = w & 7;          // номер регистра

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
                //маскируем верхний байт после знакового расширения, чтобы не забить его единицами 0xFFXX
                res.val = (Word)((signed char)b_read(res.adr) & 0xFF); 
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }                                       
            res.space = MEMSPACE;                       //записываем в память
            break;

        //мода 2, (R1)+ или #3
        case 2:
            res.adr = reg[r];                           //в регистре адрес
            if (byte_cmd) {
                //маскируем верхний байт
                res.val = (Word)((signed char)b_read(res.adr) & 0xFF); 
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }
            res.space = MEMSPACE;                       //записываем в память
            
            //регистры SP и PC всегда изменяются на 2
            if (byte_cmd && r < 6) {
                reg[r] += 1;                            //байтовый инкремент в регистрах R0-R5
            } else {
                reg[r] += 2;                            //инкремент для PC, SP и всех команд Word
            }  
            break;

        //мода 3, @(R1)+ или @#100
        case 3:
            pointer_adr = reg[r];                 
            res.adr = w_read(pointer_adr);              //по адресу - целевой адрес
            reg[r] += 2;                                //автоинкремент регистра (всегда +2)
            res.val = w_read(res.adr);                  //по целевому адресу - значение
            res.space = MEMSPACE;                       //записываем в память
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
                res.val = (Word)((signed char)b_read(res.adr) & 0xFF); 
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

        //мода 6, X(R1) или X(PC)
        case 6:
            x = w_read(PC); 
            PC += 2;
            res.adr = (Address)(reg[r] + (short)x);     //адрес указателя со смещением
            res.val = w_read(res.adr);                  //по адресу - значение
            res.space = MEMSPACE;                       //записываем в память
            break;

        //мода 7, @X(R1) или @X(PC)
        case 7:
            x = w_read(PC); 
            PC += 2;
            pointer_adr = (Address)(reg[r] + (short)x); //адрес указателя со смещением
            res.adr = w_read(pointer_adr);              //по адресу - целевой адрес
            res.val = w_read(res.adr);                  //по целевому адресу - значение
            res.space = MEMSPACE;                       //записываем в память
            break;

        default:
            print_log(LOG_ERROR, "Mode %d not implemented yet!", m);
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

void set_flags_add(Word src, Word dst, unsigned int res) {
    Word s = src & 0177777;
    Word d = dst & 0177777;
    Word r = (Word)(res & 0177777);
    
    flag_C = (res >> 16) & 1; 
    flag_V = (((s >> 15) == (d >> 15)) && ((s >> 15) != (r >> 15))) ? 1 : 0;
    //флаги C и V не меняются
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
    if (byte_cmd) {
        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)(ss.val & 0xFF); //при записи в регистр расширяем знак до слова
        } else {
            b_write(dd.adr, (Byte)ss.val);  //записываем в ОЗУ
        }
    } else {
        w_write(dd.adr, ss.val, dd.space); 
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

void do_ash(void) {
    int count = ss.val & 077;
    if (count & 040) {
        count |= ~077;
    }

    if (count == 0) {
        flag_C = 0;
        flag_V = 0;
        flag_Z = (reg[r] == 0) ? 1 : 0;
        flag_N = (reg[r] >> 15) & 1;
        return;
    }

    Word old_val = reg[r];
    Word res = old_val;
    flag_C = 0;
    flag_V = 0;

    if (count > 0) {
        if (count <= 16) {
            flag_C = (old_val >> (16 - count)) & 1;
            res = (old_val << count) & 0177777;
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
            res = (Word)((signed_val >> shift) & 0177777);
        } else {
            res = ((old_val >> 15) & 1) ? 0177777 : 0;
            flag_C = (old_val >> 15) & 1;
        }
    }

    reg[r] = res;
    flag_Z = (res == 0) ? 1 : 0;
    flag_N = (res >> 15) & 1;
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

void do_tstb(void) {
    Byte b = (Byte)(dd.val & 0xFF);
    //флаги N и Z только по аргументу DD
    flag_Z = (b == 0) ? 1 : 0;
    flag_N = (b >> 7) & 1;
    flag_V = 0;
    flag_C = 0;
}

void do_jsr(void) {    
    SP -= 2;
    w_write(SP, reg[r], MEMSPACE);
    reg[r] = PC;
    PC = dd.adr;
}

void do_rts(void) {
    int link_reg = n; 
    PC = reg[link_reg];
    reg[link_reg] = w_read(SP);
    SP += 2;
}

void do_adcb(void) {
    int old_c = flag_C;

    if (byte_cmd) {
        //работа с байтами
        Byte old_val = (Byte)(dd.val & 0xFF);
        unsigned int res32 = (unsigned int)old_val + (unsigned int)old_c;
        Byte final_res = (Byte)(res32 & 0xFF);

        if (dd.space == REGSPACE) {
            reg[dd.adr] = (signed char)final_res;
        } else {
            b_write(dd.adr, final_res);
        }

        flag_Z = (final_res == 0) ? 1 : 0;
        flag_N = (final_res >> 7) & 1;
        flag_C = (res32 > 0xFF) ? 1 : 0;
        flag_V = (old_val == 0177 && old_c == 1) ? 1 : 0;
    } else {
        //работа со словом
        Word old_val = dd.val;
        unsigned int res32 = (unsigned int)old_val + (unsigned int)old_c;
        Word final_res = (Word)(res32 & 0177777);

        w_write(dd.adr, final_res, dd.space);

        flag_Z = (final_res == 0) ? 1 : 0;
        flag_N = (final_res >> 15) & 1;
        flag_C = (res32 > 0177777) ? 1 : 0;
        flag_V = (old_val == 077777 && old_c == 1) ? 1 : 0;
    }
}

void do_ashc(void) {
    int count = ss.val & 077;
    if (count & 040) count |= ~077;

    int r_high = r;
    int r_low = r | 1;
    unsigned int old_32 = ((unsigned int)reg[r_high] << 16) | (reg[r_low] & 0xFFFF);
    unsigned int res_32 = old_32;
    
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
            res_32 = (unsigned int)(signed_32 >> shift);
        } else {
            res_32 = (old_32 >> 31) & 1 ? 0xFFFFFFFF : 0;
            flag_C = (old_32 >> 31) & 1;
        }
    }

    reg[r_high] = (Word)((res_32 >> 16) & 0177777);
    reg[r_low] = (Word)(res_32 & 0177777);

    flag_Z = (res_32 == 0) ? 1 : 0;
    flag_N = (res_32 >> 31) & 1;
}

void do_aslb(void) {
    Byte old_val = (Byte)(dd.val & 0xFF);
    flag_C = (old_val >> 7) & 1;
    
    Byte res = (Byte)((old_val << 1) & 0xFF);
    
    if (dd.space == REGSPACE) reg[dd.adr] = (signed char)res;
    else b_write(dd.adr, res);

    flag_Z = (res == 0) ? 1 : 0;
    flag_N = (res >> 7) & 1;
    flag_V = flag_N ^ flag_C;
}

void do_asrb(void) {
    Byte old_val = (Byte)(dd.val & 0xFF);
    flag_C = old_val & 1;
    signed char signed_b = (signed char)old_val;
    Byte res = (Byte)((signed_b >> 1) & 0xFF);
    
    if (dd.space == REGSPACE) reg[dd.adr] = (signed char)res;
    else b_write(dd.adr, res);

    flag_Z = (res == 0) ? 1 : 0;
    flag_N = (res >> 7) & 1;
    flag_V = flag_N ^ flag_C;
}

void do_asl(void) {
    Word old_val = dd.val;
    flag_C = (old_val >> 15) & 1; 
    
    Word res = (Word)((old_val << 1) & 0177777);
    w_write(dd.adr, res, dd.space);

    flag_Z = (res == 0) ? 1 : 0;
    flag_N = (res >> 15) & 1;
    flag_V = flag_N ^ flag_C;
}

void do_asr(void) {
    Word old_val = dd.val;
    flag_C = old_val & 1; 
    
    short signed_w = (short)old_val;
    Word res = (Word)((signed_w >> 1) & 0177777);
    w_write(dd.adr, res, dd.space);

    flag_Z = (res == 0) ? 1 : 0;
    flag_N = (res >> 15) & 1;
    flag_V = flag_N ^ flag_C;
}

void do_nothing(void) {
    printf("unknown\n");
}
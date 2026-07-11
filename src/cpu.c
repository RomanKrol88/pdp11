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
    {0170000, 0040000,  "bic",      do_bic,     HAS_SS | HAS_DD},
    {0170000, 0140000,  "bicb",     do_bic,     HAS_SS | HAS_DD},
    {0177400, 0101000,  "bhi",      do_bhi,     HAS_XX}, 
    {0177400, 0003400,  "ble",      do_ble,     HAS_XX}, 
    {0177400, 0002400,  "blt",      do_blt,     HAS_XX}, 
    {0177400, 0101400,  "blos",     do_blos,    HAS_XX}, 
    {0177400, 0100400,  "bmi",      do_bmi,     HAS_XX}, 
    {0177400, 0001000,  "bne",      do_bne,     HAS_XX}, 
    {0177400, 0100000,  "bpl",      do_bpl,     HAS_XX},
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
    {0177777, 0000000,  "halt",     do_halt,    NO_PARAMS},
    {0177700, 0005200,  "inc",      do_inc,     HAS_DD},
    {0177700, 0105200,  "incb",     do_inc,     HAS_DD},
    {0177700, 0000100,  "jmp",      do_jmp,     HAS_DD},
    {0177000, 0004000,  "jsr",      do_jsr,     HAS_RLEFT | HAS_DD},
    {0170000, 0010000,  "mov",      do_mov,     HAS_SS | HAS_DD},
    {0170000, 0110000,  "movb",     do_mov,     HAS_SS | HAS_DD},
    {0177000, 0070000,  "mul",      do_mul,     HAS_RLEFT | HAS_DD},
    {0177700, 0005400,  "neg",      do_neg,     HAS_DD},
    {0177700, 0105400,  "negb",     do_neg,     HAS_DD},
    {0177777, 0000240,  "nop",      do_clr_fl,  NO_PARAMS},
    {0177777, 0000005,  "reset",    do_reset,   NO_PARAMS},
    {0177700, 0006100,  "rol",      do_rol,     HAS_DD},
    {0177700, 0106100,  "rolb",     do_rol,     HAS_DD},
    {0177700, 0006000,  "ror",      do_ror,     HAS_DD},
    {0177700, 0106000,  "rorb",     do_ror,     HAS_DD},
    {0177770, 0000200,  "rts",      do_rts,     HAS_RRIGHT},
    {0177700, 0005600,  "sbc",      do_sbc,     HAS_DD},
    {0177700, 0105600,  "sbcb",     do_sbc,     HAS_DD},
    {0177777, 0000277,  "scc",      do_set_fl,  NO_PARAMS},
    {0177777, 0000261,  "sec",      do_set_fl,  NO_PARAMS},
    {0177777, 0000270,  "sen",      do_set_fl,  NO_PARAMS},
    {0177777, 0000262,  "sev",      do_set_fl,  NO_PARAMS},
    {0177777, 0000264,  "sez",      do_set_fl,  NO_PARAMS},
    {0177000, 0077000,  "sob",      do_sob,     HAS_RLEFT | HAS_NN},
    {0170000, 0160000,  "sub",      do_sub,     HAS_SS | HAS_DD},
    {0177700, 0000300,  "swab",     do_swab,    HAS_DD},
    {0177700, 0006700,  "sxt",      do_sxt,     HAS_DD},
    {0177700, 0005700,  "tst",      do_tst,     HAS_DD},
    {0177700, 0105700,  "tstb",     do_tst,     HAS_DD},
    {0177000, 0074000,  "xor",      do_xor,     HAS_RLEFT | HAS_DD},
    {0177777, 0000002,  "rti",      do_rti,     NO_PARAMS},
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

void reg_dump() {
    print_log(LOG_TRACE, "R0:%o R1:%o R2:%o R3:%o R4:%o R5:%o R6:%o R7:%o", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
}

Command parse_cmd(Word w) {

    byte_cmd = (w >> 15) & 1;

    //поиск в таблице команд
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if ((w & command[i].mask) == command[i].opcode) {   
            //проверка флага SS
            if (command[i].params & HAS_SS) {
                ss = get_operand(w >> 6);
            }
            //проверка флага DD
            if (command[i].params & HAS_DD) {
                dd = get_operand(w);
            }
            //проверка флага RLEFT
            if (command[i].params & HAS_RLEFT) {
                r = (w >> 6) & 7;
            }
            //проверка флага RRIGHT
            if (command[i].params & HAS_RRIGHT) {
                r = w & 7;
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
    return command[COMMAND_COUNT - 1];
}

void run(void) {
    //следующее слово будем читать по адресу 1000 (восьмеричное)
    PC = 01000;

    Word w;     //текущее слово, которое содержит команду
    
    while(1) {
        timer_tick();                                   //вызываем обработчик таймера на каждом шаге цикла процессора
        w = w_read(PC);                                 //читаем текущее слово
        Address current_pc = PC;                        //сохраняем текущее значение РС для вывода в лог
        PC += 2;                                        //PC сразу же указывает на следующее неразобранное слово
        Command cmd = parse_cmd(w);                     //декодируем считанное слово

        interrupts();                                   //проверяем, нет ли запроса от периферии на прерывание

        //печатаем лог в стиле MACRO-11
        if (strcmp(cmd.name, "unknown") == 0) {
        } else if (strcmp(cmd.name, "halt") == 0 || strcmp(cmd.name, "ccc") == 0   ||
                   strcmp(cmd.name, "clc") == 0  || strcmp(cmd.name, "clv") == 0   ||
                   strcmp(cmd.name, "clz") == 0  || strcmp(cmd.name, "cln") == 0   ||
                   strcmp(cmd.name, "nop") == 0  || strcmp(cmd.name, "reset") == 0 ||
                   strcmp(cmd.name, "scc") == 0  || strcmp(cmd.name, "sec") == 0   ||
                   strcmp(cmd.name, "sev") == 0  || strcmp(cmd.name, "sez") == 0   ||
                   strcmp(cmd.name, "sen") == 0) {
            print_log(LOG_TRACE, "%06o %06o: %s", current_pc, w, cmd.name);
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
        } else {
            print_log(LOG_TRACE, "%06o %06o: %s %s, %s", current_pc, w, cmd.name, ss.name, dd.name);
        }

        // выполняем команду
        cmd.do_command();

        reg_dump();
    }
}

Arg get_operand(Word w) {
    Arg res;
    memset(&res, 0, sizeof(Arg));

    Address pointer_adr;    // указатель на адрес
    Word x;                 // смещение (для моды 6 и 7)
    int m = (w >> 3) & 7;   // номер моды
    int r = w & 7;          // номер регистра

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
                //маскируем верхний байт после знакового расширения, чтобы не забить его единицами 0xFFXX
                res.val = (Word)((signed char)b_read(res.adr) & 0xFF); 
            } else {
                res.val = w_read(res.adr);              //по адресу Word - значение
            }                                       
            sprintf(res.name, "(R%d)", r);              //трассировка
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

            //трассировка
            if (r == 7) sprintf(res.name, "#%o", res.val);
            else sprintf(res.name, "(R%d)+", r);
            
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
            res.val = w_read(res.adr);                  //по целевому адресу - значение

            //трассировка
            if (r == 7) sprintf(res.name, "@#%o", res.adr);
            else sprintf(res.name, "@(R%d)+", r);

            reg[r] += 2;                                //автоинкремент регистра (всегда +2)
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
            sprintf(res.name, "-(R%d)", r);             //трассировка
            break;

        //мода 5, @-(R1)
        case 5:
            reg[r] -= 2;                                //автодекремент регистра (всегда -2)
            pointer_adr = reg[r];                       //в регистре адрес
            res.adr = w_read(pointer_adr);              //по адресу - целевой адрес
            res.val = w_read(res.adr);                  //по целевому адресу - значение
            sprintf(res.name, "@-(R%d)", r);            //трассировка
            break;

        //мода 6, X(R1) или X(PC)
        case 6:
            x = w_read(PC); 
            PC += 2;
            res.adr = (Address)(reg[r] + (short)x);     //адрес указателя со смещением
            res.val = w_read(res.adr);                  //по адресу - значение

            //трассировка
            if (r == 7) sprintf(res.name, "%o", res.adr);
            else sprintf(res.name, "%o(R%d)", x, r);
            break;

        //мода 7, @X(R1) или @X(PC)
        case 7:
            x = w_read(PC); 
            PC += 2;
            pointer_adr = (Address)(reg[r] + (short)x); //адрес указателя со смещением
            res.adr = w_read(pointer_adr);              //по адресу - целевой адрес
            res.val = w_read(res.adr);                  //по целевому адресу - значение

            //трассировка
            if (r == 7) sprintf(res.name, "@#%o", res.adr);
            else sprintf(res.name, "@#%o", res.adr);
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

void set_flag_C(DWord val) {
    if (byte_cmd) {
        //для байта перенос возникает, если результат вышел за пределы 8 бит (9-й бит взведен)
        flag_C = (val >> 8) & 1; 
    } else {
        //для слова перенос возникает, если результат вышел за пределы 16 бит (17-й бит взведен)
        flag_C = (val >> 16) & 1; 
    }
}

void timer_tick(void) {
    static int instruction_counter = 0;
    
    instruction_counter++;
    
    //"тик" каждые 1000 выполненных инструкций
    if (instruction_counter >= 1000) {
        timer_lks |= 0200;          //взводим 7-й бит готовности (LCM = 1)
        instruction_counter = 0;    //сбрасываем счётчик инструкций
    }
}

Word get_psw(void) {
    Word psw = 0;
    if (flag_N) psw |= 010; // 3-й бит
    if (flag_Z) psw |= 004; // 2-й бит
    if (flag_V) psw |= 002; // 1-й бит
    if (flag_C) psw |= 001; // 0-й бит
    return psw;
}

void interrupts(void) {
    //условие прерывания таймера: взведены и флаг тика (0200) и разрешение прерываний (0100)
    if ((timer_lks & 0300) == 0300) {
        
        timer_lks &= ~0200; 
        
        //записываем текущие флаги PSW
        SP -= 2;
        w_write(SP, get_psw(), MEMSPACE);
        
        //записываем текущий PC
        SP -= 2;
        w_write(SP, PC, MEMSPACE);
        
        //загружаем новый PC из вектора прерывания таймера (адрес 000100)
        PC = w_read(000100);
        
        //сбрасываем флаги
        flag_N = 0;
        flag_Z = 0;
        flag_V = 0;
        flag_C = 0;
        
        print_log(LOG_TRACE, ">>> INTERRUPT: Timer triggered! Vector 0100 loaded. New PC: %06o", PC);
    }
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
        //MOVb
        Byte b = (Byte)(ss.val & 0xFF);
        Word expanded_res = (Word)((signed char)b);

        if (dd.space == REGSPACE) {
            w_write(dd.adr, expanded_res, REGSPACE); 
        } else {
            b_write(dd.adr, b);
        }
        
        set_flags_NZ(expanded_res);
    } 
    else {
        //MOV
        w_write(dd.adr, ss.val, dd.space);
        set_flags_NZ(ss.val);
    }

    flag_V = 0;
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
    Word target_pc = dd.adr;
    SP -= 2;
    w_write(SP, reg[r], MEMSPACE);
    reg[r] = PC;
    PC = target_pc;
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
        //работа с байтами
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
        //работа со словом
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
            reg[dd.adr] = (signed char)res;
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
            reg[dd.adr] = (signed char)res;
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
    Word final_res = 0;

    if (byte_cmd) {
        //CMPb
        Byte s = (Byte)(ss.val & 0xFF);
        Byte d = (Byte)(dd.val & 0xFF);

        DWord res32 = (DWord)s - (DWord)d;
        final_res = (Byte)(res32 & 0xFF);

        flag_C = (s < d) ? 1 : 0;
        flag_V = (((s >> 7) != (d >> 7)) && ((final_res >> 7) == (d >> 7))) ? 1 : 0;
    } 
    else {
        //CMP
        Word s = ss.val;
        Word d = dd.val;

        DWord res32 = (DWord)s - (DWord)d;
        final_res = (Word)(res32 & 0xFFFF);

        flag_C = (s < d) ? 1 : 0;
        flag_V = (((s >> 017) != (d >> 15)) && ((final_res >> 15) == (d >> 15))) ? 1 : 0;
    }

    set_flags_NZ(final_res);
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
    //мода 0 (прямая адресация регистра) для JMP запрещена
    if (dd.space == REGSPACE) {
        print_log(LOG_ERROR, "HALT: Illegal JMP instruction using Register Mode 0 at address %06o", PC - 2);
        do_halt();
        return;
    }

    PC = dd.adr;
}

void do_neg(void) {
    Word final_res = 0;

    if (byte_cmd) {
        //NEGb
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
        //NEG
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

    w_write(dd.adr, final_res, dd.space);

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

    int r_high = r;
    int r_low = r | 1;

    reg[r_high] = (Word)((res32 >> 16) & 0xFFFF);
    reg[r_low] = (Word)(res32 & 0xFFFF);

    flag_Z = (signed_res == 0);
    flag_N = (res32 >> 31) & 1;
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
    //возврат PC из стека
    PC = w_read(SP);
    SP += 2;
    
    //возврат PSW из стека и восстановление флагов
    Word old_psw = w_read(SP);
    SP += 2;
    
    flag_N = (old_psw & 010) ? 1 : 0;
    flag_Z = (old_psw & 004) ? 1 : 0;
    flag_V = (old_psw & 002) ? 1 : 0;
    flag_C = (old_psw & 001) ? 1 : 0;
    
    print_log(LOG_TRACE, ">>> RTI: Returned from interrupt. Restored PC: %06o", PC);
}

void do_unknown(void) {
    Word w = w_read(PC - 2);
    print_log(LOG_ERROR, "Unknown instruction %06o at address %06o", w, PC - 2);
    exit(1);
}
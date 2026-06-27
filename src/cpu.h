#ifndef CPU_H
#define CPU_H

//#include "config.h"
//#include "logger.h"

#define SP reg[6]       //Stack Pointer (отдельно выделенный регистр R6)
#define PC reg[7]       //Program Counter (отдельно выделенный регистр R7)

#define REGSPACE 1      //пространство адресов регистров
#define MEMSPACE 0      //пространство адресов оперативной памяти

//битовые флаги параметров команд:
#define NO_PARAMS 0
#define HAS_DD    (1 << 0)  //1  (000001) - DD (6 бит)
#define HAS_SS    (1 << 1)  //2  (000010) - SS (6 бит)
#define HAS_R     (1 << 2)  //4  (000100) - R (3 бита)
#define HAS_N     (1 << 3)  //8  (001000) - N (3 бита)
#define HAS_NN    (1 << 4)  //16 (010000) - NN (6 бит)
#define HAS_XX    (1 << 5)  //32 (100000) - XX (8 бит)

//флаги условий регистра состояния PSW
extern int flag_N;  //Negative (результат отрицательный)
extern int flag_Z;  //Zero     (результат равен нулю)
extern int flag_V;  //oVerflow (Знаковое переполнение)
extern int flag_C;  //Carry    (перенос из старшего разряда

extern int byte_cmd;
extern int xx;

typedef struct {
    Word val;       //значение
    Address adr;    //адрес
    int space;      //пространство адресов (регистры или ОЗУ)
} Arg;

typedef struct {
    Word mask;
    Word opcode;
    char * name;
    void (*do_command)(void);
    char params;
} Command;

void format_arg(Arg arg, Word bits, char *out_str);             //функция вывода на печать лога
void reg_dump(void);                                            //функция дампа регистров
void run(void);                                                 //функция распознавания и запуска программ
Arg get_mr(Word w);                                             //функция разбора агрумента на моду и регистр
Command parse_cmd(Word w);                                      //декодер команд процессора
void w_reg_write(int r, Word val);                              //функция записи слова в регистр
void set_flags_mov(Word val);                                   //функция установки флагов для команд MOV, MOVB, CLR
void set_flags_add(Word src, Word dst, unsigned int res32);     //функция установки флагов для команды ADD

//команды процессора:
void do_halt(void);                                     //функция остановки HALT (opcode = 000000)
void do_add(void);                                      //..........НАПИСАТЬ ОПИСАНИЕ КОМАНД..........
void do_mov(void);
void do_sob(void);
void do_clr(void);
void do_br(void);
void do_bpl(void);
void do_bne(void);
void do_beq(void);
void do_nothing(void);

#endif
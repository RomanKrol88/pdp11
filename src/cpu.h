#ifndef CPU_H
#define CPU_H

#include "config.h"
#include "logger.h"

#define SP reg[6]       //Stack Pointer (отдельно выделенный регистр R6)
#define PC reg[7]       //Program Counter (отдельно выделенный регистр R7)

#define REGSPACE 1      //пространство адресов регистров
#define MEMSPACE 0      //пространство адресов оперативной памяти

//битовые флаги параметров команд:
#define NO_PARAMS   0
#define HAS_DD      (1 << 0)  //1   (0000001) - DD (6 бит) - источник в битах 6-11
#define HAS_SS      (1 << 1)  //2   (0000010) - SS (6 бит) - получатель в битах 0-5
#define HAS_NN      (1 << 4)  //16  (0010000) - NN (6 бит) - смещение для цикла (биты 0-5)
#define HAS_XX      (1 << 5)  //32  (0100000) - XX (8 бит) - смещение для веток (биты 0-7)
#define HAS_RLEFT   (1 << 6)  //64  (1000000) - R (3 бита) - регистр связи в битах 6-8 (XOR, MUL, DIV, ASH)
#define HAS_RRIGHT  (1 << 7)  //128 (2000000) - R (3 бита) - регистр связи в битах 0-2 (RTS)

#define EMULATOR_EXIT(code, message, ...) \
    do { \
        if ((message) != NULL && (message)[0] != '\0') { \
            print_log(LOG_ERROR, "FATAL ERROR (Exit Code %d): " message, (code), ##__VA_ARGS__); \
        } \
        exit(code); \
    } while (0)

extern int output_print;    //переменная для беспрефиксного вывода stdout на дисплей

//флаги условий регистра состояния PSW
extern int flag_N;  //Negative (результат отрицательный)
extern int flag_Z;  //Zero     (результат равен нулю)
extern int flag_V;  //oVerflow (Знаковое переполнение)
extern int flag_C;  //Carry    (перенос из старшего разряда

extern int byte_cmd;
extern int xx;

extern Byte timer_lks;
extern Byte keyboard_rcsr;

typedef struct {
    Word val;       //значение операнда
    Address adr;    //адрес операнда
    int space;      //пространство адресов (регистры или ОЗУ)
    char name[32];  //имя аргумента для трассировки
} Arg;

typedef struct {
    Word mask;
    Word opcode;
    char * name;
    void (*do_command)(void);
    char params;
} Command;

typedef union {
    float f;
    unsigned int u32;
    struct {
        unsigned short lo;
        unsigned short hi;
    } words;
} DecFloat;

//коды ошибок
typedef enum {
    EXIT_SUCCESS_HALT     = 0,  //процессор успешно остановлен командой HALT

    //оригинальные трапы PDP-11 (DEC Standard)
    EXIT_MEM_ALIGNMENT    = 4,  //трап по вектору 4: нечетный адрес (Bus Error)
    EXIT_MEM_BOUNDS       = 10, //трап по вектору 10: reserved instruction / ошибка адресации

    //cтандартные системные коды (POSIX/Unix)
    EXIT_TEST_FAILED      = 1,  //сбой в юнит-тесте (CI/CD стандарт для Unix)
    EXIT_FILE_CORRUPTION  = 11, //ошибка парсинга: повреждение данных в файле
    EXIT_FILE_INVALID_VAL = 12, //ошибка парсинга: неверное значение байта
    EXIT_FILE_MEM_BOUNDS  = 13, //ошибка парсинга: выход за границы ОЗУ при загрузке
    EXIT_FILE_UNKNOWN     = 14  //ошибка файловой системы (не удалось открыть файл)
} ExitCode;

void reg_dump(void);                            //функция дампа регистров
void run(void);                                 //функция распознавания и запуска программ
Arg get_operand(Word op_bits);                  //функция разбора агрумента на моду и регистр и вывода на печать
Command parse_cmd(Word inst_word);              //декодер команд процессора
void w_reg_write(int reg_num, Word value);      //функция записи слова в регистр
void set_flags_NZ(Word val);                    //функция выставления флагов N и Z
void set_flag_C(DWord val_32);                  //функция выставления флага переноса C по 32-битному результату
void timer_tick(void);                          //функция обработки тика таймера
Word get_psw(void);                             //функция упаковки текущих флагов PSW в одно 16-битное слово
void interrupts(void);                          //функция проверки и выполнения прерываний от периферии
float read_dec_float(Address addr);             //функция чтения 32-битного float из ОЗУ PDP-11
void write_dec_float(Address addr, float val);  //функция записи 32-битного float в ОЗУ PDP-11
void do_fis_math(const char* op_name);          //универсальный обработчик вещественных FIS-команд

//команды процессора:
//арифметика и пересылки данных
void do_mov(void);      // MOV    [01SSDD] NZVC=**0- | Пересылка данных (d = s)
void do_add(void);      // ADD    [06SSDD] NZVC=**** | Сложение слов (d = s + d)
void do_sub(void);      // SUB    [16SSDD] NZVC=**** | Вычитание слов (d = d - s)
void do_cmp(void);      // CMP    [020000] NZVC=**** | Сравнение (s - d)
void do_tst(void);      // TSTb   [B055DD] NZVC=**00 | Проверка (d), без записи
void do_clr(void);      // CLR    [0050DD] NZVC=0100 | Очистка слова или байта (d = 0)
void do_inc(void);      // INC    [B052DD] NZVC=***- | Увеличение на 1 (d = d + 1)
void do_dec(void);      // DEC    [B053DD] NZVC=***- | Уменьшение на 1 (d = d - 1)
void do_neg(void);      // NEG    [B054DD] NZVC=**** | Смена знака (d = -d)

//многоразрядная математика и сдвиги
void do_adc(void);      // ADC    [B055DD] NZVC=**** | Прибавление переноса (d = d + C)
void do_sbc(void);      // SBC    [B056DD] NZVC=**** | Вычитание переноса (d = d - C)
void do_sxt(void);      // SXT    [0067DD] NZVC=-*0- | Знаковое расширение флага N в слово (d = 0 или -1)
void do_mul(void);      // MUL    [070RSS] NZVC=**0* | Умножение слов (R, R|1 = R * SS) (EIS)
void do_div(void);      // DIV    [071RSS] NZVC=**** | Деление слов (R = частное, R|1 = остаток) (EIS)
void do_ash(void);      // ASH    [072RSS] NZVC=**** | Арифметический сдвиг регистра (r = r * 2^s) (EIS)
void do_ashc(void);     // ASHC   [073RSS] NZVC=**** | Арифметический сдвиг пары регистров (EIS)
void do_asl(void);      // ASL    [0063DD] NZVC=**** | Арифметический сдвиг влево (d = d * 2)
void do_asr(void);      // ASR    [0062DD] NZVC=**** | Арифметический сдвиг вправо (d = d / 2)
void do_rol(void);      // ROL    [B061DD] NZVC=**** | Циклический сдвиг влево через перенос
void do_ror(void);      // ROR    [B060DD] NZVC=**** | Циклический сдвиг вправо через перенос
void do_swab(void);     // SWAB   [0003DD] NZVC=**00 | Побайтовый обмен в слове (флаги по новому младшему байту)
void do_fadd(void);     // FADD   [07660R] NZVC=**00 | Вещественное сложение через стек (Rn) (FIS)
void do_fsub(void);     // FSUB   [07661R] NZVC=**00 | Вещественное вычитание через стек (Rn) (FIS)
void do_fmul(void);     // FMUL   [07662R] NZVC=**00 | Вещественное умножение через стек (Rn) (FIS)
void do_fdiv(void);     // FDIV   [07663R] NZVC=**00 | Вещественное деление через стек (Rn) (FIS)

//побитовая логика
void do_bic(void);      // BIC    [B4SSDD] NZVC=**0- | Сброс битов по маске (d = d & {~s})
void do_bis(void);      // BIS    [B5SSDD] NZVC=**0- | Установка битов / Логическое ИЛИ (d = d | s)
void do_bit(void);      // BIT    [B3SSDD] NZVC=**0- | Проверка битов / Логическое И (d & s), без записи
void do_com(void);      // COM    [000510] NZVC=**01 | Побитовая инверсия получателя (d = ~d)
void do_xor(void);      // XOR    [074RDD] NZVC=**0- | Исключающее ИЛИ регистра и памяти (d = d ^ r)

//безусловные переходы и подпрограммы
void do_jmp(void);      // JMP    [0001DD] NZVC=---- | Безусловный аппаратный переход (PC = d)
void do_jsr(void);      // JSR    [004RDD] NZVC=---- | Вызов подпрограммы (r = PC, PC = d) / CALL [0047DD]
void do_rts(void);      // RTS    [00020R] NZVC=---- | Возврат из подпрограммы (PC = r, r = (SP)+) / RETURN [000207]

//команды ветвления (условные переходы)
void do_br(void);       // BR     [0004XX] NZVC=---- | Безусловное ветвление (PC = PC + 2 * XX)
void do_sob(void);      // SOB    [077RNN] NZVC=---- | Вычитание 1 и переход назад, если r != 0 (PC = PC - 2 * NN)
void do_bcc(void);      // BCC    [0003400] NZVC=----| Переход, если перенос очищен (If C = 0) / BHIS
void do_bcs(void);      // BCS    [0003000] NZVC=----| Переход, если перенос взведен (If C = 1) / BLO
void do_beq(void);      // BEQ    [0001400] NZVC=----| Переход, если равно / ноль (If Z = 1)
void do_bne(void);      // BNE    [0001000] NZVC=----| Переход, если не равно / не ноль (If Z = 0)
void do_bpl(void);      // BPL    [0000200] NZVC=----| Переход, если плюс / положительно (If N = 0)
void do_bmi(void);      // BMI    [0100400] NZVC=----| Переход, если минус / отрицательно (If N = 1)
void do_bvc(void);      // BVC    [0002400] NZVC=----| Переход, если нет переполнения (If V = 0)
void do_bvs(void);      // BVS    [0002000] NZVC=----| Переход, если есть переполнение (If V = 1)
void do_bge(void);      // BGE    [0002000] NZVC=----| Знаковый переход «больше или равно» (If N xor V = 0)
void do_blt(void);      // BLT    [0002400] NZVC=----| Знаковый переход «меньше» (If N xor V = 1)
void do_bgt(void);      // BGT    [0003000] NZVC=----| Знаковый переход «больше» (If Z or {N xor V} = 0)
void do_ble(void);      // BLE    [0003400] NZVC=----| Знаковый переход «меньше или равно» (If Z or {N xor V} = 1)
void do_bhi(void);      // BHI    [0101000] NZVC=----| Беззнаковый переход «строго больше» (If C or Z = 0)
void do_blos(void);     // BLOS   [0101400] NZVC=----| Беззнаковый переход «меньше или равно» (If C or Z = 1)

//управление флагами PSW и системные команды
void do_clr_fl(void);   // CCC/CLC/CLN/CLV/CLZ [00024X] NZVC=маска | Очистка флагов PSW по битовой маске из опкода
void do_set_fl(void);   // SCC/SEC/SEN/SEV/SEZ [00026X] NZVC=маска | Установка флагов PSW по битовой маске из опкода
void do_nop(void);      // NOP    [000240] NZVC=---- | Пустая операция (холостой ход)
void do_reset(void);    // RESET  [000005] NZVC=---- | Сброс внешней шины периферии (холостой ход CPU)
void do_halt(void);     // HALT   [000000] NZVC=---- | Останов процессора и завершение работы эмулятора

//служебные
void do_rti(void);      // RTI    [000002] NZVC=vvvv | Возврат из обработчика прерывания через стек
void do_emt(void);      // EMT    [104000] NZVC=0000 | Программный трап эмулятора по вектору 000030
void do_trap(void);     // TRAP   [104400] NZVC=0000 | Программный пользовательский трап по вектору 000034
void do_unknown(void);  // DU     [------] NZVC=---- | Заглушка нереализованных зон дешифратора опкодов

#endif
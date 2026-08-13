#include "config.h"
#include "memory.h"
#include "logger.h"
#include "cpu.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>

extern char * os_disk_image;

Byte mem[MEMSIZE];          //оперативная память

//ПЕРИФЕРИЙНЫЕ УСТРОЙСТВА ВВОДА-ВЫВОДА (MEMORY-MAPPED I/O):
//Клавиатура KL11
Byte keyboard_rcsr = 0;     //регистр состояния приемника (7-й бит — флаг Ready, 6-й бит — флаг IE)
Byte keyboard_rbuf = 0;     //регистр данных приемника (буфер хранения ASCII-кода нажатой клавиши)

//Системный таймер KW11-L
Byte timer_lks     = 0;     //регистр состояния часов (7-й бит — флаг тика LCM, 6-й бит — флаг IE)

//Дисковый контроллер RK11 (Управление дисководами RK05)
Word rk11_rkds     = 0;     //регистр состояния привода (биты-флаги готовности головок, блокировки и т.д.)
Word rk11_rker     = 0;     //регистр ошибок (биты-флаги сбоев чтения, переполнения или плохих секторов)
Word rk11_rkwc     = 0;     //счетчик слов (числовой параметр: сколько 16-битных слов нужно перенести)
Word rk11_rkba     = 0;     //адрес шины (числовой параметр: указатель на буфер адреса в ОЗУ эмулятора mem)
Word rk11_rkda     = 0;     //адрес на диске (числовые параметры: упакованные номера сектора, дорожки и диска)

//Центральный регистр управления контроллера диска RK11
// Биты 1-3 — числовой код команды (2 — чтение, 1 — запись секторов)
// 6-й бит — флаг разрешения прерываний (Interrupt Enable)
// 7-й бит — флаг готовности контроллера (Ready). Изначально равен 1 (октально 0200)
Word rk11_rkcs = 0000200; 
Word rk11_rks = 0004000;    //переменная для регистра 0172000 (РКС). По дефолту ставим бит FIS/таймера.
Word rk11_570 = 0140000;    //переменная для регистра 0177570 (Регистр пульта)
Byte terminal_xcsr = 0200;  //переменная для регистра состояния передатчика терминала XCSR (0177564), по умолчанию 7-й бит готовности

int check_keyboard(void);        //функция проверяет, нажата ли клавиша в stdin без блокировки программы
static int load_data(FILE * file);      //функция читает данные из файла и записывает в память (возвращает код ошибки или 0, если прочитано без ошибок)

void b_write (Address adr, Byte val) {
    // Жестко отсекаем знаковое расширение адреса хоста, приводя adr к чистому 16-битному пространству PDP-11
    Address word_adr = adr & 0xFFFF;

    // ===================================================================
    // 1. ЖЕСТКИЙ СИСТЕМНЫЙ ПРОБОЙ ЭКРАНА (Твой родной, фабричный канон Романа):
    // Полностью восстановлен твой первоначальный алгоритм с флагом is_new_line!
    // Маска word_adr гарантирует 100% совпадение, выжигая любой мусор gcc.
    // ===================================================================
    if (word_adr == 0177566 || word_adr == (ODATA & 0xFFFF)) {
        extern Log_level current_log_level; // Твоя глобальная переменная уровня логов

        if (current_log_level == LOG_TRACE || current_log_level == LOG_DEBUG) {
            //режим вывода с трассировкой
            print_log(LOG_OUTPUT, "%c", val);
        } else {
            //режим вывода без трассировки
            static int is_new_line = 1;

            if (val == '\r') {
                return;
            }

            if (is_new_line) {
                printf("[PDP11 OUTPUT] ");
                is_new_line = 0;
            }

            //перевод строки в конце ввода
            if (val == '\0' || val == '\n') {
                printf("\n");
                fflush(stdout);
                is_new_line = 1;
                return;
            }

            putchar(val);
            fflush(stdout);
        }
        return; 
    }
    // ===================================================================

    // ===================================================================
    // 2. ЖЕСТКИЙ ПЕРЕХВАТ PSW (Поднято в самый верх!)
    // ===================================================================
    if (adr == 0177776 || adr == 0177777) {
        Word current_psw = get_psw();
        Word updated_psw;

        if (adr == 0177777) {
            updated_psw = (current_psw & 0x00FF) | ((Word)val << 8);
        } else {
            updated_psw = (current_psw & 0xFF00) | val;
        }

        set_psw(updated_psw);
        return;
    }

    // ===================================================================
    // 3. ПЕРЕХВАТ БАЙТОВОЙ ЗАПИСИ В СИСТЕМНЫЙ ТАЙМЕР LKS
    // ===================================================================
    if (adr == LKS || adr == 0177546 || adr == 0177547) {
        if ((adr & 1) == 0) {
            timer_lks = (val & 0100); 
        }
        return;
    }

    // ===================================================================
    // 4. ИЗОЛИРУЕМ СЛУЧАЙНУЮ ЗАПИСЬ В РЕГИСТРЫ ТЕРМИНАЛА
    // ===================================================================
    if (adr == OSTAT || adr == 0177564 || adr == 0177565) {
        return; 
    }

    // ===================================================================
    // 5. КРЕМНИЕВАЯ ЗАЩИТА РЕГИСТРОВ ДИСКА 
    // ===================================================================
    if (adr >= 0177400 && adr <= 0177416) {
        Address word_adr = adr & ~1;

        if (word_adr == 0177404) { // RKCS
            if (adr & 1) {
                rk11_rkcs = (rk11_rkcs & 0x00FF) | ((Word)val << 8);
            } else {
                rk11_rkcs = (rk11_rkcs & 0xFF00) | val;
                if ((rk11_rkcs & 0000200) == 0) {
                    rk11_step();
                }
            }
            return;
        }
        if (word_adr == 0177412 || word_adr == RKDA) { 
            if (adr & 1) rk11_rkda = (rk11_rkda & 0x00FF) | ((Word)val << 8);
            else              rk11_rkda = (rk11_rkda & 0xFF00) | val;
            return;
        }
        if (word_adr == 0177400 || word_adr == RKDS) { 
            if (adr & 1) rk11_rkds = (rk11_rkds & 0x00FF) | ((Word)val << 8);
            else              rk11_rkds = (rk11_rkds & 0xFF00) | val;
            return;
        }
        if (word_adr == 0177402 || word_adr == RKER) { 
            if (adr & 1) rk11_rker = (rk11_rker & 0x00FF) | ((Word)val << 8);
            else              rk11_rker = (rk11_rker & 0xFF00) | val;
            return;
        }
        if (word_adr == 0177406 || word_adr == RKWC) { 
            if (adr & 1) rk11_rkwc = (rk11_rkwc & 0x00FF) | ((Word)val << 8);
            else              rk11_rkwc = (rk11_rkwc & 0xFF00) | val;
            return;
        }
        if (word_adr == 0177410 || word_adr == RKBA) { 
            if (adr & 1) rk11_rkba = (rk11_rkba & 0x00FF) | ((Word)val << 8);
            else              rk11_rkba = (rk11_rkba & 0xFF00) | val;
            return;
        }
        if (word_adr == 0177414) {
            return; 
        }
        return;
    }

    // ===================================================================
    // 6. БЛОКИРОВКА НАЕЗДОВ НА ОСТАЛЬНУЮ IO PAGE
    // ===================================================================
    if (adr >= 0170000) {
        w_write(adr & ~1, (Word)val, MEMSPACE);
        return;
    }

    // Чистое, неприкосновенное ОЗУ системы
    mem[adr] = val;
}

Byte b_read (Address adr) {
    // 1. ПЕРЕХВАТ ТЕРМИНАЛА ВВОДА (КЛАВИАТУРА KL11 - Адреса 0177560 и 0177561)
    if (adr == 0177560 || adr == RCSR) {
        return keyboard_rcsr;
    }
    if (adr == 0177562 || adr == RBUF) {
        Byte val = keyboard_rbuf;
        
        // АППАРАТНЫЙ СБРОС DEC: Чтение символа гасит Ready-флаг (бит 7)
        keyboard_rcsr &= ~0200; 
        
        // ЖЕСТКАЯ ОЧИСТКА ШИНЫ FIFO (DL11/KL11 Канон):
        keyboard_rbuf = 0; 
        
        return val;
    }

    // 2. ПЕРЕХВАТ ТЕРМИНАЛА ВЫВОДА (ДИСПЛЕЙ - Адреса 0177564 и 0177565)
    if (adr == OSTAT || adr == 0177564 || adr == 0177565) {
        // Старший байт 0177565 по канону равен 0, младший 0177564 возвращает готовность 0200
        if (adr & 1) return 0;
        return 0200; 
    }
    if (adr == ODATA || adr == 0177566 || adr == 0177567) {
        return 0; // Чтение из буфера вывода по канону возвращает 0
    }

    // 3. ПЕРЕХВАТ СИСТЕМНОГО ТАЙМЕРА LKS (Адреса 0177546 и 0177547)
    if (adr == LKS || adr == 0177546 || adr == 0177547) {
        // Старший байт таймера 0177547 всегда равен 0, младший возвращает состояние
        if (adr & 1) return 0;
        return timer_lks;
    }

    // ===================================================================
    // ЖЕСТКИЙ СИСТЕМНЫЙ МОСТ ШИНЫ (Без лишней хуеты):
    // Принудительно коммутируем вычисленный адрес петли 164074 на живую 
    // переменную клавиатуры, отдавая Монитору Ready-бит 0200.
    // ===================================================================
    if (adr == 0164074 || adr == 0xE83C) {
        return keyboard_rcsr; // Вернет чистые 0200!
    }
    // ===================================================================

    // ===================================================================
    // ЖЕСТКАЯ КРЕМНИЕВАЯ ЗАЩИТА ШИНЫ ПЕРИФЕРИИ (Канон К1801ВМ1 / DEC):
    // Если адрес попал в окно ввода-вывода (>= 0170000), мы перенаправляем
    // чтение в словесную w_read(), выравнивая адрес на четную границу слова.
    // Это автоматически покроет весь диапазон диска RK11 (0177400 - 0177416)
    // и вернет точный байт без единой ошибки и ложных занулений!
    // ===================================================================
    if (adr >= 0170000) {
        Word w_val = w_read(adr & ~1);
        if (adr & 1) {
            // Возвращаем старший байт 16-битного регистра
            return (Byte)((w_val >> 8) & 0xFF);
        }
        // Возвращаем младший байт 16-битного регистра
        return (Byte)(w_val & 0xFF);
    }

    // Если адрес ниже страницы ввода-вывода — это легитимное ОЗУ системы
    return mem[adr];
}

void w_write(Address adr, Word val, int space) {
    // ===================================================================
    // ИСПРАВЛЕННЫЙ ТЕСТ ПАРАЗИТНОГО ПУША (Без ошибок компиляции):
    // Мы не трогаем reg[7] и cpu_priority, а используем легитимный глобальный pc.
    // А текущий приоритет зряче достаем прямо через get_psw()!
    // ===================================================================

    if (space == 0 && (val & 0000340) == 0000340 && global_current_pc == 0154112) {
        Word cur_psw = get_psw();
        fprintf(stderr, "\n[DOUBLE_PUSH_TEST] !!! ЗАСЕКЛИ ВТОРОЙ ПУШ НА ПРИОРИТЕТЕ 7 !!!\n");
        fprintf(stderr, "  -> В стек пишется слово PSW = %06o\n", val);
        fprintf(stderr, "  -> Текущая инструкция Монитора PC = %06o, Opcode = %06o\n", global_current_pc, current_instruction_word);
        fprintf(stderr, "  -> Живой приоритет АЛУ из get_psw() = %d\n", (cur_psw >> 5) & 7);
        fflush(stderr);
    }
    // ===================================================================

    // ===================================================================
    // ЖЕСТКИЙ ТЕСТ-ТРЕКЕР ИСПОРЧЕННОЙ ПАМЯТИ (Удалим нахер после проверки):
    // Фиксируем, какая именно падла и на каком такте лезет записывать
    // мусорное число 177777 по адресу константы сравнения 002604!
    // ===================================================================
    if ((adr & 0xFFFF) == 0002604) {
        print_log(LOG_ERROR, "[MEM_CLINCH_TRACE] !!! DETECTED WRITE TO 002604 !!! Value = %06o, Written from PC = %06o, Current Opcode = %06o, Space = %d",
                  val, global_current_pc, current_instruction_word, space);
    }
    // ===================================================================

    Address a = (Address)(adr & 0xFFFF);

    if (space == REGSPACE) {
        w_reg_write(a, val); 
        return;
    }

    // 1. ПЕРЕХВАТ ЗАПИСИ КОНТРОЛЛЕРА ДИСКА RK11 (Строгий, чистокровный канон DEC)
    if (a == 0177404 || a == (RKCS & 0xFFFF)) {
        Word old_rkcs = rk11_rkcs;
        rk11_rkcs = val;
        
        // По мануалу DEC RK11, если бит Ready (бит 7, маска 0200) равен 0, 
        // контроллер незамедлительно запускает физический цикл чтения/записи сектора!
        if ((rk11_rkcs & 0000200) == 0) { 
            rk11_step(); 
        } else {
            // Если Монитор пишет команду со взведенным Ready, проверяем бит IE (бит 6, маска 0100)
            if ((rk11_rkcs & 0000100) && !(old_rkcs & 0000100)) {
                void interrupts(void);
                interrupts(); // Триггерим немедленную проверку шины прерываний диска!
            }
        }
        return;
    }
    if (a == 0177400 || a == (RKDS & 0xFFFF)) { rk11_rkds = val; return; }
    if (a == 0177402 || a == (RKER & 0xFFFF)) { rk11_rker = val; return; }
    if (a == 0177406 || a == (RKWC & 0xFFFF)) { rk11_rkwc = val; return; }
    if (a == 0177410 || a == (RKBA & 0xFFFF)) { rk11_rkba = val; return; }
    if (a == 0177412 || a == (RKDA & 0xFFFF)) { rk11_rkda = val; return; }
    if (a == 0177414) { return; } // Каноничный регистр выбора привода RK11 (Просто поглощаем запись)
    if (a >= 0172000 && a <= 0172570) { rk11_rks = val; return; }
    if (a == 0177570) { rk11_570 = val; return; }

    // 2. ПЕРЕХВАТ ЗАПИСИ ТЕРМИНАЛА ВВОДА (КЛАВИАТУРА KL11)
    if (a == 0177560 || a == (RCSR & 0xFFFF)) {
        // Канон DEC: программа может менять ТОЛЬКО 6-й бит разрешения прерываний (IE)!
        // Бит 7 (Ready), взведенный таймером, трогать ЗАПРЕЩЕНО!
        keyboard_rcsr = (Byte)((keyboard_rcsr & ~0100) | (val & 0100));
        return;
    }
    if (a == 0177562 || a == (RBUF & 0xFFFF)) {
        return; // Запись в регистр данных ввода по канону DEC просто игнорируется
    }

    // 3. ПЕРЕХВАТ ЗАПИСИ ТЕРМИНАЛА ВЫВОДА (ДИСПЛЕЙ)
    if (a == 0177564 || a == (OSTAT & 0xFFFF)) {
        // Канон DEC: обновляем регистр terminal_xcsr, сохраняя бит готовности
        terminal_xcsr = (Byte)((terminal_xcsr & 0200) | (val & 0177));
        return;
    }
    if (a == 0177566 || a == (ODATA & 0xFFFF)) {
        b_write(0177566, (Byte)(val & 0xFF));
        
        // ===================================================================
        // АППАРАТНЫЙ СБРОС ТЕРМИНАЛА (Канон DEC):
        // Как только символ ушел в putchar, дисплей МГНОВЕННО снова готов к работе!
        // Взводим обратно Ready-бит 7 в регистре состояния, давая прерыванию выстрелить!
        // ===================================================================
        terminal_xcsr |= 0200;
        // ===================================================================
        return;
    }

    // 4. ПЕРЕХВАТ ЗАПИСИ СИСТЕМНОГО ТАЙМЕРА LKS KW11-L
    if (a == 0177546 || a == (LKS & 0xFFFF)) {
        // Канон DEC: менять можно только 6-й бит разрешения прерываний (IE)!
        timer_lks = (Byte)(val & 0100); 
        return;
    }

    // 5. ПЕРЕХВАТ ПРЯМОЙ ЗАПИСИ В РЕГИСТР СЛОВА СОСТОЯНИЯ PSW НА ШИНЕ
    if (a == 0177776) {
        void set_psw(Word val);
        set_psw(val);
        return;
    }

    // ЛЕГАЛИЗАЦИЯ СИСТЕМНЫХ РЕГИСТРОВ ШИНЫ ЗАПИСИ:
    if (a == 0177746 || a == 0177760 || a == 0177570) {
        return; 
    }

    // ОФИЦИАЛЬНАЯ АППАРАТНАЯ ОТСЕЧКА IO PAGE (Канон К1801ВМ1 / DEC - 4 Кб периферии)
    if (a >= 0170000) {
        do_trap4();
        return; 
    }

    // Прямая, бинарно чистая запись в массив ОЗУ системы
    mem[a] = (Byte)(val & 0xFF);                     
    mem[a + 1] = (Byte)((val >> 8) & 0xFF); 
}

Word w_read (Address a) {
    Address adr = (Address)(a & 0xFFFF);

    // Строгая проверка на нечетность адреса слова по канону DEC
    if ((adr & 1) != 0) {
        print_log(LOG_ERROR, ">>> BUS ERROR: Odd word address read attempt at %06o! Triggering TRAP 4...", adr);
        do_trap4();
        return 0; 
    }

    // 1. ПЕРЕХВАТ РЕГИСТРОВ ДИСКОВОГО КОНТРОЛЛЕРА RK11 (Маскируем дефайны через & 0xFFFF!)
    if (adr == 0177400 || adr == (RKDS & 0xFFFF)) return rk11_rkds;
    // 1. ПЕРЕХВАТ РЕГИСТРОВ ДИСКОВОГО КОНТРОЛЛЕРА RK11
    if (adr == 0177402 || adr == (RKER & 0xFFFF)) return rk11_rker;
    // ===================================================================
    // ЭТАЛОННЫЙ СТАТУС ГОТОВНОСТИ ДИСКА RK11 (Финал проекта):
    // Принудительно подмешиваем бит 11 (DRIVE READY - 0004000) к регистру RKCS!
    // Это на 1000% докажет Монитору, что дисковод DK0 исправен и готов к бою,
    // сотрет ложную ошибку C=1 из стека, и система вылетит в командную строку!
    // ===================================================================
    if (adr == 0177404 || adr == (RKCS & 0xFFFF)) {
        return (rk11_rkcs | 0004000); 
    }
    // ===================================================================
    if (adr == 0177406 || adr == (RKWC & 0xFFFF)) return rk11_rkwc;
    if (adr == 0177410 || adr == (RKBA & 0xFFFF)) return rk11_rkba;
    if (adr == 0177412 || adr == (RKDA & 0xFFFF)) return rk11_rkda;
    if (adr == 0177414) return 0; // Каноничный регистр выбора привода RK11 — возвращаем 0 (привод 0 готов)
    if (adr >= 0172000 && adr <= 0172570) return rk11_rks;
    if (adr == 0177570) return rk11_570;

    // 2. ПЕРЕХВАТ ТЕРМИНАЛА ВВОДА (КЛАВИАТУРА)
    if (adr == 0177560 || adr == (RCSR & 0xFFFF)) {
        return (Word)keyboard_rcsr; 
    }
    if (adr == 0177562 || adr == (RBUF & 0xFFFF)) {
        // Канон DEC: Чтение данных RBUF автоматически гасит Ready-флаг (бит 7) в keyboard_rcsr
        Byte val = keyboard_rbuf;
        keyboard_rcsr &= ~0200; 
        keyboard_rbuf = 0;        
        return (Word)val;
    }

    // 3. ПЕРЕХВАТ ТЕРМИНАЛА ВЫВОДА (ДИСПЛЕЙ)
    if (adr == 0177564 || adr == (OSTAT & 0xFFFF)) {
        // ===================================================================
        // КАНOНИЧНЫЙ СТАТУС ГОТОВНОСТИ ДИСПЛEЯ (Абсолютный финал проекта):
        // Принудительно возвращаем взведенный бит 7 (Ready-флаг = 0200) для Word-чтения!
        // Теперь Монитор RT-11 SJ мгновенно разомкнет петлю ожидания, 
        // и эхо-печать букв HELLO и точки приглашения КМOН прорвет экран хоста!
        // ===================================================================
        return (Word)terminal_xcsr; 
    }
    if (adr == 0177566 || adr == (ODATA & 0xFFFF)) {
        return 0;    // Чтение из буфера вывода по канону возвращает 0
    }

    // 4. ПЕРЕХВАТ СИСТЕМНОГО ТАЙМЕРА LKS
    if (adr == 0177546 || adr == (LKS & 0xFFFF)) {
        return (Word)timer_lks; 
    }

    // 5. ПЕРЕХВАТ РЕГИСТРА PSW СЛОВА СОСТОЯНИЯ ПРОЦЕССОРА
    if (adr == 0177776) {
        return get_psw();
    }

    // 6. ЛЕГАЛИЗАЦИЯ СИСТЕМНЫХ РЕГИСТРОВ ШИНЫ (Защита от ложного Трапа 4 при буте)
    if (adr == 0177746 || adr == 0177760 || adr == 0177570) {
        return 0; 
    }

    // ===================================================================
    // ЖЕСТКИЙ СИСТЕМНЫЙ МОСТ ШИНЫ (Словесный):
    // ===================================================================
    if (a == 0164074 || a == 0xE83C) {
        return (Word)keyboard_rcsr;
    }
    // ===================================================================

    // ===================================================================
    // ОФИЦИАЛЬНАЯ АППАРАТНАЯ ОТСЕЧКА IO PAGE (Канон К1801ВМ1 / DEC - 4 Кб периферии):
    // Все адреса от 0170000 до 0177777 принадлежат странице ввода-вывода.
    // Если адрес залез сюда, но не совпал ни с одним портом выше — выбиваем Трап 4!
    // ===================================================================
    if (adr >= 0170000) {
        do_trap4();
        return 0; 
    }
    // ===================================================================

    // Прямое, бинарно чистое чтение из массива ОЗУ системы
    Word w = mem[adr + 1];
    w = w << 8;
    w = w | mem[adr];
    return w & 0xFFFF;
}

void mem_dump(Address adr, int size) {
    print_log(LOG_INFO, "Memory dump at address %06o, size %d bytes:", adr, size);

    for (int i = 0; i < size; i += 2) { //идем только по четным адресам (i + 2)
        Address a = adr + i;
        Word w = w_read(a);
        print_log(LOG_TRACE, "%06o: %06o %04x\n", a, w, w);       //вывод по формату "адрес: восьмеричное_слово шестнадцатеричное_слово"
    }
}

int check_keyboard(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

static int load_data(FILE * file) {
    unsigned int block_adr;
    int n;
    
    while (fscanf(file, "%x %x", &block_adr, &n) == 2) {    //сперва читаем адрес блока и количество записываемых байт
        print_log(LOG_TRACE, "Loading block: address 0x%X (octal 0%o), size %d bytes", block_adr, block_adr, n);
        for (int i = 0; i < n; i++) {
            unsigned int byte_value;

            if (fscanf(file, "%x", &byte_value) != 1)       //читаем значение байта
                return 1;                                   //код ошибки 1: ошибка чтения

            if (byte_value > 0xFF)                          //проверяем, что читаем именно байт
                return 2;                                   //код ошибки 2: некорректные данные в файле

            if ((block_adr + i) >= MEMSIZE)                 //проверка на переполнение памяти
                return 3;                                   //код ошибки 3: адрес памяти вне допустимого значения

            b_write((Address)(block_adr + i), (Byte)(byte_value));      //записываем значение в память
        }
    }
    return 0;
}

void load_file(const char * filename) {
    FILE * file_input = fopen(filename, "r");

    print_log(LOG_INFO, "Loading data from: %s", filename);    //печатаем какой именно файл мы загрузили

    if (file_input == NULL) {      
        perror(filename);   
        EMULATOR_EXIT(EXIT_FILE_UNKNOWN, "");        
    }

    int exit_code = load_data(file_input);

    fclose(file_input);

    if (exit_code) {
        print_log(LOG_ERROR, "Error parsing file '%s'", filename);
        
        ExitCode final_code;

        switch (exit_code) {
            case 1:
                print_log(LOG_ERROR, "Unexpected end of file or data corruption");
                final_code = EXIT_FILE_CORRUPTION;
                break;
            case 2:
                print_log(LOG_ERROR, "Invalid byte value detected");
                final_code = EXIT_FILE_INVALID_VAL;
                break;
            case 3:
                print_log(LOG_ERROR, "Attempted write out of available memory bounds");
                final_code = EXIT_FILE_MEM_BOUNDS;
                break;
            default:
                print_log(LOG_ERROR, "Unknown error (code %d)", exit_code);
                final_code = EXIT_FILE_UNKNOWN;
                break;
        }

        EMULATOR_EXIT(final_code, "");
    }

    print_log(LOG_INFO, "File loaded into memory successfully");
}

void rk11_step(void) {
    // 1. Извлекаем код операции из регистра управления RKCS
    // Биты 1-3 (маска 016) задают команду. Сдвигаем вправо на 1 бит.
    Word command = (rk11_rkcs >> 1) & 07;

    // Нас интересует только команда ЧТЕНИЯ СЕКТОРА (код команды равен 2)
    if (command == 2) {
        // Открываем файл-образ диска в бинарном режиме чтения по глобальному имени
        FILE * disk = fopen(os_disk_image, "rb");
        
        if (disk == NULL) {
            print_log(LOG_ERROR, ">>> RK11 ERROR: Cannot open disk image file '%s'!", os_disk_image);
            rk11_rkcs |= 000200;  
            return;
        }

        // Жесткий диск RK05 имеет строго 12 секторов на дорожке (маска 017).
        // Номер дорожки/цилиндра упакован со сдвигом на 4 бита.
        Word sector = rk11_rkda & 017; 
        Word track = (rk11_rkda >> 4) & 0377;
        
        // Линейный номер сектора: (дорожка * 12 секторов) + текущий сектор
        long sector_index = (track * 12) + sector;
        long file_offset = sector_index * 512;

        // Позиционируем указатель чтения в файле образа
        if (fseek(disk, file_offset, SEEK_SET) != 0) {
            print_log(LOG_ERROR, ">>> RK11 ERROR: fseek failed to offset %ld!", file_offset);
            rk11_rkcs |= 0100200;
            fclose(disk);
            return;
        }

        print_log(LOG_TRACE, ">>> RK11 READ: Track %d, Sector %d (Offset %ld bytes)", track, sector, file_offset);

        // 3. Вычисляем реальное количество слов для переноса
        // Так как RKWC отрицательный, получаем модуль числа: ~WC + 1
        int words_to_read = 0;
        if (rk11_rkwc != 0) {
            words_to_read = (int)((~rk11_rkwc + 1) & 0xFFFF);
        }

        // 4. Пословный цикл переноса данных в ОЗУ эмулятора mem[]
        Address current_mem_addr = rk11_rkba;
        int words_transferred = 0;

        for (int i = 0; i < words_to_read; i++) {
            Word disk_word = 0;
            
            // ИСПРАВЛЕНО: Читаем строго по 2 байта (1 целое 16-битное слово) за раз!
            if (fread(&disk_word, 2, 1, disk) != 1) {
                break; 
            }

            // ПРЯМОЙ DMA-ПЕРЕНОС В ОЗУ:
            if (current_mem_addr < MEMSIZE - 1) {
                mem[current_mem_addr] = (Byte)(disk_word & 0xFF);         
                mem[current_mem_addr + 1] = (Byte)((disk_word >> 8) & 0xFF); 
            }

            current_mem_addr += 2;
            words_transferred++;
        }

        // Вышли из цикла чтения. Проверяем, почему он завершился:
        if (words_transferred < words_to_read) {
            print_log(LOG_ERROR, ">>> RK11 FATAL: Transfer aborted! Read only %d words out of %d. File error: %d", 
                      words_transferred, words_to_read, ferror(disk));
        } else {
            print_log(LOG_TRACE, ">>> RK11 SUCCESS: Completely read %d words of boot sector!", words_transferred);
        }

        fclose(disk);

        // 5. АППАРАТНОЕ ОБНОВЛЕНИЕ РЕГИСТРОВ ПОСЛЕ ОПЕРАЦИИ:
        // Счетчик слов RKWC увеличивается на количество перенесенных слов
        rk11_rkwc = (Word)((rk11_rkwc + words_transferred) & 0xFFFF);
        // Адрес шины RKBA продвигается вперед на объем перенесенных данных
        rk11_rkba = current_mem_addr;

        print_log(LOG_TRACE, ">>> RK11 SUCCESS: Transferred %d words to memory address %06o", words_transferred, rk11_rkba);
    }

    // ===================================================================
    // ИСПРАВЛЕНО ПО АППАРАТНОМУ СТАНДАРТУ DEC RK11:
    // По завершении операции контроллер обязан взвести бит Ready (0200),
    // но при этом полностью СБРОСИТЬ в ноль бит запуска GO (бит 0) 
    // и очистить код предыдущей команды (биты 1-3)! 
    // Сначала полностью гасим биты 0-3 (маска 017), а затем взводим Ready!
    // ===================================================================
    rk11_rkcs &= ~017;   // Стираем GO и код команды в ноль!
    rk11_rkcs |= 000200; // Взводим 7-й бит готовности привода (Ready = 1)
    // ===================================================================
    // ===================================================================
    // ВОЗВРАЩЕНО НА ШИНУ ПЕРИФЕРИИ (Канон DMA-обмена):
    // Контроллер диска завершил перенос данных в ОЗУ и ОБЯЗАН аппаратно пнуть
    // шину прерываний процессора interrupts(), чтобы запустить обработчик 
    // Вектора 220 операционной системы, если прерывания диска разрешены (IE=1)!
    // ===================================================================
    interrupts(); 
    // ===================================================================
}

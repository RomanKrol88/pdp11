#ifndef LOGGER_H
#define LOGGER_H

typedef enum {  //уровни логирования для отладки
    LOG_OUTPUT = 0, //stdout эмулятора
    LOG_OFF,        //логи полностью отключены
    LOG_ERROR,      //только критические ошибки
    LOG_INFO,       //общая информация о работе
    LOG_TRACE,      //пошаговая трассировка
    LOG_DEBUG       //глубокая отладка
} Log_level;

extern Log_level current_log_level;

int set_log_level(Log_level level);                           //функция установки порогового уровня логирования
void print_log(Log_level level, const char* format, ...);     //функция для обработки и вывода логов

#endif
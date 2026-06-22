#ifndef LOGGER_H
#define LOGGER_H

typedef enum {  //уровни логирования для отладки
    LOG_OFF,    //логи полностью отключены
    LOG_ERROR,  //только критические ошибки
    LOG_INFO,   //общая информация о работе
    LOG_TRACE,  //пошаговая трассировка
    LOG_PURE,   //отладочная печать без префиксов
    LOG_DEBUG   //глубокая отладка
} Log_level;

int set_log_level(int level);                           //функция установки порогового уровня логирования
void print_log(int level, const char* format, ...);     //функция для обработки и вывода логов

#endif
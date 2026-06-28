#include "logger.h"
#include <stdio.h>
#include <stdarg.h>

Log_level current_log_level = LOG_OFF;           //по умолчанию логи отключены
static const char* level_strings[] = {"PDP11 OUTPUT", "LOG_OFF", "LOG_ERROR", "LOG_INFO", "LOG_TRACE", "LOG_DEBUG"}; //наглядное оформление вывода логов

int set_log_level(Log_level level) {
    Log_level old_level = current_log_level;
    current_log_level = (Log_level)level;
    return (int)old_level;
}

void print_log(Log_level level, const char* format, ...) {
    if (level < LOG_OUTPUT || level > LOG_DEBUG || level > current_log_level || current_log_level == LOG_OUTPUT) {
        return;
    }

    printf("[%s] ", level_strings[level]);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    if (level != LOG_OUTPUT || current_log_level == LOG_TRACE || current_log_level == LOG_DEBUG) {
        printf("\n");
    }
    fflush(stdout);
}
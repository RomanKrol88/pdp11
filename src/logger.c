#include "logger.h"
#include <stdio.h>
#include <stdarg.h>

static Log_level current_log_level = LOG_OFF;           //по умолчанию логи отключены
static const char* level_strings[] = {"LOG_OFF", "LOG_ERROR", "LOG_INFO", "LOG_TRACE", "LOG_PURE", "LOG_DEBUG"}; //наглядное оформление вывода логов

int set_log_level(int level) {
    int old_level = (int)current_log_level;
    current_log_level = (Log_level)level;
    return old_level;
}

void print_log(int level, const char* format, ...) {
    if (level < LOG_OFF || level > LOG_DEBUG || level > (int)current_log_level || current_log_level == LOG_OFF) {
        return;
    }

    if (level != LOG_PURE) {
        printf("[%s] ", level_strings[level]);
    }
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
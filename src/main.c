#include "memory.h"
#include "logger.h"
#include "cpu.h"
#include "tests.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int main (int argc, char * argv[])  {
    const char * filename = (argc > 1) ? argv[1] : "test/data.txt";     //по-умолчанию используем файл data.txt

    //ключи для выставления уровня логирования
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-logoff") == 0 || strcmp(argv[i], "-quiet") == 0) {
            set_log_level(LOG_OFF);
        }
        if (strcmp(argv[i], "-error") == 0) {
            set_log_level(LOG_ERROR);
        }
        if (strcmp(argv[i], "-info") == 0) {
            set_log_level(LOG_INFO);
        }
        if (strcmp(argv[i], "-trace") == 0) {
            set_log_level(LOG_TRACE);
        }
        if (strcmp(argv[i], "-debug") == 0) {
            set_log_level(LOG_DEBUG);
        }
    }

    //запуск программы в режиме тестирования
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--testall") == 0) {
            run_all_tests(); //запуск всех тестов
            return 0;
        }

        if (strcmp(argv[i], "--test") == 0) {
            if (i + 1 < argc) {
                const char *arg = argv[i + 1];
                
                if (isdigit((unsigned char)arg[0])) { 
                    int id = atoi(arg); //перевод строки в ID
                    run_test_by_id(id); //запуск теста по ID
                } else {
                    run_test_by_name(arg); //если ID не распознан, запускаем по имени
                }
                return 0; 
            } else {
                print_log(LOG_ERROR, "Error: The --test option requires a test name or ID argument!");
                return 1;
            }
        }
    }

    print_log(LOG_INFO, "==================================================");
    print_log(LOG_INFO, "     PDP-11 EMULATOR v0.5 (beta) BY ROMAN KROL    ");
    print_log(LOG_INFO, "==================================================");

    //load_data(stdin);

    load_file(filename);

    run();
    
    return 0;
}
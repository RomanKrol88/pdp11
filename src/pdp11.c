#include "memory.h"
#include "logger.h"
#include "cpu.h"
#include "tests.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int main (int argc, char * argv[])  {
  
    set_log_level(LOG_DEBUG);

    int testing_mode = 0;

    //запуск программы в режиме тестирования
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--testall") == 0) {
            run_all_tests(); //запуск всех тестов
            testing_mode = 1;
            break;
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
                print_log(LOG_ERROR, "Error: The --test option requires a test name or ID argument!\n");
                return 1;
            }
        }
    }
    //выполняем тест и выходим из режима тестирования
    if (testing_mode) {
        return 0;
    }

    print_log(LOG_INFO, "==================================================\n");
    print_log(LOG_INFO, "         PDP-11 EMULATOR v1.0 BY ROMAN KROL       \n");
    print_log(LOG_INFO, "==================================================\n");

    const char * filename = (argc > 1) ? argv[1] : "test/data.txt";      //по-умолчанию используем файл data.txt

    //load_data(stdin);

    load_file(filename);

    run();
    
    return 0;
}
#include "memory.h"
#include "logger.h"
#include "cpu.h"

int main (int argc, char * argv[])  {
    const char * filename = (argc > 1) ? argv[1] : "test/data.txt";      //по-умолчанию используем файл data.txt
    
    set_log_level(LOG_DEBUG);

    //test_mem();

    //load_data(stdin);

    load_file(filename);

    //mem_dump(0x40, 20);
    //mem_dump(0x200, 0x26);

    test_parse_mov();
    test_mode0();
    test_mov();
    test_mode1_toreg();
    test_mode1_fromreg();
    test_mode2_reg();
    test_mode2_pc();

    run();
    
    return 0;
}
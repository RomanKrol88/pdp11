#include <stdio.h>

#define N 4 //количество ячеек памяти

typedef unsigned char Data;

Data scan_code_adress();                                                    //функция считывания адреса
Data code_adress_to_reg(Data adress_reg);                                   //функция расшифровки кода адреса
Data func_ADD(Data adress_reg1, Data adress_reg2, Data * reg_array);        //функция ADD сложения чисел
Data func_SUB(Data first_arg, Data second_arg, Data * reg_array);           //функция SUB вычитания чисел
void func_MOV(Data arg, Data value, Data * reg_array);                      //функция MOV запись числа в регистр
void print_IR(Data * reg_array);                                            //функция печати всех чисел в памяти

int main() {
    Data function_code;              //переменная хранит код вызова функции
    Data value;                      //переменная хранит значение, которое хотим записать в память
    Data first_code_adress, second_code_adress, code_adress; //переменные для временного хранения полученного кода адреса регистра 
    Data reg_array[N] = {0};         //массив для записи и хранения значений в регистрах памяти

    while (scanf("%hhu", &function_code) == 1) {
            switch (function_code) {
            case 1 : 
                first_code_adress = scan_code_adress();
                second_code_adress = scan_code_adress();
                func_ADD(first_code_adress, second_code_adress, reg_array);
                break;
            case 2 : 
                first_code_adress = scan_code_adress();
                second_code_adress = scan_code_adress();
                func_SUB(first_code_adress, second_code_adress, reg_array);
                break;
            case 3 : 
                code_adress = scan_code_adress();
                scanf("%hhu", &value);
                func_MOV(code_adress, value, reg_array);
                break;
            case 4 :
                print_IR(reg_array);
                break;
            case 0 :
            default : return 0;
        }
    }
    return 0;
}

Data scan_code_adress() {
    Data adress_reg;
    scanf("%hhu", &adress_reg);
    return adress_reg;
}

Data code_adress_to_reg(Data adress_reg) {
    Data reg_number;

    switch (adress_reg) {
    case 5 :
        reg_number = 0;
        break;
    case 6 :
        reg_number = 1;
        break;
    case 7 :
        reg_number = 2;
        break;
    case 8 :
        reg_number = 3;
        break;
    }

    return reg_number;
}

Data func_ADD(Data first_arg, Data second_arg, Data * reg_array) {
    return reg_array[code_adress_to_reg(first_arg)] +=  reg_array[code_adress_to_reg(second_arg)];
}

Data func_SUB(Data first_arg, Data second_arg, Data * reg_array) {
    return reg_array[code_adress_to_reg(first_arg)] -=  reg_array[code_adress_to_reg(second_arg)];
}

void func_MOV(Data arg, Data value, Data * reg_array) {
    reg_array[code_adress_to_reg(arg)] = value;
}

void print_IR(Data * reg_array) {
    for (Data i = 0; i < N; i++) {
        printf("%hhu ", reg_array[i]);
    }
    printf("\n");
}
#include <stdio.h>

#define N 4 //количество ячеек памяти

int scan_code_adress();                                                                         //функция считывания адреса
int code_adress_to_reg();                                                                       //функция расшифровки кода адреса
int func_ADD(int adress_reg1, int adress_reg2, int * reg_array);         //функция ADD сложения чисел
int func_SUB(int first_arg, int second_arg, int * reg_array);         //функция SUB вычитания чисел
void func_MOV(int arg, int value, int * reg_array);                 //функция MOV запись числа в регистр
void print_IR(int * reg_array);                                          //функция печати всех чисел в памяти

int code_adress_to_reg(int adress_reg) {
    int reg_number;

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

int scan_code_adress() {
    int adress_reg;
    scanf("%d ", &adress_reg);
    return adress_reg;
}

int main() {
    int function_code;              //переменная хранит код вызова функции
    int value;
    int first_code_adress, second_code_adress, code_adress;
    int reg_array[N] = {0};

    while (scanf("%d", &function_code) == 1) {
    
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
                scanf("%d", &value);
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

int func_ADD(int first_arg, int second_arg, int * reg_array) {
    return reg_array[code_adress_to_reg(first_arg)] +=  reg_array[code_adress_to_reg(second_arg)];
}

int func_SUB(int first_arg, int second_arg, int * reg_array) {
    return reg_array[code_adress_to_reg(first_arg)] -=  reg_array[code_adress_to_reg(second_arg)];
}


void func_MOV(int arg, int value, int * reg_array) {
    reg_array[code_adress_to_reg(arg)] = value;
}

void print_IR(int * reg_array) {
    for (int i = 0; i < N; i++) {
        printf("%d ", reg_array[i]);
    }
    printf("\n");
}
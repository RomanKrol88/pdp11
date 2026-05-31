#include <stdio.h>

int func_ADD(int * pa, int * pb, int * pc, int * pd, int adress_reg1, int adress_reg2);         //функция ADD сложения чисел
int func_SUB(int * pa, int * pb, int * pc, int * pd, int adress_reg1, int adress_reg2);         //функция SUB вычитания чисел
void func_MOV(int * pa, int * pb, int * pc, int * pd, int adress_reg, int num);                 //функция MOV запись числа в регистр
void print_IR(int reg1, int reg2, int reg3, int reg4);                                          //функция печати всех чисел в памяти


int main() {
    int code, adress_reg1, adress_reg2, num;
    int a = 0, b = 0, c = 0, d = 0;
    int *pa = &a; int *pb = &b; int *pc = &c; int *pd = &d;

    while (scanf("%d", &code) == 1) {
        switch (code) {
            case 1 : 
                scanf("%d %d", &adress_reg1, &adress_reg2);
                func_ADD(adress_reg1, adress_reg2);
                break;
            case 2 : 
                scanf("%d %d", &adress_reg1, &adress_reg2);
                break;
            case 3 : 
                scanf("%d %d", &adress_reg1, &num);
                func_MOV(pa, pb, pc, pd, adress_reg1, num);
                break;
            case 4 :
                print_IR(a, b, c, d);
                break;
            case 0 :
            default : return 0;
        }
    }
    
    return 0;
}

int func_ADD(int * pa, int * pb, int * pc, int * pd, int adress_reg1, int adress_reg2) {
    int reg1, reg2;
    switch (adress_reg1) {
    case 5 :
        reg1 = *pa;
        break;
    case 6 :
        reg1 = *pb;
        break;
    case 7 :
        reg1 = *pc;
        break;
    case 8 :
        reg1 = *pd;
        break;
    }

    switch (adress_reg2) {
    case 5 :
        reg2 = *pa;
        break;
    case 6 :
        reg2 = *pa;
        break;
    case 7 :
        reg2 = *pa;
        break;
    case 8 :
        reg2 = *pa;
        break;
    }


    return reg1 + reg2;
}

int func_SUB(int * pa, int * pb, int * pc, int * pd, int adress_reg1, int adress_reg2);


void func_MOV(int * pa, int * pb, int * pc, int * pd, int adress_reg, int num) {
    switch (adress_reg) {
    case 5 :
        *pa = num;
        break;
    case 6 :
        *pb = num;
        break;
    case 7 :
        *pc = num;
        break;
    case 8 :
        *pd = num;
        break;
    }
}

void print_IR(int reg1, int reg2, int reg3, int reg4) {
    printf("%d %d %d %d\n", reg1, reg2, reg3, reg4);
}
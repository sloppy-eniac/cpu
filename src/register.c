#include "register.h" 
#include <stdlib.h>   
#include <string.h>

CPU_Registers* init_registers() {
    CPU_Registers* regs = (CPU_Registers*)malloc(sizeof(CPU_Registers));
    if (regs==NULL) return NULL;
    //할당된 메모리를 0으로 초기화
    memset(regs, 0, sizeof(CPU_Registers));
    return regs;
}

void free_registers(CPU_Registers* regs) {
    if (regs!=NULL) free(regs);
}
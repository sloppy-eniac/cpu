#include "register.h"
#include <stdint.h>

/*
 * @brief 레지스터에 값을 설정합니다
 * @param regs 대상 레지스터 구조체 포인터
 * @param reg_num 설정할 레지스터 번호 (0~7)
 * @param value 설정할 값 (8비트)
 * @returns 없음 (void)
 */
void set_register(CPU_Registers* regs, uint8_t reg_num, uint8_t value) {
    if (!regs) return;
    
    switch (reg_num) {
        case 0: regs->pc = value; break;
        case 1: regs->register1 = value; break;
        case 2: regs->register2 = value; break;
        case 3: regs->register3 = value; break;
        case 4: regs->register4 = value; break;
        case 5: regs->register5 = value; break;
        case 6: regs->register6 = value; break;
        case 7: regs->register7 = value; break;
    }
}

/*
 * @brief 레지스터에서 값을 읽어옵니다
 * @param regs 레지스터 구조체 포인터
 * @param reg_num 읽을 레지스터 번호 (0~7)
 * @returns 레지스터 값 (8비트), 잘못된 입력 시 0 반환
 */
uint8_t get_register(const CPU_Registers* regs, uint8_t reg_num) {
    if (!regs) return 0;
    
    switch (reg_num) {
        case 0: return regs->pc;
        case 1: return regs->register1;
        case 2: return regs->register2;
        case 3: return regs->register3;
        case 4: return regs->register4;
        case 5: return regs->register5;
        case 6: return regs->register6;
        case 7: return regs->register7;
        default: return 0;
    }
}

/*
 * @brief 모든 레지스터를 0으로 초기화합니다
 * @param regs 초기화할 레지스터 구조체 포인터
 * @returns 없음 (void)
 */
void reset_registers(CPU_Registers* regs) {
    if (!regs) return;
    regs->pc = 0;
    regs->register1 = 0;
    regs->register2 = 0;
    regs->register3 = 0;
    regs->register4 = 0;
    regs->register5 = 0;
    regs->register6 = 0;
    regs->register7 = 0;
}

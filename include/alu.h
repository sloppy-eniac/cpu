/* core/instruction.c - ALU 명령어
 * ------------------------------------------------------------
 * fetch된 명령어에 따라 실행할 함수들 정의
 * Test Case: tests/cpu_cycle_test.c
 * Author: 이태겸
 */

#ifndef CPU_ALU_H
#define CPU_ALU_H

#include <stdint.h>

uint8_t add(uint8_t a, uint8_t b);
uint8_t subtraction(uint8_t a, uint8_t b);
uint8_t multiply(uint8_t a, uint8_t b);
uint8_t divide(uint8_t a, uint8_t b);

#endif // CPU_ALU_H
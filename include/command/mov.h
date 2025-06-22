/* include/mov.h - MOV 명령어 실행 함수 인터페이스 정의
 * ------------------------------------------------------------
 * MOV 명령어의 다양한 형태(예: reg,imm)를 실행하는 함수들의 프로토타입을 선언합니다.
 * 이 함수들은 CPU 코어(cpu.c)에서 디코딩된 명령어에 따라 호출됩니다.
 * Test Case: tests/mov_test.c
 * Author: Shin yujun
*/

#ifndef MOV_H
#define MOV_H

#include <stdint.h>
#include "register.h"

void mov_eax_imm(CPU_Registers* regs, uint32_t value);
// value를 EAX 레지스터에 저장합니다. 그리고 이동시킨 값을 반환합니다.

#endif


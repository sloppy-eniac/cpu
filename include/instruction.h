/* core/instruction.c - ALU 명령어 처리 핸들러 테이블 초기화
 * ------------------------------------------------------------
 * fetch된 명령어를 opcode 기준으로 분기 처리하고, 대응하는 실행 로직을 호출함
 * Test Case: tests/cpu_cycle_test.c
 * Author: 이태겸
 * 하고싶은말: 내 코드지만 정말 아름답다
 */

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>

typedef uint8_t (*op_handler)(uint8_t, uint8_t);

void init_handler_table();

#endif // INSTRUCTION_H

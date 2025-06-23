#include <instruction.h>
#include <stdint.h>
#include "alu.h"

// ALU 핸들러 테이블 전역 변수
op_handler handler_table[4];

/*
 * @brief ALU 핸들러 테이블을 초기화합니다
 * @param 없음
 * @returns 없음 (void)
 *
 * @details
 * handler_table 배열에 각 opcode에 해당하는 연산 함수들을 할당합니다.
 * - 0x00: 덧셈 연산 (add)
 * - 0x01: 뺄셈 연산 (subtraction)
 * - 0x02: 곱셈 연산 (multiply)
 * - 0x03: 나눗셈 연산 (divide)
 *
 * @example
 * init_handler_table();  // 테이블 초기화
 * result = handler_table[0x00](3, 2);  // 3 + 2 = 5
 * result = handler_table[0x01](7, 4);  // 7 - 4 = 3
 */
void init_handler_table()
{
    handler_table[0x00] = add;
    handler_table[0x01] = subtraction;
    handler_table[0x02] = multiply;
    handler_table[0x03] = divide;
}

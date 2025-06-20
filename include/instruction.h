/* core/instruction.c - 명령어 디코딩 및 실행 처리
 * ------------------------------------------------------------
 * fetch된 명령어를 opcode 기준으로 분기 처리하고, 대응하는 실행 로직을 호출함
 * Test Case: tests/cpu_cycle_test.c 신유준 코드 나오면 적음.
 * Author: 이태겸
 * 하고싶은말: 내 코드지만 정말 아름답다
*/


typedef uint8_t (*op_handler)(uint8_t, uint8_t);

uint8_t add(uint8_t a, uint8_t b);
uint8_t subtraction(uint8_t a, uint8_t b);
uint8_t multiply(uint8_t a, uint8_t b);
uint8_t divide(uint8_t a, uint8_t b);

void init_handler_table();



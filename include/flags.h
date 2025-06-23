#ifndef CPU_FLAG_H
#define CPU_FLAG_H

#include <stdbool.h> // bool 타입을 사용하기 위해 포함
#include <stdint.h>  // uint8_t 같은 고정 크기 정수 타입을 위해 포함

// 전방 선언
struct CPU_Registers;

/*
 * @brief 덧셈 연산에서 캐리 플래그를 설정합니다
 * @param regs CPU 레지스터 구조체 포인터
 * @param a 첫 번째 피연산자
 * @param result 덧셈 결과
 * @returns 없음 (void)
 */
void add_flag(struct CPU_Registers* regs, uint8_t a, uint8_t result);

/*
 * @brief 뺄셈 연산에서 캐리 플래그를 설정합니다
 * @param regs CPU 레지스터 구조체 포인터
 * @param a 피감수 (첫 번째 피연산자)
 * @param b 감수 (두 번째 피연산자)
 * @returns 없음 (void)
 */
void subtraction_flag(struct CPU_Registers* regs, uint8_t a, uint8_t b);

#endif

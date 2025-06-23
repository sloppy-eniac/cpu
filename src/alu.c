#include "include/alu.h"

/*
 * @brief 두 개의 8비트 값을 더합니다
 * @param a 첫 번째 피연산자
 * @param b 두 번째 피연산자
 * @returns 덧셈 결과 (8비트)
 */
uint8_t add(uint8_t a, uint8_t b)
{
    uint8_t result = a + b;
    // add_flag(a, result); // 임시로 주석 처리
    return result;
}

/*
 * @brief 첫 번째 값에서 두 번째 값을 뺍니다
 * @param a 피감수 (첫 번째 피연산자)
 * @param b 감수 (두 번째 피연산자)
 * @returns 뺄셈 결과 (8비트)
 */
uint8_t subtraction(uint8_t a, uint8_t b)
{
    uint8_t result = a - b;
    // subtraction_flag(a, b); // 임시로 주석 처리
    return result;
}

/*
 * @brief 두 개의 8비트 값을 곱합니다
 * @param a 첫 번째 피연산자
 * @param b 두 번째 피연산자
 * @returns 곱셈 결과 (8비트)
 */
uint8_t multiply(uint8_t a, uint8_t b) { return a * b; }

/*
 * @brief 첫 번째 값을 두 번째 값으로 나눕니다
 * @param a 피제수 (나누어지는 수)
 * @param b 제수 (나누는 수)
 * @returns 나눗셈 결과 (8비트), 0으로 나누는 경우 0 반환
 */
uint8_t divide(uint8_t a, uint8_t b) { return b != 0 ? a / b : 0; }

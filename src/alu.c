#include "include/alu.h"
#include "include/flags.h"
#include "include/register.h"
#include <stdio.h>

// 전역 레지스터에 대한 외부 참조
extern CPU_Registers regs;

/*
 * @brief 두 개의 8비트 값을 더합니다
 * @param a 첫 번째 피연산자
 * @param b 두 번째 피연산자
 * @returns 덧셈 결과 (8비트)
 */
uint8_t add(uint8_t a, uint8_t b)
{
    uint16_t full_result = (uint16_t)a + (uint16_t)b;
    uint8_t result = (uint8_t)full_result;

    // signed 오버플로우: 같은 부호끼리 더했는데 결과 부호가 바뀌면 오버플로우
    bool sign_a = (a & 0x80) != 0;
    bool sign_b = (b & 0x80) != 0;
    bool sign_r = (result & 0x80) != 0;
    bool overflow_occurred = (!sign_a && !sign_b && sign_r) || (sign_a && sign_b && !sign_r);
    set_overflow_flag(&regs, overflow_occurred);

    if (overflow_occurred) {
        printf("🚨 덧셈 오버플로우: %d + %d = %d (OF=1)\n", (int8_t)a, (int8_t)b, (int8_t)result);
    } else {
        printf("✅ 덧셈 정상: %d + %d = %d (OF=0)\n", (int8_t)a, (int8_t)b, (int8_t)result);
    }

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

    // signed 오버플로우: 다른 부호끼리 뺐는데 결과 부호가 바뀌면 오버플로우
    bool sign_a = (a & 0x80) != 0;
    bool sign_b = (b & 0x80) != 0;
    bool sign_r = (result & 0x80) != 0;
    bool overflow_occurred = (!sign_a && sign_b && sign_r) || (sign_a && !sign_b && !sign_r);
    set_overflow_flag(&regs, overflow_occurred);

    if (overflow_occurred) {
        printf("🚨 뺄셈 오버플로우: %d - %d = %d (OF=1)\n", (int8_t)a, (int8_t)b, (int8_t)result);
    } else {
        printf("✅ 뺄셈 정상: %d - %d = %d (OF=0)\n", (int8_t)a, (int8_t)b, (int8_t)result);
    }

    return result;
}

/*
 * @brief 두 개의 8비트 값을 곱합니다
 * @param a 첫 번째 피연산자
 * @param b 두 번째 피연산자
 * @returns 곱셈 결과 (8비트)
 */
uint8_t multiply(uint8_t a, uint8_t b) {
    int16_t signed_result = (int8_t)a * (int8_t)b;
    uint8_t result = (uint8_t)signed_result;

    // signed 오버플로우: 결과가 -128~127 범위를 벗어나면
    bool overflow_occurred = (signed_result < -128 || signed_result > 127);
    set_overflow_flag(&regs, overflow_occurred);

    if (overflow_occurred) {
        printf("🚨 곱셈 오버플로우: %d * %d = %d (실제: %d, OF=1)\n",
               (int8_t)a, (int8_t)b, (int8_t)result, signed_result);
    } else {
        printf("✅ 곱셈 정상: %d * %d = %d (OF=0)\n", (int8_t)a, (int8_t)b, (int8_t)result);
    }

    return result;
}

/*
 * @brief 첫 번째 값을 두 번째 값으로 나눕니다
 * @param a 피제수 (나누어지는 수)
 * @param b 제수 (나누는 수)
 * @returns 나눗셈 결과 (8비트), 0으로 나누는 경우 0 반환
 */
uint8_t divide(uint8_t a, uint8_t b) {
    if (b == 0) {
        set_overflow_flag(&regs, true);
        printf("🚨 0으로 나누기 에러: %d ÷ 0 (OF=1)\n", (int8_t)a);
        return 0;
    }

    int8_t signed_a = (int8_t)a;
    int8_t signed_b = (int8_t)b;
    int8_t signed_result = signed_a / signed_b;
    uint8_t result = (uint8_t)signed_result;

    // -128 / -1 = 128 (8비트 범위 초과)
    bool overflow_occurred = (signed_a == -128 && signed_b == -1);
    set_overflow_flag(&regs, overflow_occurred);

    if (overflow_occurred) {
        printf("🚨 나눗셈 오버플로우: %d ÷ %d (OF=1)\n", signed_a, signed_b);
    } else {
        printf("✅ 나눗셈 정상: %d ÷ %d = %d (OF=0)\n", signed_a, signed_b, signed_result);
    }

    return result;
}

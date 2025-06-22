#include "include/alu.h"

uint8_t add(uint8_t a, uint8_t b)
{
    uint8_t result = a + b;
    // add_flag(a, result); // 임시로 주석 처리
    return result;
}

uint8_t subtraction(uint8_t a, uint8_t b)
{
    uint8_t result = a - b;
    // subtraction_flag(a, b); // 임시로 주석 처리
    return result;
}

uint8_t multiply(uint8_t a, uint8_t b) { return a * b; }
uint8_t divide(uint8_t a, uint8_t b) { return b != 0 ? a / b : 0; }

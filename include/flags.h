#ifndef CPU_FLAG_H
#define CPU_FLAG_H

#include <stdbool.h> // bool 타입을 사용하기 위해 포함
#include <stdint.h>  // uint8_t 같은 고정 크기 정수 타입을 위해 포함

bool CF = false;
bool *cf = &CF;
/**
 * @brief // Carry Flag (CF) - 덧셈 시 자리올림 발생 혹은 뺄셈 시 자리빌림 발생 시 true가 됨
 *
 * @param a 연산될때 사용하는 첫번째 값
 * @param result a와b의 연산결과
 *
 * @details
 * 이 함수는 두 피연산자와 결과의 최상위 비트(MSB)를 검사하여
 * 부호 플래그를 설정한다. 포인터를 사용해서 원본 플래그를 직접 수정.
 *
 * 포인터를 사용하는 이유는 함수 내부에서 플래그 값을 직접 변경하여
 * 호출한 곳에 그 결과가 바로 반영되도록 하기 위함이다.
 */
void add_flag(uint8_t a, uint8_t result);
/**
 * @brief // Carry Flag (CF) - 덧셈 시 자리올림 발생 혹은 뺄셈 시 자리빌림 발생 시 true가 됨
 *
 * @param a 연산될때 사용하는 첫번째 값
 * @param b 연산될때 사용하는 두번째 값
 *
 * @details
 * 이 함수는 두 피연산자와 결과의 최상위 비트(MSB)를 검사하여
 * 부호 플래그를 설정한다. 포인터를 사용해서 원본 플래그를 직접 수정.
 *
 * 포인터를 사용하는 이유는 함수 내부에서 플래그 값을 직접 변경하여
 * 호출한 곳에 그 결과가 바로 반영되도록 하기 위함이다.
 */
void subtraction_flag(uint8_t a, uint8_t result);
#endif

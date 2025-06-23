#include "include/flags.h"
#include "include/register.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * @brief 덧셈 연산에서 캐리 플래그를 설정합니다
 * @param regs CPU 레지스터 구조체 포인터
 * @param a 첫 번째 피연산자
 * @param result 덧셈 결과
 * @returns 없음 (void)
 */
void add_flag(CPU_Registers* regs, uint8_t a, uint8_t result)
{
    if (!regs) return;
    // 오버플로우 검사: 결과가 첫 번째 피연산자보다 작으면 캐리 발생
    bool carry = (result < a);
    set_carry_flag(regs, carry);
}

/*
 * @brief 뺄셈 연산에서 캐리 플래그를 설정합니다
 * @param regs CPU 레지스터 구조체 포인터
 * @param a 피감수 (첫 번째 피연산자)
 * @param b 감수 (두 번째 피연산자)
 * @returns 없음 (void)
 */
void subtraction_flag(CPU_Registers* regs, uint8_t a, uint8_t b)
{
    if (!regs) return;
    // 언더플로우 검사: 첫 번째 피연산자가 두 번째보다 작으면 캐리 발생
    bool carry = (a < b);
    set_carry_flag(regs, carry);
}

// 이제 결과 보여줄때 캐리값 같이 보여주면 되는거임!!
// 더한후 결과는 255더한값으로 보여주면 되는거고
// 뺀후에는 걍 에러 띄워야지 뭐. 음수표현 안됨 우리
/* include/register.h - CPU Register 인터페이스 정의
 * ------------------------------------------------------------
 * CPU의 데이터 저장/참조 레지스터(GPR, Segment)의 구조를 정의합니다.
 * EIP와 EFLAGS는 CPU의 제어 상태이므로 별도의 CPU 상태 구조체에서 관리합니다.
 * Test Case: tests/register_test.c
 * Author: Shin yujun
*/

#ifndef REGISTER_H
#define REGISTER_H

#include <stdint.h>

// 범용 레지스터
// 다음 리스트는 그저 권장사항일 뿐이지, 사용하는 방법은 상관없다.
typedef struct {
    uint16_t eax; // 누산기: 산술 연산, 데이터 저장
    uint16_t ebx; // 베이스: 메모리 주소 지정 시 베이스 주소로 사용
    uint16_t ecx; // 카운터: 반복(LOOP) 명령어 등에서 카운터로 사용
    uint16_t edx; // 데이터: 곱셈/나눗셈에서 EAX 보조, I/O 주소 지정
    uint16_t esi; // 소스 인덱스: 문자열/배열 복사 시 소스 주소
    uint16_t edi; // 목적지 인덱스: 문자열/배열 복사 시 목적지 주소
    uint16_t ebp; // 베이스 포인터: 스택 프레임의 시작 주소를 가리킴 (지역 변수 접근)
    uint16_t esp; // 스택 포인터: 스택의 가장 윗부분을 가리킴 (PUSH, POP)
} GPRs;

typedef struct {
    GPRs gpr;
} CPU_Registers;

#endif
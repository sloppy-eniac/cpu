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
typedef struct {
    uint32_t eax; // 누산기: 산술 연산, 데이터 저장
    uint32_t ebx; // 베이스: 메모리 주소 지정 시 베이스 주소로 사용
    uint32_t ecx; // 카운터: 반복(LOOP) 명령어 등에서 카운터로 사용
    uint32_t edx; // 데이터: 곱셈/나눗셈에서 EAX 보조, I/O 주소 지정
    uint32_t esi; // 소스 인덱스: 문자열/배열 복사 시 소스 주소
    uint32_t edi; // 목적지 인덱스: 문자열/배열 복사 시 목적지 주소
    uint32_t ebp; // 베이스 포인터: 스택 프레임의 시작 주소를 가리킴 (지역 변수 접근)
    uint32_t esp; // 스택 포인터: 스택의 가장 윗부분을 가리킴 (PUSH, POP)
} GPRs;

// 세그먼트 레지스터
typedef struct {
    uint16_t cs; // 코드 세그먼트: 실행할 코드가 있는 세그먼트
    uint16_t ds; // 데이터 세그먼트: 데이터가 저장된 세그먼트
    uint16_t ss; // 스택 세그먼트: 스택이 위치한 세그먼트
    uint16_t es; // 추가 세그먼트
    uint16_t fs;
    uint16_t gs;
} SegmentRegisters;

typedef struct {
    GPRs gpr;
    SegmentRegisters seg;
} CPU_Registers;

#endif
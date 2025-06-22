/* include/decoder.h - 기계어 디코더 인터페이스 정의
 * ------------------------------------------------------------
 * 메모리로부터 읽어온 바이트 코드를 CPU가 실행할 수 있는 구조체로 변환합니다.
 * 이 헤더는 디코더와 CPU 코어 사이의 핵심 소통 수단 역할을 합니다.
 * Test Case: tests/decoder_test.c
 * Author: Shin yujun
*/

#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>

// CPU가 지원하는 명령어의 종류를 나타내는 열거형
// 디코더는 이 타입으로 명령어의 종류를 구분하여 반환합니다.
typedef enum {
    MOV_EAX_IMM
} OpcodeType;

// 레지스터를 인덱스로 구분하여 사용하기 위한 열거형
// 디코더는 Opcode를 분석하여 어떤 레지스터를 사용할지 이 타입으로 알려줍니다.
typedef enum {
    REG_EAX, // 0
    REG_ECX, // 1
    REG_EDX, // 2
    REG_EBX, // 3
    REG_ESP, // 4
    REG_EBP, // 5
    REG_ESI, // 6
    REG_EDI  // 7
} Reg;

//최종 반환 구조체
typedef struct {
    OpcodeType type;     // 실행할 명령어의 종류
    
    Reg dest_reg;        // 목적지 레지스터
    Reg src_reg;         // 소스 레지스터 (두 개의 레지스터를 사용하는 명령어의 경우)

    uint32_t immediate;  // 명령어에 포함된 32비트 상수 값
} Instruction;

/*
 * @brief 프로그램 카운터(pc)가 가리키는 메모리 주소에서 명령어를 해석합니다.
 * @param uint32_t pc 실행할 명령어의 메모리 주소
 * @param Instruction* instr 디코딩된 결과가 채워질 Instruction 구조체의 포인터
 * @returns int 해석된 명령어의 총 길이(bytes). CPU 코어는 이 값만큼 EIP를 증가시켜야 합니다.
 */
int decode(uint32_t pc, Instruction* instr);

#endif

//예시
//printf("테스트 2: MOV EAX, 0x1234");
//pc += len; // EIP를 다음 명령어 위치로 이동
//Instruction instr2;
//int len2 = decode(pc, &instr2);
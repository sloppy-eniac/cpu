/* include/decoder.h - 명령어 디코딩/인코딩 헤더
 * ------------------------------------------------------------
 * 바이너리 명령어와 구조체 간 변환 기능 제공
 * - decode(): 바이너리 → 구조체
 * - encode(): 구조체 → 바이너리
 * Test Case: tests/decoder_test.c
 * Author: Cascade
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

/* MOV EAX, imm32 인코딩 */
int encode_mov_reg_imm(Reg reg, uint32_t imm, uint8_t* buffer) {
    // MOV r32, imm32 포맷: 1011 1rrr (rrr = 레지스터 번호)
    buffer[0] = 0xB8 | (reg & 0x07);  // 0xB8 + 레지스터 번호
    encode_imm32(imm, buffer + 1);     // 32비트 상수 (리틀 엔디안)
    return 5;  // opcode(1) + imm32(4)
}

int encode_instruction(const Instruction* instr, uint8_t* buffer) {
    switch (instr->type) {
        case OP_MOV_REG_IMM:
            return encode_mov_reg_imm(instr->dest_reg, instr->immediate, buffer);
        default:
            return -1;  // 지원하지 않는 명령어
    }
}
#endif


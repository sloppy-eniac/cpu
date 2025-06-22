//레지스터에 저장--> alu연산-->pc값증가-->메모리에 저장

#include "include/register.h"
#include "include/memory.h"
#include "include/alu.h"
#include <stdint.h>

extern CPU_Registers regs;
extern Memory memory;


void cpu() {
    init_memory(&memory);                        // 메모리 초기화
    memcpy(Memory, buffer, sizeof(MEMORY_SIZE)); // 버퍼 → 메모리 로 복사
    reset_registers(&regs); //레지스터 초기화
    regs.pc = 0; //pc처음 주소를 0으로 설정
    init_handler_table();// 산수 핸들러 초기화

    // --- 아래부터 명령어 fetch~연산~저장~초기화 로직 ---
    // 1. 메모리에서 2바이트(16비트) 명령어 fetch (pc가 가리키는 위치)
    uint16_t inst = (memory_read(&memory, regs.pc) << 8) | memory_read(&memory, regs.pc + 1);

    // 2. 비트 분리: 4비트 opcode, 6비트 reg1, 6비트 reg2
    uint8_t opcode = (inst >> 12) & 0xF;
    uint8_t reg1_val = (inst >> 6) & 0x3F; // 6비트
    uint8_t reg2_val = inst & 0x3F;       // 6비트

    // 3. 레지스터에 값 저장 (reg1, reg2)
    regs.register1 = reg1_val;
    regs.register2 = reg2_val;

    // 4. ALU 연산 (opcode가 0~3 범위 내라고 가정)
    if (opcode < 4) {
        regs.register3 = handler_table[opcode](regs.register1, regs.register2);
    }

    // 6. PC값 2 증가 (명령어 2바이트 소모 16비트씩 메모리 띄어다니니까.)
    regs.pc += 2;

    // 7. 결과를 메모리[PC]에 저장
    memory_write(&memory, regs.pc, regs.register3);

    // 8. register3 초기화
    regs.register3 = 0;
}


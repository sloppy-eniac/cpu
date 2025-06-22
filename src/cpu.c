//레지스터에 저장--> alu연산-->pc값증가-->메모리에 저장

#include "include/cpu.h"
#include "include/register.h"
#include "include/memory.h"
#include "include/alu.h"
#include "include/cache.h"
#include "include/instruction.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// 전역 변수들 선언
CPU_Registers regs;
Memory memory;

// ALU 핸들러 테이블 전역 변수 (instruction.c에서 정의됨)
extern alu_handler handler_table[4];

// CPU 상태 변수
static int cpu_initialized = 0;

// CPU 초기화
void cpu_init(void) {
    if (!cpu_initialized) {
        // 메모리 초기화
        init_memory(&memory);
        cache_init(&memory.cache);
        
        // 레지스터 초기화
        reset_registers(&regs);
        regs.pc = 0;
        
        // ALU 핸들러 테이블 초기화
        init_handler_table();
        
        cpu_initialized = 1;
    }
}

// CPU 리셋
void cpu_reset(void) {
    reset_registers(&regs);
    regs.pc = 0;
    init_memory(&memory);
    cache_init(&memory.cache);
}

// 메모리에 프로그램 로드
void cpu_load_program(const uint8_t* program, size_t size) {
    if (size <= MEMORY_SIZE) {
        memcpy(memory.data, program, size);
    }
}

// 명령어 패치
uint16_t fetch_instruction(void) {
    if (regs.pc >= MEMORY_SIZE - 1) {
        return 0; // 메모리 범위 초과
    }
    
    uint16_t inst = (memory_read(&memory, regs.pc) << 8) | memory_read(&memory, regs.pc + 1);
    return inst;
}

// 명령어 디코드 및 실행
void decode_and_execute(uint16_t instruction) {
    // 비트 분리: 4비트 opcode, 6비트 reg1, 6비트 reg2
    uint8_t opcode = (instruction >> 12) & 0xF;
    uint8_t reg1_val = (instruction >> 6) & 0x3F; // 6비트
    uint8_t reg2_val = instruction & 0x3F;       // 6비트

    // ALU 연산 (opcode가 0~3 범위 내일 때) - 누적 계산 방식
    if (opcode < 4) {
        uint8_t operand1, operand2, result;
        
        // 첫 번째 명령어인 경우 (PC가 0일 때) reg1_val과 reg2_val 직접 사용
        if (regs.pc == 0) {
            operand1 = reg1_val;
            operand2 = reg2_val;
        } else {
            // 누적 계산: register1의 현재 값과 reg1_val을 연산
            // 단일 피연산자가 있으면 reg1_val을, 없으면 reg2_val을 사용
            operand1 = regs.register1;
            operand2 = (reg2_val == 0) ? reg1_val : reg2_val;
        }
        
        result = handler_table[opcode](operand1, operand2);
        
        // 결과를 register1에 저장 (누적)
        regs.register1 = result;
        regs.register2 = operand2;
        regs.register3 = result;
        
        printf("연산: %s %d, %d = %d (register1 누적)\n", 
               (opcode == 0) ? "ADD" : (opcode == 1) ? "SUB" : (opcode == 2) ? "MUL" : "DIV",
               operand1, operand2, result);
        
    } else if (opcode == 4) {
        // MOV 명령어: reg1_val을 메모리 주소 reg2_val에 직접 저장 (캐시 우회)
        if (reg2_val < MEMORY_SIZE) {
            // 캐시를 거치지 않고 직접 메모리에 저장
            memory.data[reg2_val] = reg1_val;
            printf("MOV: 값 %d를 메모리 주소 %d에 직접 저장\n", reg1_val, reg2_val);
        }
        regs.register1 = reg1_val;
        regs.register2 = reg2_val;
    }

    // PC값 2 증가 (명령어 2바이트 소모)
    regs.pc += 2;
}

// 한 단계 실행
void cpu_step(void) {
    if (regs.pc >= MEMORY_SIZE - 1) {
        return; // 프로그램 종료
    }
    
    uint16_t instruction = fetch_instruction();
    if (instruction != 0) {
        decode_and_execute(instruction);
    }
}

// 연속 실행 (기존 함수)
void cpu_run(void) {
    // 프로그램 실행 루프
    for (int i = 0; i < 4 && regs.pc < MEMORY_SIZE - 1; i++) {
        cpu_step();
    }
}

// 샘플 프로그램으로 CPU 실행 (기존 함수 유지)
void cpu(void) {
    // 샘플 프로그램 버퍼
    uint8_t sample_program[] = {
        0x01, 0x05,  // ADD: opcode=0, reg1=1, reg2=5
        0x23, 0x07,  // SUB: opcode=2, reg1=3, reg2=7
        0x14, 0x02,  // ADD: opcode=1, reg1=4, reg2=2
        0x30, 0x01   // MUL: opcode=3, reg1=0, reg2=1
    };
    
    cpu_init();
    cpu_load_program(sample_program, sizeof(sample_program));
    cpu_run();
}

// CPU 상태를 출력하는 함수 (디버깅용)
void print_cpu_state() {
    printf("PC: %d\n", regs.pc);
    printf("Register1: %d\n", regs.register1);
    printf("Register2: %d\n", regs.register2);
    printf("Register3: %d\n", regs.register3);
}

// CPU 레지스터 포인터 반환
CPU_Registers* get_cpu_registers(void) {
    return &regs;
}

// CPU 메모리 포인터 반환
Memory* get_cpu_memory(void) {
    return &memory;
}


#ifndef CPU_H
#define CPU_H

#include "register.h"
#include "memory.h"
#include "alu.h"
#include "cache.h"
#include <stdint.h>

// 전역 CPU 상태
extern CPU_Registers regs;
extern Memory memory;

// CPU 초기화 및 실행 함수들
void cpu_init(void);
void cpu_reset(void);
void cpu_step(void);
void cpu_run(void);
void cpu_load_program(const uint8_t* program, size_t size);

// CPU 상태 정보 함수들
void print_cpu_state(void);
CPU_Registers* get_cpu_registers(void);
Memory* get_cpu_memory(void);

// 명령어 처리 함수들
uint16_t fetch_instruction(void);
void decode_and_execute(uint16_t instruction);

// ALU 핸들러 테이블
typedef uint8_t (*alu_handler)(uint8_t, uint8_t);
extern alu_handler handler_table[4];
void init_handler_table(void);

#endif // CPU_H

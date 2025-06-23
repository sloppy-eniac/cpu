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

/*
 * @brief CPU를 초기화합니다
 * @param 없음
 * @returns 없음 (void)
 */
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

/*
 * @brief CPU를 리셋합니다
 * @param 없음
 * @returns 없음 (void)
 */
void cpu_reset(void) {
    reset_registers(&regs);
    regs.pc = 0;
    init_memory(&memory);
    cache_init(&memory.cache);
}

/*
 * @brief 메모리에 프로그램을 로드합니다
 * @param program 로드할 프로그램 바이트 배열
 * @param size 프로그램 크기 (바이트)
 * @returns 없음 (void)
 */
void cpu_load_program(const uint8_t* program, size_t size) {
    if (size <= MEMORY_SIZE) {
        memcpy(memory.data, program, size);
    }
}

/*
 * @brief 현재 PC 위치에서 명령어를 패치합니다
 * @param 없음
 * @returns 패치된 16비트 명령어
 */
uint16_t fetch_instruction(void) {
    if (regs.pc >= MEMORY_SIZE - 1) {
        return 0; // 메모리 범위 초과
    }
    
    uint16_t inst = (memory_read(&memory, regs.pc) << 8) | memory_read(&memory, regs.pc + 1);
    return inst;
}

/*
 * @brief 명령어를 디코드하고 실행합니다
 * @param instruction 실행할 16비트 명령어
 * @returns 없음 (void)
 */
void decode_and_execute(uint16_t instruction) {
    // 4비트 opcode 추출
    uint8_t opcode = (instruction >> 12) & 0xF;

    printf("\n=== 명령어 디코딩 ===\n");
    printf("바이트: 0x%04X -> opcode=%d\n", instruction, opcode);

    // 🎯 MOV 명령어 특별 처리
    if (opcode == 4) {
        // MOV 레지스터, 즉시값: 4비트 opcode + 4비트 레지스터 + 8비트 즉시값
        uint8_t reg_num = (instruction >> 8) & 0xF;
        uint8_t immediate_val = instruction & 0xFF;
        
        // 레지스터 번호가 1-7 범위인지 확인 (새로운 MOV 포맷)
        if (reg_num >= 1 && reg_num <= 7) {
            printf("🎯 새로운 MOV 포맷: R%d에 값 %d 저장\n", reg_num, immediate_val);
            
            // 레지스터에 값 저장
            set_register(&regs, reg_num, immediate_val);
            
            // 저장 확인
            uint8_t stored_value = get_register(&regs, reg_num);
            printf("✅ MOV 완료: R%d = %d (저장됨!)\n", reg_num, stored_value);
            
            // 모든 레지스터 상태 출력 (디버깅용)
            printf("전체 레지스터 상태:\n");
            for (int i = 1; i <= 7; i++) {
                printf("  R%d = %d\n", i, get_register(&regs, i));
            }
            
            regs.pc += 2;
            printf("PC: %d\n", regs.pc);
            printf("====================\n\n");
            return;
        }
        // 기존 MOV 포맷으로 처리 (메모리 주소)
        else {
            printf("📊 기존 MOV 포맷으로 처리\n");
        }
    }
    
    // 🚀 ADD/SUB/MUL/DIV 명령어 새로운 레지스터 포맷 처리
    if (opcode >= 0 && opcode <= 3) {
        uint8_t flag = instruction & 0xF;
        
        // 새로운 레지스터 포맷인지 확인 (플래그가 0xF)
        if (flag == 0xF) {
            uint8_t reg1_num = (instruction >> 8) & 0xF;
            uint8_t reg2_num = (instruction >> 4) & 0xF;
            
            if (reg1_num >= 1 && reg1_num <= 7 && reg2_num >= 1 && reg2_num <= 7) {
                printf("🚀 새로운 ALU 포맷: R%d %s R%d\n", reg1_num, 
                       (opcode == 0) ? "+" : (opcode == 1) ? "-" : (opcode == 2) ? "*" : "/", reg2_num);
                
                // 레지스터에서 값 읽기
                uint8_t operand1 = get_register(&regs, reg1_num);
                uint8_t operand2 = get_register(&regs, reg2_num);
                
                printf("값: R%d(%d) %s R%d(%d)\n", reg1_num, operand1, 
                       (opcode == 0) ? "+" : (opcode == 1) ? "-" : (opcode == 2) ? "*" : "/", reg2_num, operand2);
                
                // ALU 연산 수행
                uint8_t result = handler_table[opcode](operand1, operand2);
                
                printf("결과: %d %s %d = %d\n", operand1, 
                       (opcode == 0) ? "+" : (opcode == 1) ? "-" : (opcode == 2) ? "*" : "/", operand2, result);
                
                // 결과를 R7에 저장 (결과 레지스터)
                set_register(&regs, 7, result);
                
                printf("✅ ALU 완료: R7 = %d (결과 저장됨!)\n", result);
                
                // 모든 레지스터 상태 출력 (디버깅용)
                printf("전체 레지스터 상태:\n");
                for (int i = 1; i <= 7; i++) {
                    printf("  R%d = %d\n", i, get_register(&regs, i));
                }
                
                regs.pc += 2;
                printf("PC: %d\n", regs.pc);
                printf("====================\n\n");
                return;
            }
        }
    }

    // 기존 방식: 4비트 opcode + 6비트 reg1 + 6비트 reg2
    uint8_t reg1_val = (instruction >> 6) & 0x3F;
    uint8_t reg2_val = instruction & 0x3F;

    printf("기존 포맷: reg1=%d, reg2=%d\n", reg1_val, reg2_val);

    uint8_t operand1, operand2;
    
    // 첫 번째 피연산자 해석
    if (reg1_val >= 101 && reg1_val <= 107) {
        // 레지스터 (R1=101, R2=102, ..., R7=107)
        uint8_t reg_num = reg1_val - 100;
        operand1 = get_register(&regs, reg_num);
        printf("첫 번째: R%d (현재값=%d)\n", reg_num, operand1);
    } else {
        // 즉시값 또는 메모리 주소
        operand1 = reg1_val;
        printf("첫 번째: 즉시값/주소 %d\n", operand1);
    }
    
    // 두 번째 피연산자 해석
    if (reg2_val >= 101 && reg2_val <= 107) {
        // 레지스터
        uint8_t reg_num = reg2_val - 100;
        operand2 = get_register(&regs, reg_num);
        printf("두 번째: R%d (현재값=%d)\n", reg_num, operand2);
    } else {
        // 즉시값
        operand2 = reg2_val;
        printf("두 번째: 즉시값 %d\n", operand2);
    }

    // 명령어 실행
    if (opcode < 4) {
        // ALU 연산
        uint8_t result = handler_table[opcode](operand1, operand2);
        
        set_register(&regs, 1, operand1);  // R1 = 첫 번째 피연산자
        set_register(&regs, 2, operand2);  // R2 = 두 번째 피연산자  
        set_register(&regs, 7, result);    // resultR = 결과
        
        if (70 + opcode < MEMORY_SIZE) {
            memory.data[70 + opcode] = result;
        }
        
        printf("✅ %s 연산: %d %c %d = %d\n", 
               (opcode == 0) ? "ADD" : (opcode == 1) ? "SUB" : (opcode == 2) ? "MUL" : "DIV",
               operand1, (opcode == 0) ? '+' : (opcode == 1) ? '-' : (opcode == 2) ? '*' : '/',
               operand2, result);
               
    } else if (opcode == 4) {
        // 기존 MOV 명령어 (메모리 주소)
        if (reg1_val >= 101 && reg1_val <= 107) {
            // 🎯 MOV R4, 50 형태 → 레지스터에 값 저장
            uint8_t target_reg = reg1_val - 100;
            
            printf("📝 기존 MOV 실행 중: R%d에 값 %d 저장...\n", target_reg, operand2);
            
            // 레지스터에 값 저장
            set_register(&regs, target_reg, operand2);
            
            // 저장 확인
            uint8_t stored_value = get_register(&regs, target_reg);
            printf("✅ MOV 완료: R%d = %d (저장됨!)\n", target_reg, stored_value);
            
            // 모든 레지스터 상태 출력 (디버깅용)
            printf("전체 레지스터 상태:\n");
            for (int i = 1; i <= 7; i++) {
                printf("  R%d = %d\n", i, get_register(&regs, i));
            }
            
        } else {
            // 🗃️ MOV 255, 32 형태 → 메모리에 값 저장
            printf("📝 MOV 실행 중: 메모리[%d]에 값 %d 저장...\n", operand1, operand2);
            
            if (operand1 < MEMORY_SIZE) {
                memory.data[operand1] = operand2;
                printf("✅ MOV 완료: 메모리[%d] = %d (저장됨!)\n", operand1, operand2);
            } else {
                printf("❌ MOV 실패: 메모리 주소 %d 범위 초과\n", operand1);
            }
        }
    }

    regs.pc += 2;
    printf("PC: %d\n", regs.pc);
    printf("====================\n\n");
}

/*
 * @brief CPU를 한 단계 실행합니다
 * @param 없음
 * @returns 없음 (void)
 */
void cpu_step(void) {
    if (regs.pc >= MEMORY_SIZE - 1) {
        return; // 프로그램 종료
    }
    
    uint16_t instruction = fetch_instruction();
    if (instruction != 0) {
        decode_and_execute(instruction);
    }
}

/*
 * @brief CPU를 연속으로 실행합니다
 * @param 없음
 * @returns 없음 (void)
 */
void cpu_run(void) {
    // 프로그램 실행 루프
    for (int i = 0; i < 4 && regs.pc < MEMORY_SIZE - 1; i++) {
        cpu_step();
    }
}

/*
 * @brief 샘플 프로그램으로 CPU를 실행합니다
 * @param 없음
 * @returns 없음 (void)
 */
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

/*
 * @brief CPU 상태를 출력합니다 (디버깅용)
 * @param 없음
 * @returns 없음 (void)
 */
void print_cpu_state() {
    printf("PC: %d\n", regs.pc);
    printf("Register1: %d\n", regs.register1);
    printf("Register2: %d\n", regs.register2);
    printf("Register3: %d\n", regs.register3);
}

/*
 * @brief CPU 레지스터 포인터를 반환합니다
 * @param 없음
 * @returns CPU 레지스터 구조체 포인터
 */
CPU_Registers* get_cpu_registers(void) {
    return &regs;
}

/*
 * @brief CPU 메모리 포인터를 반환합니다
 * @param 없음
 * @returns 메모리 구조체 포인터
 */
Memory* get_cpu_memory(void) {
    return &memory;
}


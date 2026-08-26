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
 *
 * 명령어 포맷 (16비트):
 *   Type I (즉시값):   [opcode:4][0:1][Rd:3][immediate:8]
 *   Type R (레지스터): [opcode:4][1:1][Rd:3][Rs:3][unused:5]
 *
 * opcode: ADD=0, SUB=1, MUL=2, DIV=3, MOV=4, CMP=5, JMP=6, JE=7, JNE=8
 */
void decode_and_execute(uint16_t instruction) {
    uint8_t opcode = (instruction >> 12) & 0xF;
    uint8_t mode   = (instruction >> 11) & 0x1;
    uint8_t rd     = (instruction >> 8)  & 0x7;

    printf("\n=== 명령어 디코딩 ===\n");

    // JMP/JE/JNE: 레지스터 불필요, 주소만 사용
    if (opcode >= 6 && opcode <= 8) {
        uint8_t address = instruction & 0xFF;
        const char* jmp_names[] = {NULL, NULL, NULL, NULL, NULL, NULL, "JMP", "JE", "JNE"};

        printf("바이트: 0x%04X | %s %d\n", instruction, jmp_names[opcode], address);

        bool do_jump = false;
        if (opcode == 6) {
            // JMP: 무조건 점프
            do_jump = true;
            printf("JMP: 무조건 주소 %d로 점프\n", address);
        } else if (opcode == 7) {
            // JE: ZF=1이면 점프 (같으면 점프)
            do_jump = get_zero_flag(&regs);
            printf("JE: ZF=%d → %s\n", do_jump, do_jump ? "점프" : "통과");
        } else if (opcode == 8) {
            // JNE: ZF=0이면 점프 (다르면 점프)
            do_jump = !get_zero_flag(&regs);
            printf("JNE: ZF=%d → %s\n", !do_jump, do_jump ? "점프" : "통과");
        }

        if (do_jump) {
            regs.pc = address;
            printf("PC = %d (점프)\n", regs.pc);
        } else {
            regs.pc += 2;
            printf("PC = %d (다음 명령어)\n", regs.pc);
        }
        printf("====================\n\n");
        return;
    }

    printf("바이트: 0x%04X | opcode=%d, mode=%s, Rd=R%d\n",
           instruction, opcode, mode ? "REG" : "IMM", rd);

    if (rd < 1 || rd > 7) {
        printf("잘못된 목적지 레지스터: %d\n", rd);
        regs.pc += 2;
        return;
    }

    // 두 번째 피연산자 결정
    uint8_t operand2;
    char operand2_str[16];

    if (mode == 1) {
        // Type R: 레지스터 + 레지스터
        uint8_t rs = (instruction >> 5) & 0x7;
        if (rs < 1 || rs > 7) {
            printf("잘못된 소스 레지스터: %d\n", rs);
            regs.pc += 2;
            return;
        }
        operand2 = get_register(&regs, rs);
        snprintf(operand2_str, sizeof(operand2_str), "R%d(%d)", rs, operand2);
    } else {
        // Type I: 레지스터 + 즉시값
        operand2 = instruction & 0xFF;
        snprintf(operand2_str, sizeof(operand2_str), "%d", operand2);
    }

    if (opcode <= 3) {
        // ALU 연산: ADD(0), SUB(1), MUL(2), DIV(3)
        const char* op_names[] = {"ADD", "SUB", "MUL", "DIV"};
        const char  op_chars[] = {'+', '-', '*', '/'};

        uint8_t operand1 = get_register(&regs, rd);
        uint8_t result   = handler_table[opcode](operand1, operand2);

        printf("%s: R%d(%d) %c %s = %d\n",
               op_names[opcode], rd, operand1, op_chars[opcode], operand2_str, result);

        set_register(&regs, rd, result);
        printf("R%d = %d\n", rd, result);

    } else if (opcode == 4) {
        // MOV: 값을 목적지 레지스터에 저장
        set_register(&regs, rd, operand2);
        printf("MOV: R%d = %s\n", rd, operand2_str);

    } else if (opcode == 5) {
        // CMP: 두 값을 빼서 플래그만 설정 (결과 저장 안 함)
        uint8_t operand1 = get_register(&regs, rd);
        uint8_t diff = operand1 - operand2;

        set_zero_flag(&regs, diff == 0);

        // signed 오버플로우: SUB와 동일한 판정
        bool sign_a = (operand1 & 0x80) != 0;
        bool sign_b = (operand2 & 0x80) != 0;
        bool sign_r = (diff & 0x80) != 0;
        set_overflow_flag(&regs, (!sign_a && sign_b && sign_r) || (sign_a && !sign_b && !sign_r));

        printf("CMP: R%d(%d) vs %s → ZF=%d, OF=%d\n",
               rd, operand1, operand2_str,
               get_zero_flag(&regs), get_overflow_flag(&regs));

    } else {
        printf("알 수 없는 opcode: %d\n", opcode);
    }

    // 레지스터 상태 출력
    printf("전체 레지스터: ");
    for (int i = 1; i <= 7; i++) {
        printf("R%d=%d ", i, get_register(&regs, i));
    }
    printf("| ZF=%d OF=%d\n", get_zero_flag(&regs), get_overflow_flag(&regs));

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
    // 샘플 프로그램: MOV R1,10 → MOV R2,20 → ADD R1,R2 → SUB R1,R2
    uint8_t sample_program[] = {
        0x41, 0x0A,  // MOV R1, 10  (Type I: opcode=4, mode=0, rd=1, imm=10)
        0x42, 0x14,  // MOV R2, 20  (Type I: opcode=4, mode=0, rd=2, imm=20)
        0x09, 0x40,  // ADD R1, R2  (Type R: opcode=0, mode=1, rd=1, rs=2)
        0x19, 0x40   // SUB R1, R2  (Type R: opcode=1, mode=1, rd=1, rs=2)
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


#include "include/cpu.h"
#include "include/alu.h"
#include "include/register.h"
#include "include/instruction.h"
#include <stdio.h>

// 전역 레지스터 (cpu.c에서 정의됨)
extern CPU_Registers regs;

int main() {
    printf("=== 언더플로우 테스트 시작 ===\n\n");
    
    // CPU 초기화
    cpu_init();
    
    printf("1. 초기 캐리 플래그 상태: %s\n", get_carry_flag(&regs) ? "ON" : "OFF");
    
    printf("\n2. 직접 ALU 함수 호출 테스트: subtraction(0, 3)\n");
    uint8_t direct_result = subtraction(0, 3);
    printf("   결과: %d\n", direct_result);
    printf("   캐리 플래그: %s\n", get_carry_flag(&regs) ? "ON" : "OFF");
    
    printf("\n3. handler_table을 통한 호출 테스트\n");
    // handler_table 초기화 확인
    extern alu_handler handler_table[4];
    if (handler_table[1] == NULL) {
        printf("   ❌ handler_table[1]이 NULL입니다!\n");
        init_handler_table();
        printf("   ✅ handler_table 초기화 완료\n");
    }
    
    // 플래그 리셋
    set_carry_flag(&regs, false);
    printf("   플래그 리셋 후: %s\n", get_carry_flag(&regs) ? "ON" : "OFF");
    
    uint8_t handler_result = handler_table[1](0, 3);
    printf("   handler_table[1](0, 3) 결과: %d\n", handler_result);
    printf("   캐리 플래그: %s\n", get_carry_flag(&regs) ? "ON" : "OFF");
    
    printf("\n4. CPU 명령어 실행 테스트: SUB 0, 3 (기존 포맷)\n");
    // 플래그 리셋
    set_carry_flag(&regs, false);
    printf("   실행 전 캐리 플래그: %s\n", get_carry_flag(&regs) ? "ON" : "OFF");
    
    // SUB 0, 3을 바이트로 인코딩: opcode=1, reg1=0, reg2=3
    // 16비트: 0001 000000 000011 = 0x1003
    uint8_t program[] = {0x10, 0x03};  // SUB 0, 3
    
    cpu_reset();
    cpu_load_program(program, 2);
    
    printf("   프로그램 로드: SUB 0, 3 (바이트: 0x%02X 0x%02X)\n", program[0], program[1]);
    printf("   실행 전 캐리 플래그: %s\n", get_carry_flag(&regs) ? "ON" : "OFF");
    
    cpu_step();
    
    printf("   실행 후 캐리 플래그: %s\n", get_carry_flag(&regs) ? "ON" : "OFF");
    printf("   R7 (결과): %d\n", get_register(&regs, 7));
    
    printf("\n=== 테스트 완료 ===\n");
    return 0;
} 
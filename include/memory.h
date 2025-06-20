/* include/memory.h - 메모리 인터페이스 정의
 * ------------------------------------------------------------
 * MEMORY_SIZE 65536 (64KB) 고정 크기의 구조체 선언 및
 * 메모리 초기화, 읽기, 쓰기 함수 정의
 * Test Case: tests/memory_test.c
 * Author: Cho Sungju
*/

#ifndef CPU_MEMORY_H
#define CPU_MEMORY_H

#include "include/cache.h"

#include <stdint.h>

#define MEMORY_SIZE 65536 // 64KB 매모리

typedef struct {
    uint8_t data[MEMORY_SIZE]; // 0x0000 ~ 0xFFFF (총 65536 바이트)
    Cache cache;
} Memory;

void init_memory(Memory *memory);
void memory_write(Memory *memory, uint16_t address, uint8_t value);
uint8_t memory_read(Memory *memory, uint16_t address);

#endif //CPU_MEMORY_H

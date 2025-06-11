/* src/memory.c - 메모리 초기화 및 접근 구현
 * ------------------------------------------------------------
 * 메모리 공간을 초기화하고, 주소 기반 바이트 읽기/쓰기 제공
 * 잘못된 주소 접근에 대한 예외 처리 포함
 * Test Case: tests/memory_test.c
 * Author: Cho Sungju
*/

#include "include/memory.h"

#include <string.h>

/*
 * @brief 메모리의 모든 영역을 0으로 초기화 합니다
 * @param Memory *memory 메모리 인스턴스를 가리키는 포인터
 */
void init_memory(Memory *memory) {
    memset(memory->data, 0, MEMORY_SIZE);
}


uint8_t memory_read(Memory *memory, uint16_t address) {
    if (address >= MEMORY_SIZE) return 0;

    uint16_t index = address % CACHE_SIZE;
    CacheLine *line = &memory->cache[index];

    if (line->valid && line->address == address) {
        return line->data; // 캐시 히트
    }

    // 캐시 미스 → 캐시라인 교체
    if (line->valid && line->dirty) {
        memory->data[line->address] = line->data;
    }

    line->address = address;
    line->data = memory->data[address];
    line->valid = 1;
    line->dirty = 0;

    return line->data;
}


/*
 * @brief 지정된 주소에 값을 기록합니다 (캐시 → 메모리 Write-Back 모사)
 * @param Memory *memory 메모리 인스턴스를 가리키는 포인터
 * @param uint16_t address 기록할 주소
 * @param uint8_t value 기록할 값
 */
void memory_write(Memory *memory, uint16_t address, uint8_t value) {
    if (address >= MEMORY_SIZE) return;

    uint16_t index = address % CACHE_SIZE;
    CacheLine *line = &memory->cache[index];

    // 기존 캐시라인이 다른 주소를 가리키고 dirty하면 메모리에 반영
    if (line->valid && line->dirty && line->address != address) {
        memory->data[line->address] = line->data;
    }

    // 캐시 업데이트
    line->address = address;
    line->data = value;
    line->valid = 1;
    line->dirty = 1;
}

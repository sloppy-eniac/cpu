/* src/memory.c - 메모리 초기화 및 접근 구현
 * ------------------------------------------------------------
 * 메모리 공간을 초기화하고, 주소 기반 바이트 읽기/쓰기 제공
 * 잘못된 주소 접근에 대한 예외 처리 포함
 * Test Case: tests/memory_test.c
 * Author: Cho Sungju
*/

#include "include/memory.h"

#include <stdint.h>
#include <string.h>

/*
 * @brief 메모리의 모든 영역을 0으로 초기화합니다
 * @param memory 메모리 인스턴스를 가리키는 포인터
 * @returns 없음 (void)
 */
void init_memory(Memory *memory) {
    memset(memory->data, 0, MEMORY_SIZE);
}

/*
 * @brief 메모리에서 값을 읽습니다 (캐시 포함)
 * @param memory Memory 구조체 포인터 (캐시 + 실제 메모리 포함)
 * @param address 읽을 메모리 주소
 * @returns 읽은 값 (1바이트), 주소가 잘못된 경우 0 반환
 */
uint8_t memory_read(Memory *memory, uint16_t address) {
    if(address >= MEMORY_SIZE) {
        return 0; // 잘못된 주소 접근 시 0 반환
    }
    return cache_read(&memory->cache, memory->data, MEMORY_SIZE, address);
}

/*
 * @brief 메모리에 값을 기록합니다 (캐시 포함)
 * @param memory Memory 구조체 포인터 (캐시 + 실제 메모리 포함)
 * @param address 기록할 메모리 주소
 * @param value 기록할 값 (1바이트)
 * @returns 없음 (void)
 */
void memory_write(Memory *memory, uint16_t address, uint8_t value) {
    if(address >= MEMORY_SIZE) {
        return; // 잘못된 주소 접근 시 아무 작업도 하지 않음
    }
    cache_write(&memory->cache, memory->data, MEMORY_SIZE, address, value);
}

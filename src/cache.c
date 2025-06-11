/* src/cache.c - 캐시 초기화 및 접근 구현
 * ------------------------------------------------------------
 * 캐시 공간을 초기화하고, 주소 기반 바이트 읽기/쓰기 제공
 * Test Case: tests/cache_test.c
 * Author: Cho Sungju
*/

#include "include/cache.h"
#include <string.h>

/*
 * @brief 캐시의 모든 영역을 0으로 초기화 합니다
 * @param Cache *cache 캐시의 인스턴스를 가리키는 포인터
 */
void cache_init(Cache *cache) {
    memset(cache, 0, sizeof(Cache));
}

/*
 * @brief 캐시를 플러시하여 메모리에 dirty 라인을 기록합니다
 * @param Cache *cache 캐시의 인스턴스를 가리키는 포인터
 * @param uint8_t *memory 메모리 공간을 가리키는 포인터
 * @param size_t mem_size 메모리 크기 (바이트 단위)
 */
void cache_flush(Cache *cache, uint8_t *memory, size_t mem_size) {
    for (size_t i = 0; i < CACHE_SIZE; ++i) {
        CacheLine *line = &cache->lines[i];
        if (line->valid && line->dirty && line->address < mem_size) {
            memory[line->address] = line->data;
        }
        line->valid = CACHE_INVALID;
        line->dirty = 0;
    }
}

/*
 * @brief 캐시에서 주소를 검색하여 히트 여부를 확인합니다
 * @param Cache *cache 캐시의 인스턴스를 가리키는 포인터
 * @param uint16_t address 검색할 주소
 * @return CacheLine* 히트 시 해당 캐시 라인 포인터, 미스 시 NULL
 */
static CacheLine *cache_probe(Cache *cache, uint16_t address) {
    uint16_t index = address % CACHE_SIZE;
    CacheLine *line = &cache->lines[index];
    return (line->valid && line->address == address) ? line : NULL;
}

/*
 * @brief 캐시에서 주소를 읽고, 캐시 미스 시 메모리에서 데이터를 로드합니다
 * @param Cache *cache 캐시의 인스턴스를 가리키는 포인터
 * @param uint8_t *memory 메모리 공간을 가리키는 포인터
 * @param size_t mem_size 메모리 크기 (바이트 단위)
 * @param uint16_t address 읽을 주소
 * @return uint8_t 읽은 값
 */
uint8_t cache_read(Cache *cache, uint8_t *memory, size_t mem_size, uint16_t address) {
    CacheLine *hit = cache_probe(cache, address);
    if (hit) return hit->data;               // 캐시 히트

    /* 캐시 미스 → victim 라인 선택(같은 index) */
    uint16_t index = address % CACHE_SIZE;
    CacheLine *victim = &cache->lines[index];

    /* victim이 dirty면 write-back */
    if (victim->valid && victim->dirty && victim->address < mem_size) {
        memory[victim->address] = victim->data;
    }

    victim->address = address;
    victim->data    = (address < mem_size) ? memory[address] : 0;
    victim->valid   = CACHE_VALID;
    victim->dirty   = 0;

    return victim->data;
}

/*
 * @brief 캐시에 값을 기록하고, 필요 시 메모리에 write-back 합니다
 * @param Cache *cache 캐시의 인스턴스를 가리키는 포인터
 * @param uint8_t *memory 메모리 공간을 가리키는 포인터
 * @param size_t mem_size 메모리 크기 (바이트 단위)
 * @param uint16_t address 기록할 주소
 * @param uint8_t value 기록할 값
 */
void cache_write(Cache *cache, uint8_t *memory, size_t mem_size,
                 uint16_t address, uint8_t value) {
    CacheLine *hit = cache_probe(cache, address);
    if (!hit) {
        /* 캐시 미스 → victim 라인 교체 */
        uint16_t index = address % CACHE_SIZE;
        hit = &cache->lines[index];

        /* 기존 dirty 라인 flush */
        if (hit->valid && hit->dirty && hit->address < mem_size) {
            memory[hit->address] = hit->data;
        }

        hit->address = address;
        hit->valid   = CACHE_VALID;
        hit->dirty   = 0;
    }

    /* 데이터 기록 및 dirty 표시 */
    hit->data  = value;
    hit->dirty = 1;
}
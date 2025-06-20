/* include/cache.h - 캐시 인터페이스 정의
 * ------------------------------------------------------------
 * CPU_CACHE_SIZE 256 (256 캐시 라인) 고정 크기의 구조체 선언 및
 * 캐시 초기화, 읽기, 쓰기 함수 정의
 * Test Case: tests/cache_test.c
 * Author: Cho Sungju
*/

#ifndef CPU_CACHE_H
#define CPU_CACHE_H

#include <stdint.h>
#include <stddef.h>

#define CACHE_LINE_SIZE     4U       /* 4 B 1블록 */
#define CACHE_NUM_LINES     64U      /* 총 64라인 → 256 B */
#define CACHE_ASSOCIATIVITY 1U       /* direct-mapped */

typedef struct {
    uint16_t tag;                    /* 태그 필드 */
    uint8_t block[CACHE_LINE_SIZE];  /* 캐시 라인 데이터 */
    uint8_t valid;                   /* 유효한 캐시 라인인가? */
    uint8_t dirty;                   /* 메모리에 반영되어있는 값인가? */
} CacheLine;

typedef struct {
    CacheLine lines[CACHE_NUM_LINES];
} Cache;

/* 초기화 및 유지보수 */
void cache_init(Cache *cache);
void cache_flush(Cache *cache, uint8_t *memory, size_t mem_size);

/* 읽기/쓰기 연산 */
uint8_t cache_read(Cache *cache, uint8_t *memory, size_t mem_size, uint16_t address);
void    cache_write(Cache *cache, uint8_t *memory, size_t mem_size, uint16_t address, uint8_t value);

#endif //CPU_CACHE_H

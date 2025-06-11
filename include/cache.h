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

#define CACHE_SIZE 256          // 캐시 라인 수
#define CACHE_INVALID 0
#define CACHE_VALID   1

typedef struct {
    uint16_t address;   // 메모리 상 실제 주소(태그 역할)
    uint8_t  data;      // 1-바이트 데이터(간단화를 위해 라인 크기를 1 B로 가정)
    uint8_t  valid;     // 0: invalid, 1: valid
    uint8_t  dirty;     // 0: clean,   1: dirty (write-back 필요)
} CacheLine;

typedef struct {
    CacheLine lines[CACHE_SIZE];
} Cache;

/* 초기화 및 유지보수 */
void cache_init(Cache *cache);
void cache_flush(Cache *cache, uint8_t *memory, size_t mem_size);

/* 읽기/쓰기 연산 */
uint8_t cache_read(Cache *cache, uint8_t *memory, size_t mem_size, uint16_t address);
void    cache_write(Cache *cache, uint8_t *memory, size_t mem_size, uint16_t address, uint8_t value);

#endif //CPU_CACHE_H

/* src/cache.c - 캐시 초기화 및 접근 구현
 * ------------------------------------------------------------
 * 캐시 공간을 초기화하고, 주소 기반 바이트 읽기/쓰기 제공
 * Test Case: tests/cache_test.c
 * Author: Cho Sungju
*/

#include "include/cache.h"
#include <string.h>

/*
 * @brief 메모리 주소를 캐시 접근을 위한 구성 요소로 분해한 구조체
 */
typedef struct {
    uint16_t tag; // 그 칸에 지금 어떤 메모리 블록이 들어와 있는지를 구분 (값 고유 ID)
    uint8_t index; // 캐시 라인 위치
    uint8_t offset; // 한 라인 내 바이트 위치
} AddressInfo;

/*
 * @brief 주소를 tag, index, offset으로 분해합니다
 * @param address 16비트 메모리 주소
 * @returns 분해된 주소 정보 구조체
 */
static inline AddressInfo decode_address(uint16_t address) {
    AddressInfo info;
    info.index = address % CACHE_NUM_LINES; // 캐시 라인 인덱스
    info.offset = address % CACHE_LINE_SIZE; // 블록 내부 오프셋
    info.tag = address / CACHE_NUM_LINES; // 태그 필드 (캐시 라인 외부 정보)
    return info;
}

/*
 * @brief 캐시의 모든 영역을 0으로 초기화합니다
 * @param cache 캐시의 인스턴스를 가리키는 포인터
 * @returns 없음 (void)
 */
void cache_init(Cache *cache) {
    memset(cache, 0, sizeof(Cache));
}

/*
 * @brief 모든 캐시 라인을 메모리에 반영하고 캐시를 초기화합니다
 * @param cache 사용할 캐시 구조체 포인터
 * @param memory 전체 메모리 배열 포인터
 * @param mem_size 메모리의 크기 (바이트 단위)
 * @returns 없음 (void)
 */
void cache_flush(Cache *cache, uint8_t *memory, size_t mem_size) {
    for (size_t i = 0; i < CACHE_NUM_LINES; ++i) {
        CacheLine *l = &cache->lines[i];
        if (l->valid && l->dirty) {
            uint16_t base = (l->tag * CACHE_NUM_LINES + i) * CACHE_LINE_SIZE;
            if (base + CACHE_LINE_SIZE <= mem_size) {
                memcpy(&memory[base], l->block, CACHE_LINE_SIZE);
            }
        }
        memset(l, 0, sizeof(*l)); // 비우기
    }
}

/*
 * @brief 캐시에서 데이터를 읽습니다 (Write-Back + Write-Allocate 방식)
 * @param cache 사용할 캐시 구조체 포인터
 * @param memory 전체 메모리 배열 포인터
 * @param mem_size 메모리의 크기 (바이트 단위)
 * @param address 읽을 대상 주소
 * @returns 읽은 값 (1바이트)
 */
uint8_t cache_read(Cache *cache, uint8_t *memory, size_t mem_size, uint16_t address) {
    AddressInfo a = decode_address(address);
    CacheLine *line = &cache->lines[a.index];

    // 유효한 캐시이다 
    if(line->valid && line->tag == a.tag) {
        return line->block[a.offset];
    }

    // 미스 → 메모리에 있는 값보다 캐시가 더 최신 상태 -> 메모리에 반영
    if(line->valid && line->dirty) {
        uint16_t base = (line->tag * CACHE_NUM_LINES + a.index) * CACHE_LINE_SIZE;
        if(base + CACHE_LINE_SIZE <= mem_size) {
            memcpy(&memory[base], line->block, CACHE_LINE_SIZE);
        }
    }

    /* 
        메모리에 있는 값을 캐시로 가져옵니다.
        캐시 라인에 해당하는 블록을 메모리에서 읽어와서 캐시 라인의 블록에 저장합니다.
        만약 메모리 경계를 넘어가는 경우에는 캐시 라인의 블록을 0으로 초기화합니다.
    */
    uint16_t block_address = address - a.offset; // 블록의 시작 주소
    if(block_address + CACHE_LINE_SIZE <= mem_size) {
        memcpy(line->block, &memory[block_address], CACHE_LINE_SIZE);
    } else {
        // 메모리 경계를 넘어가면 실제 하드웨어처럼 접근할 실 데이터가 없으므로 전부 0으로 초기화
        memset(line->block, 0, CACHE_LINE_SIZE);
    }

    line->tag = a.tag; // 캐시에 새 블록을 할당했으니 태그를 업데이트
    line->valid = 1; // 유효한 캐시 라인으로 설정
    line->dirty = 0; // 더티 플래그 초기화

    return line->block[a.offset]; // 읽은 값 반환
}

/*
 * @brief 캐시에 데이터를 기록합니다 (Write-Back + Write-Allocate 방식)
 * @param cache 사용할 캐시 구조체 포인터
 * @param memory 전체 메모리 배열 포인터
 * @param mem_size 메모리의 크기 (바이트 단위)
 * @param address 쓰기 대상 주소
 * @param value 저장할 값 (1바이트)
 * @returns 없음 (void)
 */
void cache_write(Cache *cache, uint8_t *memory, size_t mem_size, uint16_t address, uint8_t value) {
    AddressInfo a = decode_address(address);
    CacheLine *line = &cache->lines[a.index];

    // 캐시에 해당 블록이 없거나(tag 불일치) 유효하지 않은 경우
    if(!(line->valid && line->tag == a.tag)) {
        // 기존 캐시 블록이 유효하고, 수정된 상태(dirty)면 → 메모리에 반영 (Write-Back)
        if(line->valid && line->dirty) {
            uint16_t base = (line->tag * CACHE_NUM_LINES + a.index) * CACHE_LINE_SIZE;

            // 메모리 범위를 초과하지 않는 경우에만 저장
            if(base + CACHE_LINE_SIZE <= mem_size) {
                memcpy(&memory[base], line->block, CACHE_LINE_SIZE);
            }
        }

        // 새로운 메모리 블록을 캐시에 로드 (Write-Allocate)
        uint16_t block_address = address - a.offset;
        if(block_address + CACHE_LINE_SIZE <= mem_size) {
            memcpy(line->block, &memory[block_address], CACHE_LINE_SIZE); // 메모리 → 캐시
        } else {
            memset(line->block, 0, CACHE_LINE_SIZE);
        };

        line->tag = a.tag;
        line->valid = 1;
        line->dirty = 0; // 아직 메모리에 반영 안 했으므로 false로 초기화
    }

    // 값을 캐시에 기록
    line->block[a.offset] = value;
    line->dirty = 1; // 이후에 Write-Back 필요하므로 dirty 플래그 설정
};
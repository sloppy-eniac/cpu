#include <string.h>
#include "interpreter.c"
#include "include/memory.h"
#include "cpu.c"

void main()
{
    init_memory(&memory);                        // 메모리 초기화화
    memcpy(Memory, buffer, sizeof(MEMORY_SIZE)); // 버퍼 → 메모리 로 복사
    // 레지스터의 pc가 가르키는 주소를 메모리 처음 주소로 설정.
    cpu(); // cpu.c에 만들어놓은 함수 실행행
}
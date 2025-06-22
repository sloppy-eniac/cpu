#include <stdio.h>
#include <stdlib.h>
#include "encoder.h"


Instruction mov_command = {
    .type = MOV_EAX_IMM,
    .dest_reg = REG_EAX,
    .immediate = 0x12345678,  // 여기 콤마는 선택사항 (C99 이상)
}; 
//이거 수정해서 넣으시면 됨

uint8_t buffer[10];
//여기다 저장되는거임

int encode() {
    int bytes = encode_instruction(&mov_command, buffer); 
    return 0;  // 명시적 반환
}

#include "encoder.h"

Instruction mov_command = {
    .type = OP_MOV_REG_IMM,
    .dest_reg = REG_EAX,
    .immediate = 0x12345678,
}

uint8_t buffer[5]; //이제 여기다 메모리 넣으면 될듯

int main() {
    int bytes = encode_instruction(&mov_command, buffer)
    printf("인코딩 결과: ");
    for (int i = 0; i < bytes; i++) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
}

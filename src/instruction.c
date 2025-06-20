include <instruction.h>

typedef uint8_t (*op_handler)(uint8_t, uint8_t);
op_handler handler_table[4];

uint8_t add(uint8_t a, uint8_t b) {return a + b;}
uint8_t subtraction(uint8_t a, uint8_t b) {return a - b;}
uint8_t multiply(uint8_t a, uint8_t b) {return a * b;}
uint8_t divide(uint8_t a, uint8_t b) {return b != 0 ? a / b : 0;}

void init_handler_table() {
    handler_table[0x00] = add;
    handler_table[0x01] = subtraction;
    handler_table[0x02] = multiply;
    handler_table[0x03] = divide;
}



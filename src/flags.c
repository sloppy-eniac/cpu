#include <flags.h>
#include <stdbool.h>

void add_flag(uint8_t a, uint8_t result)
{
    *cf = (result < a);
}

void add_flag(uint8_t a, uint8_t b)
{
    *cf = (b > a);
}

// 이제 결과 보여줄때 캐리값 같이 보여주면 되는거임!!
// 더한후 결과는 255더한값으로 보여주면 되는거고
// 뺀후에는 걍 에러 띄워야지 뭐. 음수표현 안됨 우리
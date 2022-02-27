#include <stdio.h>
#include <stdint.h>

int main() {
    uint8_t a = 0;
    uint16_t b = 0;
    uint32_t c = 0;
    a = ~a;
    b = ~b;
    c = ~c;

    printf("8b: %u\n16b: %u\n32b: %u\n", a, b, c);
}

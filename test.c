#include <stdio.h>
#include <stdint.h>

int main() {
    // uint8_t a = 0;
    // uint16_t b = 0;
    // uint32_t c = 0;
    // scanf("%hhd %hd", &a, &b);
    // scanf("%d", &c);
    // printf("%hhd %hd %d", a, b, c);

    char x;
    scanf("%c", &x);
    printf("%d", (int) (x == '\n'));
}

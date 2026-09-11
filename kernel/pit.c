#include "pit.h"
#include "io.h"
#define PIT_CH0  0x40
#define PIT_CMD  0x43
#define PIT_BASE 1193182
void pit_init_100hz(void) {
    uint32_t divisor = PIT_BASE / 100;
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
}

void kernel_main(void) __attribute__((section(".text")));
#include <stdint.h>
#include "kprintf.h"
#include "idt.h"

void kernel_main(void) {
    kclear_screen();
    kprintf("LikeOS-NG kernel booting...\n");
    kprintf("Enabled protected mode.\n");
    idt_install();
    kprintf("Idt initialized.\n");

    int i = 1/0;

    for (;;) { __asm__ __volatile__("hlt"); }
}

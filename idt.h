#pragma once
#include <stdint.h>

void idt_install(void);
void isr_common_stub(uint32_t interrupt_number);
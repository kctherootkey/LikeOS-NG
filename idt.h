#pragma once
#include <stdint.h>

// Interrupt frame structure (matches what ISR pushes on stack)
struct interrupt_frame {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;  // Pushed by pushad
    uint32_t interrupt_number, error_code;                   // Pushed by ISR
    uint32_t eip, cs, eflags, esp, ss;                      // Pushed by CPU
} __attribute__((packed));

void idt_install(void);
void isr_common_stub(struct interrupt_frame* frame);
void irq_common_stub(struct interrupt_frame* frame);
void show_register_dump(struct interrupt_frame* frame);
void kernel_panic(struct interrupt_frame* frame) __attribute__((noreturn));
void irq_install(void);
void irq_set_mask(unsigned char irq_line);
void irq_clear_mask(unsigned char irq_line);
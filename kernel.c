void kernel_main(void) __attribute__((section(".text")));
#include <stdint.h>
#include "kprintf.h"
#include "idt.h"
#include "keyboard.h"
#include "paging.h"
#include "pmm.h"

void kernel_main(void) {
    kclear_screen();
    kprintf("LikeOS-NG kernel booting...\n");
    kprintf("Enabled protected mode.\n");
    idt_install();
    kprintf("IDT initialized.\n");
    
    kprintf("Initializing memory management...\n");
    pmm_init();
    pmm_print_stats();
    pmm_print_memory_map();
    
    paging_init();
    setup_identity_mapping();
    setup_kernel_heap();
    enable_pae_paging();
    kprintf("PAE paging is now active.\n");
    get_memory_stats();
    
    keyboard_init();
    kprintf("Keyboard initialized.\n");
    irq_install();
    kprintf("IRQ handlers installed.\n");
    kprintf("Enabling interrupts...\n");
    
    // Enable interrupts
    __asm__ __volatile__("sti");
    
    kprintf("System ready.\n");
    
    // Remove the division by zero test for now since we want to test IRQs
    // int i = 1/0;

    for (;;) { __asm__ __volatile__("hlt"); }
}

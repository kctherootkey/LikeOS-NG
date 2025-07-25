void kernel_main(void) __attribute__((section(".text")));
#include <stdint.h>
#include "lib/kprintf.h"
#include <stdint.h>
#include "lib/kprintf.h"
#include "lib/timing.h"
#include "interrupt/idt.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "memory/paging.h"
#include "memory/pmm.h"

void kernel_main(void) {
    kclear_screen();
    kprintf("LikeOS-NG kernel booting...\n");
    kprintf("Enabled protected mode.\n");
    
    // Enable A20 gate early to access memory above 1MB
    enable_a20_gate();
    
    // Initialize timing subsystem early
    timing_init();
    
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
    
    vga_init();
    kprintf("VGA driver initialized.\n");
    
    irq_install();
    kprintf("IRQ handlers installed.\n");
    kprintf("Enabling interrupts...\n");
    
    // Enable interrupts
    __asm__ __volatile__("sti");
    
    kprintf("System ready.\n");
    kprintf("Switching to VESA 1024x768 graphics mode in 3 seconds...\n");
    
    // Countdown with accurate timing
    for (int countdown = 3; countdown > 0; countdown--) {
        kprintf(".");
        timing_delay_seconds(1);
    }
    kprintf("\n");
    
    // Automatically switch to VESA mode
    kprintf("Switching to VESA mode now...\n");
    vga_set_vesa_mode_1024x768();
    
    // After VESA mode switch, kprintf won't work anymore
    // Just halt the system
    for (;;) { __asm__ __volatile__("hlt"); }
}

void kernel_main(void) __attribute__((section(".text")));
#include <stdint.h>
#include "lib/kprintf.h"
#include <stdint.h>
#include "lib/kprintf.h"
#include "interrupt/idt.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "memory/paging.h"
#include "memory/pmm.h"

// Accurate delay function using TSC (Time Stamp Counter)
static uint64_t rdtsc(void) {
    uint32_t low, high;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

static void delay_seconds(uint32_t seconds) {
    // Calibrate TSC frequency by measuring ticks over a known delay
    // Use a short busy-wait to estimate CPU frequency
    uint64_t start_tsc = rdtsc();
    
    // Short calibration loop (approximately 100ms)
    for (volatile uint32_t i = 0; i < 5000000; i++) {
        __asm__ __volatile__("" ::: "memory");
    }
    
    uint64_t end_tsc = rdtsc();
    uint64_t ticks_per_100ms = end_tsc - start_tsc;
    uint64_t ticks_per_second = ticks_per_100ms * 10;
    
    // Now use TSC for accurate timing
    uint64_t target_ticks = ticks_per_second * seconds;
    start_tsc = rdtsc();
    
    while ((rdtsc() - start_tsc) < target_ticks) {
        // Yield CPU to prevent 100% usage
        __asm__ __volatile__("pause");
    }
}

void kernel_main(void) {
    kclear_screen();
    kprintf("LikeOS-NG kernel booting...\n");
    kprintf("Enabled protected mode.\n");
    
    // Enable A20 gate early to access memory above 1MB
    enable_a20_gate();
    
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
    kprintf("Switching to VESA 1024x768 graphics mode.\n");
    
    // Countdown with accurate timing
    for (int countdown = 15; countdown > 0; countdown--) {
        kprintf(".", countdown);
        delay_seconds(1);
    }
    kprintf("\n");
    
    // Automatically switch to VESA mode
    kprintf("Switching to VESA mode now...\n");
    vga_set_vesa_mode_1024x768();
    
    // After VESA mode switch, kprintf won't work anymore
    // Just halt the system
    for (;;) { __asm__ __volatile__("hlt"); }
}

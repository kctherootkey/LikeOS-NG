#include "lib/timing.h"
#include "lib/kprintf.h"

static uint64_t tsc_frequency = 0;

uint64_t timing_read_tsc(void) {
    uint32_t low, high;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

uint64_t timing_calibrate_tsc_frequency(void) {
    kprintf("Calibrating TSC frequency...\n");
    
    // Measure TSC ticks over a known delay
    uint64_t start_tsc = timing_read_tsc();
    
    // Calibration loop (approximately 100ms)
    // This is a rough calibration - adjust the loop count if needed
    for (volatile uint32_t i = 0; i < 5000000; i++) {
        __asm__ __volatile__("" ::: "memory");
    }
    
    uint64_t end_tsc = timing_read_tsc();
    uint64_t ticks_per_100ms = end_tsc - start_tsc;
    
    // Calculate ticks per second (multiply by 10 for 100ms -> 1000ms)
    uint64_t frequency = ticks_per_100ms * 10;
    
    kprintf("TSC frequency estimated at %llu Hz\n", frequency);
    return frequency;
}

void timing_init(void) {
    kprintf("Initializing timing subsystem...\n");
    tsc_frequency = timing_calibrate_tsc_frequency();
    kprintf("Timing subsystem initialized.\n");
}

void timing_delay_seconds(uint32_t seconds) {
    if (tsc_frequency == 0) {
        kprintf("Warning: TSC not calibrated, using fallback delay\n");
        // Fallback to simple busy wait
        for (uint32_t s = 0; s < seconds; s++) {
            for (volatile uint64_t i = 0; i < 50000000ULL; i++) {
                __asm__ __volatile__("" ::: "memory");
            }
        }
        return;
    }
    
    uint64_t target_ticks = tsc_frequency * seconds;
    uint64_t start_tsc = timing_read_tsc();
    
    while ((timing_read_tsc() - start_tsc) < target_ticks) {
        // Yield CPU to prevent 100% usage
        __asm__ __volatile__("pause");
    }
}

void timing_delay_milliseconds(uint32_t milliseconds) {
    if (tsc_frequency == 0) {
        kprintf("Warning: TSC not calibrated, using fallback delay\n");
        // Simple fallback
        for (uint32_t ms = 0; ms < milliseconds; ms++) {
            for (volatile uint32_t i = 0; i < 50000; i++) {
                __asm__ __volatile__("" ::: "memory");
            }
        }
        return;
    }
    
    // Convert milliseconds to seconds and use the seconds function
    // For sub-second delays, use a simpler approach
    if (milliseconds >= 1000) {
        uint32_t seconds = milliseconds / 1000;
        uint32_t remaining_ms = milliseconds % 1000;
        
        timing_delay_seconds(seconds);
        
        if (remaining_ms > 0) {
            // For remaining milliseconds, use a simple busy wait
            for (volatile uint32_t i = 0; i < remaining_ms * 50000; i++) {
                __asm__ __volatile__("" ::: "memory");
            }
        }
    } else {
        // For less than 1 second, use busy wait
        for (volatile uint32_t i = 0; i < milliseconds * 50000; i++) {
            __asm__ __volatile__("" ::: "memory");
        }
    }
}

uint64_t timing_get_elapsed_ticks(uint64_t start_tsc) {
    return timing_read_tsc() - start_tsc;
}

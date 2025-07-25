#include "drivers/vga.h"
#include "lib/kprintf.h"
#include "memory/paging.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// External assembly functions
extern int vesa_set_mode_1024x768(void);
extern uint32_t vesa_get_lfb_address(void);
extern uint32_t vesa_get_mode_width(void);
extern uint32_t vesa_get_mode_height(void);
extern uint32_t vesa_get_mode_bpp(void);
extern uint32_t vesa_get_pitch(void);
extern int vesa_set_text_mode_80x25(void);

// VGA driver state
static int current_mode = 0;  // 0 = text, 1 = VESA graphics

// Initialize VGA driver
int vga_init(void) {
    kprintf("VGA: Initializing graphics driver...\n");
    current_mode = 0;  // Start in text mode
    kprintf("VGA: Driver initialized in text mode\n");
    return 0;
}

// Set VESA 1024x768x32 mode
int vga_set_vesa_mode_1024x768(void) {
    kprintf("VGA: Attempting to set VESA 1024x768 mode...\n");
    
    // Call the assembly function that does the real mode switching
    int result = vesa_set_mode_1024x768();
    
    if (result == 0) {
        // Get and display the mode information for debugging
        uint32_t lfb_addr = vesa_get_lfb_address();
        uint32_t width = vesa_get_mode_width();
        uint32_t height = vesa_get_mode_height();
        uint32_t bpp = vesa_get_mode_bpp();
        
        kprintf("VGA: VESA mode set successfully!\n");
        kprintf("VGA: Resolution: %dx%d, %d bpp, LFB at 0x%08X\n", width, height, bpp, lfb_addr);
        
        // IMPORTANT: Once we're in VESA mode, kprintf won't work anymore!
        // We need to draw the screen immediately
        current_mode = 1;
        
        // Draw screen immediately - no debug output after this point
        vga_clear_screen();
        
        return 0;
    } else {
        kprintf("VGA: Failed to set VESA mode (error %d)\n", result);
        return -1;
    }
}

// Clear screen immediately without any debug output (for use after VESA mode switch)
void vga_clear_screen_(void) {
    // Get the actual mode information from VESA BIOS
    uint32_t lfb_addr = vesa_get_lfb_address();
    uint32_t width = vesa_get_mode_width();
    uint32_t height = vesa_get_mode_height();
    uint32_t bpp = vesa_get_mode_bpp();
    
    // If LFB address is 0, fall back to common address
    if (lfb_addr == 0) {
        lfb_addr = 0xE0000000;  // Fallback to common address
    }
    
    // If we don't have valid dimensions, use defaults
    if (width == 0 || height == 0) {
        width = 1024;
        height = 768;
    }
    
    // Calculate framebuffer size based on actual mode info
    uint32_t bytes_per_pixel = (bpp + 7) / 8;  // Round up to nearest byte
    uint32_t fb_size = width * height * bytes_per_pixel;
    uint32_t pages_needed = (fb_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Map the framebuffer memory into our virtual address space
    for (uint32_t i = 0; i < pages_needed; i++) {
        uint32_t virtual_addr = lfb_addr + (i * PAGE_SIZE);
        uint64_t physical_addr = lfb_addr + (i * PAGE_SIZE);
        
        // Map with present, writable, and cache-disabled flags
        int result = map_page(virtual_addr, physical_addr, 
                             PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        
        if (result != 0) {
            // Mapping failed - can't draw to framebuffer
            return;
        }
    }
    
    // Handle different color depths
    if (bpp == 16) {
        // 16-bit color mode
        uint16_t *framebuffer = (uint16_t*)lfb_addr;
        uint16_t white = 0xFFFF;  // White in RGB565
        
        uint32_t total_pixels = width * height;
        for (uint32_t i = 0; i < total_pixels; i++) {
            framebuffer[i] = white;
        }
    } else if (bpp == 24 || bpp == 32) {
        // 24-bit or 32-bit color mode
        uint32_t *framebuffer = (uint32_t*)lfb_addr;
        uint32_t white = 0x00FFFFFF;  // White in RGB
        
        uint32_t total_pixels = width * height;
        for (uint32_t i = 0; i < total_pixels; i++) {
            framebuffer[i] = white;
        }
    }
}

// Clear screen (legacy function with debug output)
void vga_clear_screen(void) {
    if (current_mode != 1) {
        kprintf("VGA: Not in VESA graphics mode, cannot clear\n");
        return;
    }
    
    // Just call the immediate version
    vga_clear_screen_();
}

// Get current mode
int vga_get_mode(void) {
    return current_mode;
}

// Get the linear framebuffer address
uint32_t vga_get_lfb_address(void) {
    return vesa_get_lfb_address();
}

// Switch back to VGA text mode (80x25, 16 color)
void vga_set_text_mode_80x25(void) {
    // Call the assembly function that does proper real mode switching
    int result = vesa_set_text_mode_80x25();
    
    if (result == 0) {
        current_mode = 0;  // Set mode back to text
        kprintf("VGA: Switched back to 80x25 text mode\n");
    } else {
        kprintf("VGA: Failed to switch to text mode\n");
    }
}

#include "drivers/vga.h"
#include "lib/kprintf.h"

// External assembly functions
extern int vesa_set_mode_1024x768x24(void);
extern int vesa_test_real_mode(void);

// VGA driver state
static int current_mode = 0;  // 0 = text, 1 = VESA graphics

// Initialize VGA driver
int vga_init(void) {
    kprintf("VGA: Initializing graphics driver...\n");
    current_mode = 0;  // Start in text mode
    kprintf("VGA: Driver initialized in text mode\n");
    return 0;
}

// Test real mode switching before attempting VESA
int vga_test_real_mode_switching(void) {
    kprintf("VGA: Testing real mode switching...\n");
    
    int result = vesa_test_real_mode();
    
    if (result == 0) {
        kprintf("VGA: Real mode switching test PASSED\n");
        return 0;
    } else {
        kprintf("VGA: Real mode switching test FAILED\n");
        return -1;
    }
}

// Set VESA 1024x768x24 mode
int vga_set_vesa_mode_1024x768x24(void) {
    kprintf("VGA: Attempting to set VESA 1024x768x16 mode...\n");
    
    // First test if real mode switching works
    /*if (vga_test_real_mode_switching() != 0) {
        kprintf("VGA: Cannot proceed - real mode switching failed\n");
        return -1;
    }*/
    
    // Call the assembly function that does the real mode switching
    int result = vesa_set_mode_1024x768x24();
    
    if (result == 0) {
        kprintf("VGA: VESA mode set successfully!\n");
        current_mode = 1;
        kprintf("VGA: Now in 1024x768x16 graphics mode\n");
        return 0;
    } else {
        kprintf("VGA: Failed to set VESA mode (error %d)\n", result);
        return -1;
    }
}

// Clear screen to blue color (only in VESA mode)
void vga_clear_screen_blue(void) {
    if (current_mode != 1) {
        kprintf("VGA: Not in VESA graphics mode, cannot clear to blue\n");
        return;
    }
    
    kprintf("VGA: Screen should now display in 1024x768x16 mode\n");
    kprintf("VGA: (Blue screen drawing skipped - requires memory mapping)\n");
}

// Get current mode
int vga_get_mode(void) {
    return current_mode;
}

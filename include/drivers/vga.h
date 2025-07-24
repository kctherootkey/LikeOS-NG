#pragma once
#include <stdint.h>

// VGA Graphics Driver for VESA modes

// Function declarations
int vga_init(void);
int vga_test_real_mode_switching(void);  // Test real mode switching capability
int vga_set_vesa_mode_1024x768x24(void);
void vga_clear_screen_blue(void);
int vga_get_mode(void);

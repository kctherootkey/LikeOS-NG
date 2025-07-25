#pragma once
#include <stdint.h>

// VGA Graphics Driver for VESA modes

// Function declarations
int vga_init(void);
int vga_test_real_mode_switching(void);  // Test real mode switching capability
int vga_set_vesa_mode_1024x768(void);
void vga_clear_screen_blue(void);
void vga_clear_screen_blue_immediate(void);  // Draw blue screen without debug output
int vga_get_mode(void);
uint32_t vga_get_lfb_address(void);  // Get linear framebuffer address
uint32_t vesa_get_pitch(void);  // Get pitch of the framebuffer
void vga_set_text_mode_80x25(void);  // Switch back to text mode

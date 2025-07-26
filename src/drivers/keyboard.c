#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "lib/kprintf.h"

// Scancode to ASCII conversion table (US QWERTY layout)
static char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Shift versions for uppercase and special characters
static char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int shift_pressed = 0;
static int caps_lock = 0;
static int alt_pressed = 0;
static int vesa_mode_active = 0;  // Track current mode

void keyboard_init(void) {
    // Initialize keyboard state
    shift_pressed = 0;
    caps_lock = 0;
    alt_pressed = 0;
    vesa_mode_active = 0;
}

void keyboard_handler(uint8_t scancode) {
    // Handle key releases (scancode with bit 7 set)
    if (scancode & 0x80) {
        scancode &= 0x7F;  // Remove release bit
        
        // Handle modifier key releases
        if (scancode == 0x2A || scancode == 0x36) {  // Left or right shift
            shift_pressed = 0;
        } else if (scancode == 0x38) {  // Left Alt
            alt_pressed = 0;
        }
    } else {
        // Handle key presses
        
        // Handle modifier keys
        if (scancode == 0x2A || scancode == 0x36) {  // Left or right shift
            shift_pressed = 1;
        } else if (scancode == 0x38) {  // Left Alt
            alt_pressed = 1;
        } else if (scancode == 0x01) {  // Escape key
            // Remove old Shift+Escape handling since we're using Alt+G now
            // Just process escape normally
            if (!alt_pressed) {
                kputchar(27);  // ESC character
            }
        } else if (scancode == 0x3A) {  // Caps lock
            caps_lock = !caps_lock;
        } else if (scancode < sizeof(scancode_to_ascii)) {
            // Check for Alt+G combination
            if (alt_pressed && (scancode == 0x22)) {  // 0x22 is scancode for 'G'
                // Toggle between VESA mode and text mode
                if (vesa_mode_active) {
                    // Switch back to text mode
                    vga_set_text_mode_80x25();
                    vesa_mode_active = 0;
                    kprintf("\n[DEBUG] Switched back to text mode via Alt+G\n");
                } else {
                    // Switch to VESA mode
                    kprintf("\nSwitching to VESA mode via Alt+G...\n");
                    if (vga_set_vesa_mode_1024x768() == 0) {
                        vesa_mode_active = 1;
                        // Mode set successfully, blue screen is drawn automatically
                        // We're now in graphics mode - no more text output will be visible
                    } else {
                        kprintf("VESA mode failed.\n");
                    }
                }
                return;  // Don't process the 'G' as a regular character
            }
            
            // Convert scancode to ASCII (only if Alt is not pressed)
            if (!alt_pressed) {
                char ascii = 0;
                
                if (shift_pressed) {
                    ascii = scancode_to_ascii_shift[scancode];
                } else {
                    ascii = scancode_to_ascii[scancode];
                    
                    // Apply caps lock for letters
                    if (caps_lock && ascii >= 'a' && ascii <= 'z') {
                        ascii = ascii - 'a' + 'A';
                    }
                }
                
                // Print the character if it's printable
                if (ascii != 0) {
                    if (ascii == '\n') {
                        kprintf("\n");
                    } else if (ascii == '\b') {
                        // Send backspace character directly to kputchar for proper handling
                        kputchar('\b');
                    } else if (ascii == '\t') {
                        kprintf("    ");  // 4 spaces for tab
                    } else {
                        // Remove special handling for 'g' since we now use Alt+G
                        kputchar(ascii);
                    }
                }
            }
        }
    }
}

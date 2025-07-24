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

void keyboard_init(void) {
    // Initialize keyboard state
    shift_pressed = 0;
    caps_lock = 0;
}

void keyboard_handler(uint8_t scancode) {
    // Handle key releases (scancode with bit 7 set)
    if (scancode & 0x80) {
        scancode &= 0x7F;  // Remove release bit
        
        // Handle shift key release
        if (scancode == 0x2A || scancode == 0x36) {  // Left or right shift
            shift_pressed = 0;
        }
    } else {
        // Handle key presses
        
        // Handle special keys
        if (scancode == 0x2A || scancode == 0x36) {  // Left or right shift
            shift_pressed = 1;
        } else if (scancode == 0x3A) {  // Caps lock
            caps_lock = !caps_lock;
        } else if (scancode < sizeof(scancode_to_ascii)) {
            // Convert scancode to ASCII
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
                } else if (ascii == 'g' || ascii == 'G') {
                    // Special handling for 'g' - switch to VESA mode
                    kprintf("\nSwitching to VESA mode...\n");
                    
                    if (vga_set_vesa_mode_1024x768x24() == 0) {
                        // Mode set successfully, blue screen is drawn automatically
                        // We're now in graphics mode - no more text output will be visible
                    } else {
                        kprintf("VESA mode failed, halting...\n");
                        for (;;) { __asm__ __volatile__("hlt"); }
                    }
                } else {
                    kputchar(ascii);
                }
            }
        }
    }
}

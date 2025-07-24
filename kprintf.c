#include <stdarg.h>
#include "kprintf.h"

// Helper function declarations
static void kprint_number(int num, int base, int uppercase);
static void kprint_unsigned(unsigned int num, int base, int uppercase);
static void kprint_number_padded(int num, int base, int padding);
static void kprint_unsigned_padded(unsigned int num, int base, int uppercase, int padding);

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static int kprint_x = 0;
static int kprint_y = 0;

// Function to scroll the screen up by one line
static void scroll_screen(void) {
    volatile char *video = (char*)0xb8000;
    
    // Move all lines up by one line
    for (int line = 0; line < VGA_HEIGHT - 1; line++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            int dest_pos = (line * VGA_WIDTH + col) * 2;
            int src_pos = ((line + 1) * VGA_WIDTH + col) * 2;
            
            video[dest_pos] = video[src_pos];         // Character
            video[dest_pos + 1] = video[src_pos + 1]; // Attribute
        }
    }
    
    // Clear the last line
    for (int col = 0; col < VGA_WIDTH; col++) {
        int pos = ((VGA_HEIGHT - 1) * VGA_WIDTH + col) * 2;
        video[pos] = ' ';      // Space character
        video[pos + 1] = 0x07; // Attribute (light gray on black)
    }
}

// Function to set hardware cursor position
static void update_hardware_cursor(void) {
    unsigned short position = (kprint_y * VGA_WIDTH) + kprint_x;
    
    // Send the high byte
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x0E), "Nd"((unsigned short)0x3D4));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)(position >> 8)), "Nd"((unsigned short)0x3D5));
    
    // Send the low byte
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x0F), "Nd"((unsigned short)0x3D4));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)(position & 0xFF)), "Nd"((unsigned short)0x3D5));
}

void kputchar(char c) {
    volatile char *video = (char*)0xb8000;
    
    if (c == '\n') {
        kprint_x = 0;
        kprint_y++;
    } else if (c == '\b') {
        // Handle backspace
        if (kprint_x > 0) {
            kprint_x--;
        } else if (kprint_y > 0) {
            // Move to end of previous line
            kprint_y--;
            kprint_x = VGA_WIDTH - 1;
        }
        // Clear the character at the new cursor position
        int pos = (kprint_y * VGA_WIDTH + kprint_x) * 2;
        video[pos] = ' ';      // Replace with space
        video[pos + 1] = 0x07; // Keep same attribute
    } else {
        int pos = (kprint_y * VGA_WIDTH + kprint_x) * 2;
        video[pos] = c;
        video[pos + 1] = 0x07;
        kprint_x++;
        if (kprint_x >= VGA_WIDTH) {
            kprint_x = 0;
            kprint_y++;
        }
    }
    if (kprint_y >= VGA_HEIGHT) {
        scroll_screen();
        kprint_y = VGA_HEIGHT - 1; // Keep cursor on the last line
    }
    
    // Update hardware cursor after each character
    update_hardware_cursor();
}

void kprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    int num = va_arg(args, int);
                    kprint_number(num, 10, 0);
                    break;
                }
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    kprint_unsigned(num, 10, 0);
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    kprint_unsigned(num, 16, 0);
                    break;
                }
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    kprint_unsigned(num, 16, 1);
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    kputchar('0');
                    kputchar('x');
                    kprint_unsigned((unsigned int)ptr, 16, 0);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    kputchar(c);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        while (*str) {
                            kputchar(*str++);
                        }
                    } else {
                        // Print "(null)" without recursion
                        kputchar('(');
                        kputchar('n');
                        kputchar('u');
                        kputchar('l');
                        kputchar('l');
                        kputchar(')');
                    }
                    break;
                }
                case '%': {
                    kputchar('%');
                    break;
                }
                case '0': {
                    // Handle zero-padding (e.g., %08x)
                    int padding = 0;
                    format++;
                    while (*format >= '0' && *format <= '9') {
                        padding = padding * 10 + (*format - '0');
                        format++;
                    }
                    if (*format == 'x') {
                        unsigned int num = va_arg(args, unsigned int);
                        kprint_unsigned_padded(num, 16, 0, padding);
                    } else if (*format == 'X') {
                        unsigned int num = va_arg(args, unsigned int);
                        kprint_unsigned_padded(num, 16, 1, padding);
                    } else if (*format == 'd') {
                        int num = va_arg(args, int);
                        kprint_number_padded(num, 10, padding);
                    }
                    break;
                }
                default:
                    kputchar('%');
                    kputchar(*format);
                    break;
            }
        } else {
            kputchar(*format);
        }
        format++;
    }
    
    va_end(args);
}

static void kprint_number(int num, int base, int uppercase) {
    if (num < 0) {
        kputchar('-');
        num = -num;
    }
    kprint_unsigned((unsigned int)num, base, uppercase);
}

static void kprint_unsigned(unsigned int num, int base, int uppercase) {
    char digits[] = "0123456789abcdef";
    char upper_digits[] = "0123456789ABCDEF";
    char* digit_set = uppercase ? upper_digits : digits;
    
    if (num == 0) {
        kputchar('0');
        return;
    }
    
    char buffer[32];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = digit_set[num % base];
        num /= base;
    }
    
    while (i > 0) {
        kputchar(buffer[--i]);
    }
}

static void kprint_number_padded(int num, int base, int padding) {
    if (num < 0) {
        kputchar('-');
        num = -num;
        padding--;
    }
    kprint_unsigned_padded((unsigned int)num, base, 0, padding);
}

static void kprint_unsigned_padded(unsigned int num, int base, int uppercase, int padding) {
    char digits[] = "0123456789abcdef";
    char upper_digits[] = "0123456789ABCDEF";
    char* digit_set = uppercase ? upper_digits : digits;
    
    char buffer[32];
    int i = 0;
    
    if (num == 0) {
        buffer[i++] = '0';
    } else {
        while (num > 0) {
            buffer[i++] = digit_set[num % base];
            num /= base;
        }
    }
    
    // Add padding
    while (i < padding) {
        buffer[i++] = '0';
    }
    
    while (i > 0) {
        kputchar(buffer[--i]);
    }
}

void kclear_screen(void) {
    volatile char *video = (char*)0xb8000;
    int i;
    
    // Clear the entire screen with spaces and reset cursor position
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        video[i] = ' ';      // Character
        video[i + 1] = 0x07; // Attribute (light gray on black)
    }
    
    // Reset cursor position
    kprint_x = 0;
    kprint_y = 0;
    
    // Update hardware cursor
    update_hardware_cursor();
}

void kset_cursor_position(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        kprint_x = x;
        kprint_y = y;
        update_hardware_cursor();
    }
}
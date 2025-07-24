#include "kprintf.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static int kprint_x = 0;
static int kprint_y = 0;

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
        kprint_y = 0; // or: implement scrolling
    }
    
    // Update hardware cursor after each character
    update_hardware_cursor();
}

void kprintf(const char *s) {
    while (*s) {
        kputchar(*s++);
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
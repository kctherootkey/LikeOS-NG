#include "idt.h"
#include "kprintf.h"
#include "keyboard.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed));

#define IDT_ENTRIES 256
struct idt_entry idt[IDT_ENTRIES];

extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);

// IRQ function declarations
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);  extern void irq3(void);
extern void irq4(void);  extern void irq5(void);  extern void irq6(void);  extern void irq7(void);
extern void irq8(void);  extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

static void idt_set_gate(int n, uint32_t handler) {
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

static inline void lidt(void* base, uint16_t size) {
    struct {
        uint16_t length;
        uint32_t base;
    } __attribute__((packed)) IDTR = { size, (uint32_t)base };
    __asm__ __volatile__("lidt %0" : : "m"(IDTR));
}

static void print_hex(uint32_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9] = "00000000";
    int i;
    
    for (i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    
    kprintf("0x");
    kprintf(buffer);
}

void idt_install(void) {
    idt_set_gate(0,  (uint32_t)isr0);   idt_set_gate(1,  (uint32_t)isr1);
    idt_set_gate(2,  (uint32_t)isr2);   idt_set_gate(3,  (uint32_t)isr3);
    idt_set_gate(4,  (uint32_t)isr4);   idt_set_gate(5,  (uint32_t)isr5);
    idt_set_gate(6,  (uint32_t)isr6);   idt_set_gate(7,  (uint32_t)isr7);
    idt_set_gate(8,  (uint32_t)isr8);   idt_set_gate(9,  (uint32_t)isr9);
    idt_set_gate(10, (uint32_t)isr10);  idt_set_gate(11, (uint32_t)isr11);
    idt_set_gate(12, (uint32_t)isr12);  idt_set_gate(13, (uint32_t)isr13);
    idt_set_gate(14, (uint32_t)isr14);  idt_set_gate(15, (uint32_t)isr15);
    idt_set_gate(16, (uint32_t)isr16);  idt_set_gate(17, (uint32_t)isr17);
    idt_set_gate(18, (uint32_t)isr18);  idt_set_gate(19, (uint32_t)isr19);
    idt_set_gate(20, (uint32_t)isr20);  idt_set_gate(21, (uint32_t)isr21);
    idt_set_gate(22, (uint32_t)isr22);  idt_set_gate(23, (uint32_t)isr23);
    idt_set_gate(24, (uint32_t)isr24);  idt_set_gate(25, (uint32_t)isr25);
    idt_set_gate(26, (uint32_t)isr26);  idt_set_gate(27, (uint32_t)isr27);
    idt_set_gate(28, (uint32_t)isr28);  idt_set_gate(29, (uint32_t)isr29);
    idt_set_gate(30, (uint32_t)isr30);  idt_set_gate(31, (uint32_t)isr31);

    lidt(idt, sizeof(idt) - 1);
}

// PIC (Programmable Interrupt Controller) ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// PIC end-of-interrupt command
#define PIC_EOI 0x20

static void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ __volatile__("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Send end-of-interrupt signal to PIC
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

// Remap PIC interrupts to avoid conflict with CPU exceptions
void pic_remap(void) {
    // Save masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Start initialization sequence
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    
    // Set vector offsets
    outb(PIC1_DATA, 32);  // IRQs 0-7 mapped to interrupts 32-39
    outb(PIC2_DATA, 40);  // IRQs 8-15 mapped to interrupts 40-47
    
    // Configure cascading
    outb(PIC1_DATA, 4);   // Tell PIC1 that PIC2 is at IRQ2
    outb(PIC2_DATA, 2);   // Tell PIC2 its cascade identity
    
    // Set mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void irq_set_mask(unsigned char irq_line) {
    uint16_t port;
    uint8_t value;
    
    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) | (1 << irq_line);
    outb(port, value);
}

void irq_clear_mask(unsigned char irq_line) {
    uint16_t port;
    uint8_t value;
    
    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);
}

void irq_install(void) {
    // Remap PIC to avoid conflicts with CPU exceptions
    pic_remap();
    
    // Install IRQ handlers in IDT
    idt_set_gate(32, (uint32_t)irq0);   idt_set_gate(33, (uint32_t)irq1);
    idt_set_gate(34, (uint32_t)irq2);   idt_set_gate(35, (uint32_t)irq3);
    idt_set_gate(36, (uint32_t)irq4);   idt_set_gate(37, (uint32_t)irq5);
    idt_set_gate(38, (uint32_t)irq6);   idt_set_gate(39, (uint32_t)irq7);
    idt_set_gate(40, (uint32_t)irq8);   idt_set_gate(41, (uint32_t)irq9);
    idt_set_gate(42, (uint32_t)irq10);  idt_set_gate(43, (uint32_t)irq11);
    idt_set_gate(44, (uint32_t)irq12);  idt_set_gate(45, (uint32_t)irq13);
    idt_set_gate(46, (uint32_t)irq14);  idt_set_gate(47, (uint32_t)irq15);
    
    // Enable timer and keyboard IRQs by default
    irq_clear_mask(0);  // Timer
    irq_clear_mask(1);  // Keyboard
}

void isr_common_stub(struct interrupt_frame* frame) {
    // Clear screen first for better visibility of the exception
    kclear_screen();
    
    switch (frame->interrupt_number) {
        case 0:
            kprintf("EXCEPTION: Division by Zero!\n");
            break;
        case 1:
            kprintf("EXCEPTION: Debug!\n");
            break;
        case 2:
            kprintf("EXCEPTION: Non-Maskable Interrupt!\n");
            break;
        case 3:
            kprintf("EXCEPTION: Breakpoint!\n");
            break;
        case 4:
            kprintf("EXCEPTION: Overflow!\n");
            break;
        case 5:
            kprintf("EXCEPTION: Bound Range Exceeded!\n");
            break;
        case 6:
            kprintf("EXCEPTION: Invalid Opcode!\n");
            break;
        case 7:
            kprintf("EXCEPTION: Device Not Available!\n");
            break;
        case 8:
            kprintf("EXCEPTION: Double Fault!\n");
            break;
        case 10:
            kprintf("EXCEPTION: Invalid TSS!\n");
            break;
        case 11:
            kprintf("EXCEPTION: Segment Not Present!\n");
            break;
        case 12:
            kprintf("EXCEPTION: Stack Segment Fault!\n");
            break;
        case 13:
            kprintf("EXCEPTION: General Protection Fault!\n");
            break;
        case 14:
            kprintf("EXCEPTION: Page Fault!\n");
            break;
        case 16:
            kprintf("EXCEPTION: Floating Point Exception!\n");
            break;
        case 17:
            kprintf("EXCEPTION: Alignment Check!\n");
            break;
        case 18:
            kprintf("EXCEPTION: Machine Check!\n");
            break;
        case 19:
            kprintf("EXCEPTION: SIMD Floating Point Exception!\n");
            break;
        default:
            kprintf("EXCEPTION: Unknown interrupt!\n");
            break;
    }
    
    // Call the panic handler which will not return
    kernel_panic(frame);
}

void irq_common_stub(struct interrupt_frame* frame) {
    // Calculate IRQ number (interrupt number - 32)
    uint8_t irq = frame->interrupt_number - 32;
    
    // Handle specific IRQs
    switch (irq) {
        case 0:
            // Timer interrupt - happens ~18.2 times per second
            // kprintf("Timer tick\n");
            break;
        case 1:
            // Keyboard interrupt
            {
                uint8_t scancode = inb(0x60);  // Read scancode from keyboard controller
                keyboard_handler(scancode);
            }
            break;
        case 14:
            // Primary ATA hard disk
            //kprintf("Primary ATA interrupt\n");
            break;
        case 15:
            // Secondary ATA hard disk
            // kprintf("Secondary ATA interrupt\n");
            break;
        default:
            // Other IRQs
            // kprintf("IRQ ");
            char irq_str[4];
            int i = 0;
            if (irq == 0) {
                irq_str[i++] = '0';
            } else {
                char temp[4];
                int j = 0;
                uint8_t num = irq;
                while (num > 0) {
                    temp[j++] = '0' + (num % 10);
                    num /= 10;
                }
                while (j > 0) {
                    irq_str[i++] = temp[--j];
                }
            }
            irq_str[i] = '\0';
            // kprintf(irq_str);
            // kprintf(" received\n");
            break;
    }
    
    // Send EOI (End Of Interrupt) to PIC
    pic_send_eoi(irq);
}

void kernel_panic(struct interrupt_frame* frame) {
    // Show system information and provide a panic screen
    kprintf("\n");
    kprintf("=== KERNEL PANIC ===\n");
    kprintf("The system has encountered a fatal error and cannot continue.\n");
    kprintf("This exception occurred in kernel mode.\n\n");
    
    kprintf("System Information:\n");
    kprintf("- Kernel: LikeOS-NG\n");
    kprintf("- Architecture: x86 (32-bit)\n");
    kprintf("- Exception Number: ");
    
    // Print the interrupt number
    char num_str[12];
    int i = 0;
    uint32_t num = frame->interrupt_number;
    if (num == 0) {
        num_str[i++] = '0';
    } else {
        char temp[12];
        int j = 0;
        while (num > 0) {
            temp[j++] = '0' + (num % 10);
            num /= 10;
        }
        while (j > 0) {
            num_str[i++] = temp[--j];
        }
    }
    num_str[i] = '\0';
    kprintf(num_str);
    kprintf("\n");
    
    // Show fault address (EIP where exception occurred)
    kprintf("- Fault Address (EIP): ");
    print_hex(frame->eip);
    kprintf("\n");
    
    // Show error code if present
    if (frame->interrupt_number == 8 || (frame->interrupt_number >= 10 && frame->interrupt_number <= 14)) {
        kprintf("- Error Code: ");
        print_hex(frame->error_code);
        kprintf("\n");
    }
    kprintf("\n");
    
    // Show register information for debugging
    show_register_dump(frame);
    
    kprintf("=== END PANIC SCREEN ===\n");
    
    // Halt the system completely - this function never returns
    kprintf("\nSystem halted.\n");
    __asm__ __volatile__(
        "cli\n"           // Disable interrupts completely
        "1: hlt\n"        // Halt processor  
        "jmp 1b"          // Infinite loop - never return
        ::: "memory"
    );
}

void show_register_dump(struct interrupt_frame* frame) {
    kprintf("Register Dump (at time of exception):\n");
    kprintf("EAX=");
    print_hex(frame->eax);
    kprintf(" EBX=");
    print_hex(frame->ebx);
    kprintf(" ECX=");
    print_hex(frame->ecx);
    kprintf(" EDX=");
    print_hex(frame->edx);
    kprintf("\n");
    
    kprintf("ESP=");
    print_hex(frame->esp);
    kprintf(" EBP=");
    print_hex(frame->ebp);
    kprintf(" ESI=");
    print_hex(frame->esi);
    kprintf(" EDI=");
    print_hex(frame->edi);
    kprintf("\n");
    
    kprintf("EIP=");
    print_hex(frame->eip);
    kprintf(" CS=");
    print_hex(frame->cs);
    kprintf(" EFLAGS=");
    print_hex(frame->eflags);
    kprintf("\n");
    
    kprintf("SS=");
    print_hex(frame->ss);
    kprintf("\n\n");
}
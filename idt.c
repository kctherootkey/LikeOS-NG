#include "idt.h"
#include "kprintf.h"

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

void isr_common_stub(uint32_t interrupt_number) {
    // Clear screen first for better visibility of the exception
    kclear_screen();
    
    switch (interrupt_number) {
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
            kprintf("EXCEPTION: Unknown interrupt ");
            // Simple number printing since we don't have full printf
            char num_str[12];
            int i = 0;
            uint32_t num = interrupt_number;
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
            kprintf("!\n");
            break;
    }
    
    // Halt the system after handling the exception
    kprintf("System halted.\n");
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
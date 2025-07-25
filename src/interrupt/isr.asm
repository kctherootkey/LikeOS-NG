; filepath: isr.asm
[BITS 32]
extern isr_common_stub
extern irq_common_stub

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0          ; Push dummy error code
    push dword %1         ; Push interrupt number
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1         ; Push interrupt number (error code already on stack)
    jmp isr_common
%endmacro

isr_common:
    pushad                ; Push all general purpose registers
    
    ; Create interrupt frame structure and pass pointer to C handler
    mov eax, esp          ; ESP now points to our register structure
    push eax              ; Pass pointer to registers as parameter
    call isr_common_stub
    add esp, 4            ; Clean up parameter
    
    popad                 ; Restore all general purpose registers
    add esp, 8            ; Clean up interrupt number and error code
    iretd                 ; Return from interrupt

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; IRQ handlers (32-47)
%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0          ; Push dummy error code
    push dword %2         ; Push IRQ number (32 + IRQ)
    jmp irq_common
%endmacro

IRQ 0, 32   ; Timer
IRQ 1, 33   ; Keyboard
IRQ 2, 34   ; Cascade (never raised)
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; Floppy disk
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; CMOS real-time clock
IRQ 9, 41   ; Free for peripherals
IRQ 10, 42  ; Free for peripherals
IRQ 11, 43  ; Free for peripherals
IRQ 12, 44  ; PS2 mouse
IRQ 13, 45  ; FPU / Coprocessor / Inter-processor
IRQ 14, 46  ; Primary ATA hard disk
IRQ 15, 47  ; Secondary ATA hard disk

irq_common:
    pushad                ; Push all general purpose registers
    
    ; Create interrupt frame structure and pass pointer to C handler
    mov eax, esp          ; ESP now points to our register structure
    push eax              ; Pass pointer to registers as parameter
    call irq_common_stub
    add esp, 4            ; Clean up parameter
    
    popad                 ; Restore all general purpose registers
    add esp, 8            ; Clean up interrupt number and error code
    iretd                 ; Return from interrupt


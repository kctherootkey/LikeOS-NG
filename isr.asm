; filepath: isr.asm
[BITS 32]
extern isr_common_stub
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    pushad
    push dword %1
    call isr_common_stub
    add esp, 4
    popad
    iretd
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    pushad
    push dword %1
    call isr_common_stub
    add esp, 4
    popad
    add esp, 4
    iretd
%endmacro

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


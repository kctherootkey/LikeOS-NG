; filepath: bootloader.asm
; Minimal 512-byte bootloader for x86, loads 32 KiB kernel to 0x8000 and jumps there (Protected Mode)
BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov byte [boot_drive], dl

    ; Output kernel info
    mov si, kernel_load_msg
    call print_string

    ; Load kernel (64 sectors) to 0x8000
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000
    mov ah, 0x02          ; BIOS: read sectors
    mov al, 64            ; 64 sectors (32 KiB)
    mov ch, 0             ; cylinder 0
    mov cl, 2             ; sector 2 (after boot sector)
    mov dh, 0             ; head 0
    mov dl, byte [boot_drive]
    int 0x13
    jc disk_error

    ; Switch to Protected Mode
    mov si, kernel_jump_msg
    call print_string

    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

; GDT for Protected Mode
gdt_start:
    dq 0x0000000000000000         ; Null descriptor
    dq 0x00cf9a000000ffff         ; Code segment
    dq 0x00cf92000000ffff         ; Data segment
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

kernel_load_msg db "Loading kernel...", 0
kernel_jump_msg db 13,10,"Jumping to kernel...", 0
disk_error_msg db 13,10,"Disk read error!", 0

boot_drive db 0

; Protected Mode Entry
protected_mode:
[BITS 32]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ;mov esi, 0x8000      ; Start address
    ;mov ecx, 16          ; Number of bytes
    ;call hexdump_pm

    ; Jump to kernel at 0x8000
    jmp 0x08:0x8000

hexdigit_table db '0123456789ABCDEF'

hexdump_pm:
    pusha
    mov edi, 0xb8000         ; VGA textmode start address
.nextbyte:
    mov al, [esi]            ; load byte
    mov ah, 0x07             ; attribute: light gray on black

    ; High nibble
    mov bl, al
    shr bl, 4
    movzx bx, bl
    mov dl, [hexdigit_table + bx]
    mov [edi], dl
    mov [edi+1], ah
    add edi, 2

    ; Low nibble
    mov bl, al
    and bl, 0x0F
    movzx bx, bl
    mov dl, [hexdigit_table + bx]
    mov [edi], dl
    mov [edi+1], ah
    add edi, 2

    ; Space character
    mov byte [edi], ' '
    mov byte [edi+1], ah
    add edi, 2

    inc esi
    loop .nextbyte
    popa
    ret

times 510-($-$$) db 0
dw 0xAA55
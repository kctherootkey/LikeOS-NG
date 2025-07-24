; filepath: bootloader.asm
; Minimaler 512-Byte-Bootloader für x86, lädt 32 KiB Kernel nach 0x100000 und springt dorthin (Protected Mode)
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

    ; Kernel-Info ausgeben
    mov si, kernel_load_msg
    call print_string

    ; Kernel (64 Sektoren) nach 0x100000 laden
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000
    mov ah, 0x02          ; BIOS: Sektoren lesen
    mov al, 64            ; 64 Sektoren (32 KiB)
    mov ch, 0             ; Zylinder 0
    mov cl, 2             ; Sektor 2 (nach Bootsektor)
    mov dh, 0             ; Kopf 0
    mov dl, byte [boot_drive]
    int 0x13
    jc disk_error

    ; Umschalten auf Protected Mode
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

; GDT für Protected Mode
gdt_start:
    dq 0x0000000000000000         ; Null-Deskriptor
    dq 0x00cf9a000000ffff         ; Code-Segment
    dq 0x00cf92000000ffff         ; Data-Segment
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

    ;mov esi, 0x8000      ; Startadresse
    ;mov ecx, 16          ; Anzahl Bytes
    ;call hexdump_pm

    ; Sprung zum Kernel bei 0x100000
    jmp 0x08:0x8000

hexdigit_table db '0123456789ABCDEF'

hexdump_pm:
    pusha
    mov edi, 0xb8000         ; VGA Textmode Startadresse
.nextbyte:
    mov al, [esi]            ; Byte laden
    mov ah, 0x07             ; Attribut: hellgrau auf schwarz

    ; High-Nibble
    mov bl, al
    shr bl, 4
    movzx bx, bl
    mov dl, [hexdigit_table + bx]
    mov [edi], dl
    mov [edi+1], ah
    add edi, 2

    ; Low-Nibble
    mov bl, al
    and bl, 0x0F
    movzx bx, bl
    mov dl, [hexdigit_table + bx]
    mov [edi], dl
    mov [edi+1], ah
    add edi, 2

    ; Leerzeichen
    mov byte [edi], ' '
    mov byte [edi+1], ah
    add edi, 2

    inc esi
    loop .nextbyte
    popa
    ret

times 510-($-$$) db 0
dw 0xAA55
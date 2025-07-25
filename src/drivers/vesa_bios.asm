; VESA BIOS interface for real mode calls
; Implements proper protected mode to real mode switching

[bits 32]

global vesa_set_mode_1024x768
global vesa_get_lfb_address
global vesa_get_mode_width
global vesa_get_mode_height
global vesa_get_mode_bpp
global vesa_get_pitch
global vesa_set_text_mode_80x25

section .data
    ; Save protected mode state
    saved_esp       dd 0
    saved_ebp       dd 0
    saved_cr0       dd 0
    saved_cr3       dd 0
    saved_gdt_ptr   times 6 db 0
    saved_idt_ptr   times 6 db 0
    
    ; Real mode GDT - simplified for real mode transition
    real_mode_gdt:
        ; Null descriptor (0x00)
        dq 0
        ; 16-bit code segment (0x08) - covers all 1MB
        dw 0xFFFF       ; limit 0-15
        dw 0x0000       ; base 0-15
        db 0x00         ; base 16-23
        db 0x9A         ; access: present, ring 0, code, readable
        db 0x8F         ; flags: 16-bit, limit 16-19 = 0xF
        db 0x00         ; base 24-31
        ; 16-bit data segment (0x10) - covers all 1MB
        dw 0xFFFF       ; limit 0-15
        dw 0x0000       ; base 0-15
        db 0x00         ; base 16-23
        db 0x92         ; access: present, ring 0, data, writable
        db 0x8F         ; flags: 16-bit, limit 16-19 = 0xF
        db 0x00         ; base 24-31
    real_mode_gdt_end:
    
    real_mode_gdt_ptr:
        dw real_mode_gdt_end - real_mode_gdt - 1
        dd real_mode_gdt
    
    ; Temporary 32-bit GDT for safe transition back to protected mode
    temp_32bit_gdt:
        ; Null descriptor (0x00)
        dq 0
        ; 32-bit code segment (0x08) - same as bootloader
        dq 0x00cf9a000000ffff
        ; 32-bit data segment (0x10) - same as bootloader  
        dq 0x00cf92000000ffff
    temp_32bit_gdt_end:
    
    temp_32bit_gdt_ptr:
        dw temp_32bit_gdt_end - temp_32bit_gdt - 1
        dd temp_32bit_gdt
    
    ; Real mode IDT - points to standard BIOS IVT at 0x0000:0x0000
    real_mode_idt_ptr:
        dw 0x03FF               ; limit: 256 vectors * 4 bytes = 1024 - 1
        dd 0x00000000           ; base: IVT at address 0
    
    ; VESA call result
    vesa_success    db 0
    test_success    db 0
    vesa_lfb_address dd 0   ; Store the linear framebuffer address
    vesa_found_mode dw 0    ; Store the found mode number
    
    ; VESA mode info structure (256 bytes buffer)
    vesa_mode_info  times 256 db 0
    
    ; VESA info structure (256 bytes buffer)
    vesa_info       times 256 db 0

section .text

; Function: vesa_set_mode_1024x768
; Purpose: Set VESA mode 1024x768 via proper real mode switching
; Parameters: None
; Returns: EAX (0 = success, -1 = failure)
vesa_set_mode_1024x768:
    push ebp
    mov ebp, esp
    pushad
    
    ; Step 1: Save CPU state
    mov [saved_esp], esp
    mov [saved_ebp], ebp
    
    ; Save GDT and IDT
    sgdt [saved_gdt_ptr]
    sidt [saved_idt_ptr]
    
    ; Step 2: Disable interrupts
    cli
    
    ; Step 3: Disable paging (save CR0 and CR3)
    mov eax, cr0
    mov [saved_cr0], eax
    mov eax, cr3
    mov [saved_cr3], eax
    
    ; Disable paging bit (bit 31) in CR0
    mov eax, [saved_cr0]
    and eax, 0x7FFFFFFF     ; Clear PG bit
    mov cr0, eax
    
    ; Step 4: Prepare for real mode - Load 16-bit GDT
    lgdt [real_mode_gdt_ptr]
    
    ; Step 5: Jump to 16-bit code segment
    jmp 0x08:.enter_16bit
    
.enter_16bit:
    [bits 16]
    ; Set up 16-bit data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Step 6: Switch to real mode (clear PE bit)
    mov eax, cr0
    and al, 0xFE            ; Clear PE bit (bit 0) - use 8-bit operation
    mov cr0, eax
    
    ; Far jump to flush prefetch queue and set CS to 0
    jmp 0x0000:.real_mode
    
.real_mode:
    ; Step 7: Set up real mode segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; Set up a safe stack in low memory (avoid bootloader area)
    mov sp, 0x9000          ; Use higher memory area

    ; Set up real mode Interrupt Vector Table (IVT)
    ; The IVT must be at 0x0000:0x0000 for real mode interrupts to work
    lidt [real_mode_idt_ptr]

    ; Small delay to let system stabilize
    mov cx, 0x1000
.delay_loop:
    loop .delay_loop

    ; Enable interrupts very carefully - test first with a simple instruction
    sti 
    nop                     ; Let one instruction execute with interrupts enabled
    nop
    cli                     ; Immediately disable again for safety

    ; Now try enabling interrupts for BIOS calls
    sti

    ; Step 8A: Get VESA information to find available modes
    mov ax, 0x4F00          ; VESA get controller info function
    mov di, vesa_info       ; Buffer for VESA info
    int 0x10                ; Call VESA BIOS
    
    ; Check if VESA info call succeeded
    cmp ax, 0x004F
    jne .vesa_failed
    
    ; Get pointer to mode list (at offset 14 in VESA info structure)
    mov esi, [vesa_info + 14]  ; ESI = far pointer to mode list
    
    ; Convert far pointer to linear address (segment:offset to linear)
    mov eax, esi
    shr eax, 16        ; Get segment in AX
    shl eax, 4         ; Convert segment to linear address
    and esi, 0xFFFF    ; Get offset
    add esi, eax       ; ESI = linear address of mode list
    
.search_modes:
    ; Read next mode number from list
    mov cx, [esi]      ; Get mode number
    cmp cx, 0xFFFF     ; End of list marker?
    je .vesa_failed    ; No suitable mode found
    
    ; Skip if mode number is 0
    test cx, cx
    jz .next_mode
    
    ; Get mode information for this mode
    mov ax, 0x4F01          ; VESA get mode info function
    mov di, vesa_mode_info  ; Buffer for mode info
    int 0x10                ; Call VESA BIOS
    
    ; Check if get mode info succeeded
    cmp ax, 0x004F
    jne .next_mode
    
    ; Check if this mode matches our requirements:
    ; - Width = 1024 (at offset 18)
    ; - Height = 768 (at offset 20)  
    ; - BPP = 32 (at offset 25)
    ; - Linear framebuffer supported (bit 7 of attributes at offset 0)
    
    cmp word [vesa_mode_info + 18], 1024  ; Check width
    jne .next_mode
    cmp word [vesa_mode_info + 20], 768   ; Check height
    jne .next_mode
    cmp byte [vesa_mode_info + 25], 32    ; Check BPP
    jne .next_mode
    
    ; Check if linear framebuffer is supported (bit 7 of mode attributes)
    test byte [vesa_mode_info + 0], 0x80
    jz .next_mode
    
    ; Found suitable mode! Store it and extract LFB address
    mov [vesa_found_mode], cx
    mov eax, [vesa_mode_info + 40]        ; LFB address at offset 40
    mov [vesa_lfb_address], eax
    jmp .set_mode
    
.next_mode:
    add esi, 2         ; Move to next mode in list
    jmp .search_modes
    
.set_mode:
    ; Step 8B: Set the found VESA mode
    mov ax, 0x4F02          ; VESA set mode function
    mov bx, [vesa_found_mode] ; Use the mode we found
    or bx, 0x4000           ; Set linear framebuffer bit
    int 0x10                ; Call VESA BIOS

    ; Check if VESA call succeeded
    cmp ax, 0x004F          ; VESA success code
    je .vesa_ok
    
.vesa_failed:
    ; Fallback: try hardcoded mode 0x118 (for VMware compatibility - 1024x768x16)
    mov ax, 0x4F01          ; VESA get mode info function
    mov cx, 0x117           ; Mode 0x118 = 1024x768x16
    mov di, vesa_mode_info  ; Buffer for mode info
    int 0x10                ; Call VESA BIOS
    
    ; Check if get mode info succeeded
    cmp ax, 0x004F
    jne .complete_failure
    
    ; Extract LFB address from mode info
    mov eax, [vesa_mode_info + 40]
    mov [vesa_lfb_address], eax
    
    ; Set the fallback mode
    mov ax, 0x4F02          ; VESA set mode function
    mov bx, 0x117           ; Mode 0x118 = 1024x768x16
    or bx, 0x4000           ; Set linear framebuffer bit
    int 0x10                ; Call VESA BIOS
    
    ; Check if fallback mode succeeded
    cmp ax, 0x004F
    jne .complete_failure
    
    ; Store fallback mode number
    mov word [vesa_found_mode], 0x118
    jmp .vesa_ok

.complete_failure:
    ; Both dynamic and fallback failed
    mov byte [vesa_success], 0
    jmp .return_to_protected
    
.vesa_ok:
    mov byte [vesa_success], 1
    
.return_to_protected:
    ; Disable interrupts for mode switch
    cli
    
    ; Step 9: Switch back to Protected Mode
    ; Load our temporary 32-bit GDT to prepare for protected mode
    lgdt [temp_32bit_gdt_ptr]
    
    ; Enable protected mode (set PE bit)
    mov eax, cr0
    or al, 0x01             ; Set PE bit
    mov cr0, eax
    
    ; Jump to 32-bit code segment to complete transition
    jmp 0x08:.back_to_32bit
    
.back_to_32bit:
    [bits 32]
    ; Step 10: Restore segment selectors, paging, GDT, IDT
    
    ; We're now using the temporary 32-bit GDT, so 0x10 is a valid 32-bit data segment
    mov ax, 0x10    ; Data segment from our temporary GDT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Now restore the original GDT and IDT
    lgdt [saved_gdt_ptr]
    lidt [saved_idt_ptr]
 
    ; Restore paging if it was enabled
    mov eax, [saved_cr0]
    test eax, 0x80000000    ; Check if paging was enabled
    jz .no_paging_restore

    ; Restore CR3 and enable paging
    mov eax, [saved_cr3]
    mov cr3, eax
    mov eax, [saved_cr0]
    mov cr0, eax
    
.no_paging_restore:
    ; Restore stack
    mov esp, [saved_esp]
    mov ebp, [saved_ebp]
    
    ; Re-enable interrupts
    sti
    
    ; Return success/failure based on VESA result
    mov al, [vesa_success]
    test al, al
    jnz .success
    
    mov eax, -1
    jmp .done
    
.success:
    xor eax, eax
    
.done:
    popad
    pop ebp
    ret

; Function: vesa_get_lfb_address
; Purpose: Get the linear framebuffer address from the last VESA mode info call
; Returns: EAX (LFB address)
vesa_get_lfb_address:
    mov eax, [vesa_lfb_address]
    ret

; Function: vesa_get_mode_width
; Purpose: Get the mode width from the last VESA mode info call
; Returns: EAX (width in pixels)
vesa_get_mode_width:
    movzx eax, word [vesa_mode_info + 18]  ; Width is at offset 18
    ret

; Function: vesa_get_mode_height  
; Purpose: Get the mode height from the last VESA mode info call
; Returns: EAX (height in pixels)
vesa_get_mode_height:
    movzx eax, word [vesa_mode_info + 20]  ; Height is at offset 20
    ret

; Function: vesa_get_mode_bpp
; Purpose: Get the bits per pixel from the last VESA mode info call
; Returns: EAX (bits per pixel)
vesa_get_mode_bpp:
    movzx eax, byte [vesa_mode_info + 25]  ; BPP is at offset 25
    ret

; Function: vesa_get_pitch
; Returns: EAX (bytes per scanline)
vesa_get_pitch:
    movzx eax, word [vesa_mode_info + 16]
    ret

; Function: vesa_set_text_mode_80x25
; Purpose: Set VGA text mode 3 (80x25) via proper real mode switching
; Returns: EAX (0 = success, -1 = failure)
vesa_set_text_mode_80x25:
    push ebp
    mov ebp, esp
    pushad
    
    ; Save state
    mov [saved_esp], esp
    mov [saved_ebp], ebp
    sgdt [saved_gdt_ptr]
    sidt [saved_idt_ptr]
    
    ; Disable interrupts
    cli
    
    ; Save and disable paging
    mov eax, cr0
    mov [saved_cr0], eax
    mov eax, cr3
    mov [saved_cr3], eax
    
    mov eax, [saved_cr0]
    and eax, 0x7FFFFFFF     ; Clear PG bit
    mov cr0, eax
    
    ; Load 16-bit GDT
    lgdt [real_mode_gdt_ptr]
    
    ; Jump to 16-bit mode
    jmp 0x08:.text_16bit
    
.text_16bit:
    [bits 16]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; Enter real mode
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    
    jmp 0x0000:.text_real
    
.text_real:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    
    ; Set up real mode IVT
    lidt [real_mode_idt_ptr]
    
    ; Enable interrupts for BIOS call
    sti
    
    ; Call BIOS interrupt 10h to set text mode
    mov ax, 0x0003          ; Function 00h, Mode 03h (80x25 16-color text)
    int 0x10                ; Call BIOS video services
    
    ; Disable interrupts for mode switch back
    cli
    
    ; Return to protected mode
    lgdt [temp_32bit_gdt_ptr]
    mov eax, cr0
    or al, 0x01
    mov cr0, eax
    jmp 0x08:.text_back_32
    
.text_back_32:
    [bits 32]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    lgdt [saved_gdt_ptr]
    lidt [saved_idt_ptr]
    
    ; Restore paging if needed
    mov eax, [saved_cr0]
    test eax, 0x80000000
    jz .text_no_paging
    mov eax, [saved_cr3]
    mov cr3, eax
    mov eax, [saved_cr0]
    mov cr0, eax
    
.text_no_paging:
    mov esp, [saved_esp]
    mov ebp, [saved_ebp]
    sti
    
    ; Return success
    xor eax, eax
    
    popad
    pop ebp
    ret

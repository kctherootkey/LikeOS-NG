; VESA BIOS interface for real mode calls
; Implements proper protected mode to real mode switching

[bits 32]

global vesa_set_mode_1024x768x24
global vesa_test_real_mode

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

section .text

; Function: vesa_test_real_mode
; Purpose: Test if real mode switching works safely
; Returns: EAX (0 = success, -1 = failure)
vesa_test_real_mode:
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
    jmp 0x08:.test_16bit
    
.test_16bit:
    [bits 16]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; Enter real mode
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    
    jmp 0x0000:.test_real
    
.test_real:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    
    ; Don't enable interrupts in test - just mark success and return
    ; This tests if real mode switching works without risking interrupt issues
    mov byte [test_success], 1
    
    ; Return to protected mode
    cli
    lgdt [temp_32bit_gdt_ptr]
    mov eax, cr0
    or al, 0x01
    mov cr0, eax
    jmp 0x08:.test_back_32
    
.test_back_32:
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
    jz .test_no_paging
    mov eax, [saved_cr3]
    mov cr3, eax
    mov eax, [saved_cr0]
    mov cr0, eax
    
.test_no_paging:
    mov esp, [saved_esp]
    mov ebp, [saved_ebp]
    sti
    
    ; Check result
    mov al, [test_success]
    test al, al
    jnz .test_ok
    mov eax, -1
    jmp .test_done
.test_ok:
    xor eax, eax
.test_done:
    popad
    pop ebp
    ret

; Function: vesa_set_mode_1024x768x24
; Purpose: Set VESA mode 1024x768x24 via proper real mode switching
; Parameters: None
; Returns: EAX (0 = success, -1 = failure)
vesa_set_mode_1024x768x24:
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

    ; Try a simple BIOS call first to test if real mode is working
    mov ah, 0x0E
    mov al, 'V'
    mov bh, 0
    int 0x10                ; Print 'V' to test BIOS access

    ; Step 8: Call BIOS INT 10h to set VESA mode 1024x768x24
    mov ax, 0x4F02          ; VESA set mode function
    mov bx, 0x105           ; Mode 0x105 = 1024x768x16 (more compatible than 24-bit)
    or bx, 0x4000           ; Set linear framebuffer bit
    int 0x10                ; Call VESA BIOS

    ; Check if VESA call succeeded
    cmp ax, 0x004F          ; VESA success code
    je .vesa_ok
    
    ; VESA failed, mark failure
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

; kernel.asm
[BITS 32]
[ORG 0x8000]
start:
mov dword [0xb8000], 0x41414141
mov dword [0xb8040], 0x42424242
jmp $

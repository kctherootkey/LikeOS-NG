# Tools
AS=nasm
CC=gcc
LD=ld

# Files
BOOTLOADER=bootloader.asm
BOOTLOADER_BIN=bootloader.bin
KERNEL_SRC=kernel.c
KERNEL_OBJ=kernel.o
ISR_OBJ=isr.o
KERNEL_BIN=kernel.bin
KERNEL_BIN_PADDED=kernel_padded.bin
KERNEL_LD=kernel.ld
OS_IMG=os.img
KPRINTF_OBJ=kprintf.o
IDT_OBJ=idt.o
KEYBOARD_OBJ=keyboard.o
PAGING_OBJ=paging.o

.PHONY: all clean run

all: $(OS_IMG)

$(BOOTLOADER_BIN): $(BOOTLOADER)
	$(AS) -f bin $(BOOTLOADER) -o $(BOOTLOADER_BIN)

$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) -m32 -ffreestanding -c $(KERNEL_SRC) -o $(KERNEL_OBJ)

$(ISR_OBJ): isr.asm
	$(AS) -f elf32 isr.asm -o $(ISR_OBJ)

$(KPRINTF_OBJ): kprintf.c kprintf.h
	$(CC) -m32 -ffreestanding -c kprintf.c -o kprintf.o

$(IDT_OBJ): idt.c idt.h
	$(CC) -m32 -ffreestanding -c idt.c -o idt.o

$(KEYBOARD_OBJ): keyboard.c keyboard.h
	$(CC) -m32 -ffreestanding -c keyboard.c -o keyboard.o

$(PAGING_OBJ): paging.c paging.h
	$(CC) -m32 -ffreestanding -c paging.c -o paging.o

$(KERNEL_BIN): $(KERNEL_OBJ) $(KPRINTF_OBJ) $(IDT_OBJ) $(KEYBOARD_OBJ) $(PAGING_OBJ) $(ISR_OBJ) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) $(KERNEL_OBJ) $(KPRINTF_OBJ) $(IDT_OBJ) $(KEYBOARD_OBJ) $(PAGING_OBJ) $(ISR_OBJ) -o kernel.elf -nostdlib
	objcopy -O binary kernel.elf $(KERNEL_BIN)

$(KERNEL_BIN_PADDED): $(KERNEL_BIN)
	dd if=/dev/zero bs=512 count=64 of=$(KERNEL_BIN_PADDED)
	dd if=$(KERNEL_BIN) of=$(KERNEL_BIN_PADDED) conv=notrunc

$(OS_IMG): $(BOOTLOADER_BIN) $(KERNEL_BIN_PADDED)
	cat $(BOOTLOADER_BIN) $(KERNEL_BIN_PADDED) > $(OS_IMG)

run: $(OS_IMG)
	qemu-system-x86_64 -drive format=raw,file=$(OS_IMG)

clean:
	rm -f *.o *.bin *.img kernel_padded.bin kernel.elf
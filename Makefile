# Tools
AS=nasm
CC=gcc
LD=ld

# Directories
SRC_DIR=src
BOOT_DIR=$(SRC_DIR)/boot
KERNEL_DIR=$(SRC_DIR)/kernel
MEMORY_DIR=$(SRC_DIR)/memory
INTERRUPT_DIR=$(SRC_DIR)/interrupt
DRIVERS_DIR=$(SRC_DIR)/drivers
LIB_DIR=$(SRC_DIR)/lib
INCLUDE_DIR=include

# Include paths
INCLUDES=-I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/memory -I$(INCLUDE_DIR)/interrupt -I$(INCLUDE_DIR)/drivers -I$(INCLUDE_DIR)/lib

# Compiler flags
CFLAGS=-m32 -ffreestanding $(INCLUDES) -Wall -Wextra

# Files
BOOTLOADER=$(BOOT_DIR)/bootloader.asm
BOOTLOADER_BIN=bootloader.bin
KERNEL_SRC=$(KERNEL_DIR)/kernel.c
KERNEL_OBJ=kernel.o
ISR_OBJ=isr.o
KERNEL_BIN=kernel.bin
KERNEL_BIN_PADDED=kernel_padded.bin
KERNEL_LD=kernel.ld
OS_IMG=os.img
KPRINTF_OBJ=kprintf.o
TIMING_OBJ=timing.o
IDT_OBJ=idt.o
KEYBOARD_OBJ=keyboard.o
PAGING_OBJ=paging.o
PMM_OBJ=pmm.o
VGA_OBJ=vga.o
VESA_BIOS_OBJ=vesa_bios.o

.PHONY: all clean run

all: $(OS_IMG)

$(BOOTLOADER_BIN): $(BOOTLOADER)
	$(AS) -f bin $(BOOTLOADER) -o $(BOOTLOADER_BIN)

$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) $(CFLAGS) -c $(KERNEL_SRC) -o $(KERNEL_OBJ)

$(ISR_OBJ): $(INTERRUPT_DIR)/isr.asm
	$(AS) -f elf32 $(INTERRUPT_DIR)/isr.asm -o $(ISR_OBJ)

$(KPRINTF_OBJ): $(LIB_DIR)/kprintf.c $(INCLUDE_DIR)/lib/kprintf.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/kprintf.c -o $(KPRINTF_OBJ)

$(TIMING_OBJ): $(LIB_DIR)/timing.c $(INCLUDE_DIR)/lib/timing.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/timing.c -o $(TIMING_OBJ)

$(IDT_OBJ): $(INTERRUPT_DIR)/idt.c $(INCLUDE_DIR)/interrupt/idt.h
	$(CC) $(CFLAGS) -c $(INTERRUPT_DIR)/idt.c -o $(IDT_OBJ)

$(KEYBOARD_OBJ): $(DRIVERS_DIR)/keyboard.c $(INCLUDE_DIR)/drivers/keyboard.h
	$(CC) $(CFLAGS) -c $(DRIVERS_DIR)/keyboard.c -o $(KEYBOARD_OBJ)

$(PAGING_OBJ): $(MEMORY_DIR)/paging.c $(INCLUDE_DIR)/memory/paging.h
	$(CC) $(CFLAGS) -c $(MEMORY_DIR)/paging.c -o $(PAGING_OBJ)

$(PMM_OBJ): $(MEMORY_DIR)/pmm.c $(INCLUDE_DIR)/memory/pmm.h
	$(CC) $(CFLAGS) -c $(MEMORY_DIR)/pmm.c -o $(PMM_OBJ)

$(VGA_OBJ): $(DRIVERS_DIR)/vga.c $(INCLUDE_DIR)/drivers/vga.h
	$(CC) $(CFLAGS) -c $(DRIVERS_DIR)/vga.c -o $(VGA_OBJ)

$(VESA_BIOS_OBJ): $(DRIVERS_DIR)/vesa_bios.asm
	$(AS) -f elf32 $(DRIVERS_DIR)/vesa_bios.asm -o $(VESA_BIOS_OBJ)

$(KERNEL_BIN): $(KERNEL_OBJ) $(KPRINTF_OBJ) $(TIMING_OBJ) $(IDT_OBJ) $(KEYBOARD_OBJ) $(PAGING_OBJ) $(PMM_OBJ) $(VGA_OBJ) $(VESA_BIOS_OBJ) $(ISR_OBJ) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) $(KERNEL_OBJ) $(KPRINTF_OBJ) $(TIMING_OBJ) $(IDT_OBJ) $(KEYBOARD_OBJ) $(PAGING_OBJ) $(PMM_OBJ) $(VGA_OBJ) $(VESA_BIOS_OBJ) $(ISR_OBJ) -o kernel.elf -nostdlib
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
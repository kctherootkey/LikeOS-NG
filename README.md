# Changelog

All notable changes to LikeOS-NG will be documented in this file.

## [Enhanced Version] - 2025-07-24

### Added
- **PAE Paging Support**
  - Physical Address Extension for 64GB memory addressing capability
  - 64-bit page table entries with NX bit support
  - Page Directory Pointer Table (PDPT) implementation
  - Identity mapping for first 32MB of memory
  - Proper page alignment and validation

- **Advanced Memory Management**
  - Free list-based physical page allocator
  - Memory statistics tracking (allocated/freed pages)
  - Automatic page zeroing on allocation
  - Page-aligned memory allocation validation
  - Memory layout constants adjusted for actual kernel location (0x8000)

- **Enhanced Interrupt Handling**
  - Complete IDT setup for all 32 CPU exceptions
  - Detailed page fault handler with CR2 register analysis
  - Error code decoding for page faults (present/read/write/user/kernel mode)
  - Enhanced kernel panic screen with register dump
  - Comprehensive exception descriptions

- **IRQ and PIC Support**
  - Complete PIC (8259) interrupt controller support
  - IRQ remapping to avoid conflicts with CPU exceptions
  - Proper End-of-Interrupt (EOI) handling
  - IRQ mask/unmask functionality
  - Timer and keyboard IRQ support

- **PS/2 Keyboard Driver**
  - Complete US QWERTY scancode-to-ASCII conversion
  - Shift key support for uppercase and special characters
  - Caps Lock functionality
  - Key press and release detection
  - Proper backspace handling

- **Advanced Display System**
  - Printf-style formatted output with kprintf
  - Support for format specifiers: %d, %u, %x, %X, %p, %c, %s, %%
  - Zero-padding support (e.g., %08x)
  - Hardware cursor synchronization
  - Automatic screen scrolling when text exceeds screen boundaries
  - Backspace functionality that properly removes characters

- **Enhanced Build System**
  - Updated Makefile with proper dependencies
  - Clean target now removes kernel.elf file
  - Modular compilation for all components
  - Proper linking with custom linker script

### Changed
- **Memory Layout Corrections**
  - Fixed kernel start address from 0x100000 to 0x8000 to match linker script
  - Corrected bootloader comments to reflect actual load address
  - Updated heap layout to start at 0x20000 (128KB)
  - Extended identity mapping from 4MB to 32MB

- **Code Organization**
  - Separated keyboard functionality into dedicated keyboard.c/keyboard.h
  - Modularized kprintf into separate kprintf.c/kprintf.h files
  - Moved paging functionality to paging.c/paging.h
  - Enhanced IDT handling in idt.c/idt.h

- **Error Handling Improvements**
  - Replaced simple print_hex calls with formatted kprintf
  - Enhanced exception messages with detailed context
  - Added register dump functionality for debugging
  - Improved kernel panic screen layout

- **Language Improvements**
  - Translated German comments to English throughout codebase
  - Standardized code formatting and documentation
  - Added comprehensive function documentation

### Fixed
- **Linker Issues**
  - Resolved undefined reference to 'isr_common_stub'
  - Fixed missing function declarations
  - Corrected function parameter names in interrupt handlers

- **Paging Issues**
  - Fixed page fault at 0x1411000 by extending identity mapping
  - Resolved PDPT alignment issues
  - Fixed page table allocation using proper free list

- **Display Issues**
  - Fixed cursor positioning problems
  - Resolved screen wrapping behavior
  - Corrected VGA memory access patterns

- **Input Issues**
  - Fixed backspace showing "<BS>" instead of removing characters
  - Corrected scancode handling for special keys
  - Fixed shift key state management

### Technical Details

#### Memory Management
- Implemented proper free list with linked pages
- Added memory statistics: `get_memory_stats()`
- Free list initialization with 1024 pages (4MB) pool
- Automatic expansion when free list is exhausted

#### Paging Implementation
- PAE-compatible page structures with 8-byte entries
- Virtual address breakdown: 2-bit PDPT + 9-bit PD + 9-bit PT + 12-bit offset
- Complete page mapping/unmapping functionality
- TLB invalidation for modified pages

#### Interrupt Architecture
- 32 CPU exception handlers (ISR 0-31)
- 16 hardware interrupt handlers (IRQ 0-15)
- Proper stack frame handling in assembly stubs
- C-based interrupt processing with full context

#### Keyboard System
- Scancode tables for normal and shifted characters
- State tracking for modifier keys
- ASCII conversion with caps lock support
- Integration with kprintf for character output

### Development Process
This enhanced version represents a complete overhaul of the original kernel by kingcope, transforming it from a basic proof-of-concept into a functional operating system kernel with modern features suitable for educational use and virtualization environments.

Key development areas included:
1. Memory management modernization
2. Interrupt handling robustness
3. Input/output system implementation
4. Display and terminal functionality
5. Code organization and maintainability

---

## [Original Version] - Creator: kingcope

### Initial Implementation
- Basic x86 32-bit kernel framework
- Simple bootloader for protected mode entry
- Minimal VGA text output
- Basic exception handling structure
- Foundation code architecture

*Original creator: kingcope - provided the foundational kernel structure and bootloader implementation that served as the base for all subsequent enhancements.*

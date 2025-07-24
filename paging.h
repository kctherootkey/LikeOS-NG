#pragma once
#include <stdint.h>

// PAE paging structures
#define PAGE_SIZE 4096
#define ENTRIES_PER_TABLE 512
#define PDPT_ENTRIES 4

// Page flags
#define PAGE_PRESENT    0x001
#define PAGE_WRITABLE   0x002
#define PAGE_USER       0x004
#define PAGE_WRITE_THROUGH 0x008
#define PAGE_CACHE_DISABLE 0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_SIZE_FLAG  0x080  // For 2MB pages
#define PAGE_GLOBAL     0x100
#define PAGE_NX         0x8000000000000000ULL  // No-execute bit (bit 63)

// PAE structures (8-byte entries)
typedef uint64_t pae_entry_t;

// Page Directory Pointer Table (PDPT)
typedef struct {
    pae_entry_t entries[PDPT_ENTRIES];
} __attribute__((packed, aligned(32))) pdpt_t;

// Page Directory
typedef struct {
    pae_entry_t entries[ENTRIES_PER_TABLE];
} __attribute__((packed, aligned(PAGE_SIZE))) page_directory_t;

// Page Table
typedef struct {
    pae_entry_t entries[ENTRIES_PER_TABLE];
} __attribute__((packed, aligned(PAGE_SIZE))) page_table_t;

// Virtual address breakdown for PAE
typedef union {
    uint32_t raw;
    struct {
        uint32_t offset : 12;    // Bits 0-11: Offset within page
        uint32_t pt_index : 9;   // Bits 12-20: Page table index
        uint32_t pd_index : 9;   // Bits 21-29: Page directory index
        uint32_t pdpt_index : 2; // Bits 30-31: PDPT index
    } __attribute__((packed));
} virtual_addr_t;

// Physical memory management
typedef struct {
    uint32_t total_memory;
    uint32_t used_memory;
    uint32_t free_memory;
    uint8_t* bitmap;
    uint32_t bitmap_size;
} physical_memory_manager_t;

// Function declarations
void paging_init(void);
void enable_pae_paging(void);
int map_page(uint32_t virtual_addr, uint64_t physical_addr, uint32_t flags);
int unmap_page(uint32_t virtual_addr);
uint64_t get_physical_addr(uint32_t virtual_addr);
void* allocate_physical_page(void);
void free_physical_page(void* page);
void setup_identity_mapping(void);
void setup_kernel_heap(void);

// Helper function (should be in kprintf.h but avoiding circular dependencies)
void print_hex(uint32_t value);

#pragma once
#include <stdint.h>

// Physical Memory Manager (PMM) definitions
#define PMM_PAGE_SIZE 4096
#define PMM_PAGES_PER_BITMAP_ENTRY 32
#define PMM_BITMAP_ENTRY_SIZE 4  // 32 bits per entry

// Memory regions
#define PMM_KERNEL_START    0x8000      // Kernel starts at 32KB
#define PMM_KERNEL_END      0x20000     // Kernel ends at 128KB
#define PMM_HEAP_START      0x20000     // Heap starts at 128KB
#define PMM_HEAP_SIZE       0x1000000   // 16MB heap
#define PMM_MANAGED_START   0x1020000   // Managed memory starts at ~16MB
#define PMM_MANAGED_SIZE    0x1000000   // Manage 16MB initially
#define PMM_MAX_MEMORY      0x10000000  // Support up to 256MB

// PMM status codes
#define PMM_SUCCESS         0
#define PMM_ERROR_NO_MEMORY -1
#define PMM_ERROR_INVALID   -2
#define PMM_ERROR_ALIGNED   -3

// Physical memory statistics
typedef struct {
    uint32_t total_pages;
    uint32_t free_pages;
    uint32_t used_pages;
    uint32_t reserved_pages;
    uint32_t bitmap_size;
    uint32_t last_allocated_page;
} pmm_stats_t;

// Physical memory region descriptor
typedef struct {
    uint32_t start_address;
    uint32_t size;
    uint32_t type;  // 0 = reserved, 1 = available, 2 = kernel
} pmm_region_t;

// Memory region types
#define PMM_REGION_RESERVED 0
#define PMM_REGION_AVAILABLE 1
#define PMM_REGION_KERNEL   2

// Function declarations
int pmm_init(void);
void* pmm_alloc_page(void);
void* pmm_alloc_pages(uint32_t count);
int pmm_free_page(void* page);
int pmm_free_pages(void* pages, uint32_t count);
int pmm_reserve_region(uint32_t start, uint32_t size);
int pmm_mark_available(uint32_t start, uint32_t size);
pmm_stats_t pmm_get_stats(void);
void pmm_print_stats(void);
void pmm_print_memory_map(void);
uint32_t pmm_get_free_memory(void);
uint32_t pmm_get_used_memory(void);
int pmm_is_page_allocated(void* page);

// Internal functions (for debugging/testing)
void pmm_dump_bitmap(uint32_t start_page, uint32_t count);
uint32_t pmm_find_free_pages(uint32_t count);

#include "memory/pmm.h"
#include "lib/kprintf.h"
#include <stdint.h>

#define NULL ((void*)0)

// Global PMM state
static uint32_t* pmm_bitmap = NULL;
static uint32_t pmm_bitmap_size = 0;
static uint32_t pmm_total_pages = 0;
static uint32_t pmm_free_page_count = 0;
static uint32_t pmm_last_allocated = 0;
static int pmm_initialized = 0;

// Memory regions
static pmm_region_t pmm_regions[16];
static uint32_t pmm_region_count = 0;

// Convert physical address to page number
static inline uint32_t pmm_addr_to_page(uint32_t addr) {
    return addr / PMM_PAGE_SIZE;
}

// Convert page number to physical address
static inline uint32_t pmm_page_to_addr(uint32_t page) {
    return page * PMM_PAGE_SIZE;
}

// Get bitmap entry index and bit position for a page
static inline void pmm_page_to_bitmap(uint32_t page, uint32_t* entry, uint32_t* bit) {
    *entry = page / PMM_PAGES_PER_BITMAP_ENTRY;
    *bit = page % PMM_PAGES_PER_BITMAP_ENTRY;
}

// Set a bit in the bitmap (mark page as used)
static inline void pmm_set_bit(uint32_t page) {
    uint32_t entry, bit;
    pmm_page_to_bitmap(page, &entry, &bit);
    if (entry < pmm_bitmap_size) {
        pmm_bitmap[entry] |= (1U << bit);
    }
}

// Clear a bit in the bitmap (mark page as free)
static inline void pmm_clear_bit(uint32_t page) {
    uint32_t entry, bit;
    pmm_page_to_bitmap(page, &entry, &bit);
    if (entry < pmm_bitmap_size) {
        pmm_bitmap[entry] &= ~(1U << bit);
    }
}

// Test if a bit is set in the bitmap
static inline int pmm_test_bit(uint32_t page) {
    uint32_t entry, bit;
    pmm_page_to_bitmap(page, &entry, &bit);
    if (entry < pmm_bitmap_size) {
        return (pmm_bitmap[entry] & (1U << bit)) != 0;
    }
    return 1; // Assume allocated if out of range
}

// Add a memory region
static int pmm_add_region(uint32_t start, uint32_t size, uint32_t type) {
    if (pmm_region_count >= 16) {
        return PMM_ERROR_NO_MEMORY;
    }
    
    pmm_regions[pmm_region_count].start_address = start;
    pmm_regions[pmm_region_count].size = size;
    pmm_regions[pmm_region_count].type = type;
    pmm_region_count++;
    
    return PMM_SUCCESS;
}

// Initialize the physical memory manager
int pmm_init(void) {
    if (pmm_initialized) {
        return PMM_SUCCESS;
    }
    
    kprintf("PMM: Initializing Physical Memory Manager...\n");
    
    // Calculate total pages we want to manage
    pmm_total_pages = PMM_MANAGED_SIZE / PMM_PAGE_SIZE;
    
    // Calculate bitmap size (in 32-bit entries)
    pmm_bitmap_size = (pmm_total_pages + PMM_PAGES_PER_BITMAP_ENTRY - 1) / PMM_PAGES_PER_BITMAP_ENTRY;
    
    // Place bitmap at the start of managed memory
    pmm_bitmap = (uint32_t*)PMM_MANAGED_START;
    
    // Clear the bitmap (all pages initially free)
    for (uint32_t i = 0; i < pmm_bitmap_size; i++) {
        pmm_bitmap[i] = 0;
    }
    
    // Set up memory regions
    pmm_region_count = 0;
    
    // Add kernel region (reserved)
    pmm_add_region(PMM_KERNEL_START, PMM_KERNEL_END - PMM_KERNEL_START, PMM_REGION_KERNEL);
    
    // Add heap region (reserved)
    pmm_add_region(PMM_HEAP_START, PMM_HEAP_SIZE, PMM_REGION_RESERVED);
    
    // Add bitmap region (reserved)
    uint32_t bitmap_bytes = pmm_bitmap_size * sizeof(uint32_t);
    pmm_add_region(PMM_MANAGED_START, bitmap_bytes, PMM_REGION_RESERVED);
    
    // Add available region (after bitmap)
    uint32_t available_start = PMM_MANAGED_START + ((bitmap_bytes + PMM_PAGE_SIZE - 1) & ~(PMM_PAGE_SIZE - 1));
    uint32_t available_size = PMM_MANAGED_SIZE - (available_start - PMM_MANAGED_START);
    pmm_add_region(available_start, available_size, PMM_REGION_AVAILABLE);
    
    // Mark reserved pages in bitmap
    for (uint32_t i = 0; i < pmm_region_count; i++) {
        if (pmm_regions[i].type != PMM_REGION_AVAILABLE) {
            uint32_t start_page = pmm_addr_to_page(pmm_regions[i].start_address);
            uint32_t page_count = (pmm_regions[i].size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
            
            // Adjust for our managed range
            if (pmm_regions[i].start_address >= PMM_MANAGED_START) {
                start_page -= pmm_addr_to_page(PMM_MANAGED_START);
                for (uint32_t j = 0; j < page_count && (start_page + j) < pmm_total_pages; j++) {
                    pmm_set_bit(start_page + j);
                }
            }
        }
    }
    
    // Count free pages
    pmm_free_page_count = 0;
    for (uint32_t i = 0; i < pmm_total_pages; i++) {
        if (!pmm_test_bit(i)) {
            pmm_free_page_count++;
        }
    }
    
    pmm_last_allocated = 0;
    pmm_initialized = 1;
    
    kprintf("PMM: Initialized. Managing %u pages (%u KB)\n", 
            pmm_total_pages, (pmm_total_pages * PMM_PAGE_SIZE) / 1024);
    kprintf("PMM: Bitmap size: %u entries (%u bytes)\n", 
            pmm_bitmap_size, pmm_bitmap_size * sizeof(uint32_t));
    kprintf("PMM: Free pages: %u (%u KB)\n", 
            pmm_free_page_count, (pmm_free_page_count * PMM_PAGE_SIZE) / 1024);
    
    return PMM_SUCCESS;
}

// Find a contiguous block of free pages
uint32_t pmm_find_free_pages(uint32_t count) {
    if (!pmm_initialized || count == 0) {
        return 0xFFFFFFFF;
    }
    
    // Start searching from last allocated position (next-fit algorithm)
    uint32_t start = pmm_last_allocated;
    uint32_t found = 0;
    uint32_t start_page = 0;
    
    for (uint32_t i = 0; i < pmm_total_pages; i++) {
        uint32_t page = (start + i) % pmm_total_pages;
        
        if (!pmm_test_bit(page)) {
            if (found == 0) {
                start_page = page;
            }
            found++;
            
            if (found == count) {
                return start_page;
            }
        } else {
            found = 0;
        }
    }
    
    return 0xFFFFFFFF; // Not found
}

// Allocate a single page
void* pmm_alloc_page(void) {
    return pmm_alloc_pages(1);
}

// Allocate multiple contiguous pages
void* pmm_alloc_pages(uint32_t count) {
    if (!pmm_initialized || count == 0 || pmm_free_page_count < count) {
        return NULL;
    }
    
    uint32_t start_page = pmm_find_free_pages(count);
    if (start_page == 0xFFFFFFFF) {
        return NULL;
    }
    
    // Mark pages as allocated
    for (uint32_t i = 0; i < count; i++) {
        pmm_set_bit(start_page + i);
    }
    
    pmm_free_page_count -= count;
    pmm_last_allocated = (start_page + count) % pmm_total_pages;
    
    // Convert page to physical address
    uint32_t phys_addr = PMM_MANAGED_START + (start_page * PMM_PAGE_SIZE);
    
    // Zero the allocated pages
    uint32_t* ptr = (uint32_t*)phys_addr;
    uint32_t total_words = (count * PMM_PAGE_SIZE) / sizeof(uint32_t);
    for (uint32_t i = 0; i < total_words; i++) {
        ptr[i] = 0;
    }
    
    return (void*)phys_addr;
}

// Free a single page
int pmm_free_page(void* page) {
    return pmm_free_pages(page, 1);
}

// Free multiple pages
int pmm_free_pages(void* pages, uint32_t count) {
    if (!pmm_initialized || pages == NULL || count == 0) {
        return PMM_ERROR_INVALID;
    }
    
    uint32_t addr = (uint32_t)pages;
    
    // Check if address is page-aligned
    if (addr & (PMM_PAGE_SIZE - 1)) {
        return PMM_ERROR_ALIGNED;
    }
    
    // Check if address is in our managed range
    if (addr < PMM_MANAGED_START || addr >= (PMM_MANAGED_START + PMM_MANAGED_SIZE)) {
        return PMM_ERROR_INVALID;
    }
    
    // Convert to page number within our managed range
    uint32_t start_page = (addr - PMM_MANAGED_START) / PMM_PAGE_SIZE;
    
    // Check bounds
    if (start_page + count > pmm_total_pages) {
        return PMM_ERROR_INVALID;
    }
    
    // Mark pages as free
    for (uint32_t i = 0; i < count; i++) {
        if (!pmm_test_bit(start_page + i)) {
            kprintf("PMM: Warning - freeing already free page at 0x%x\n", 
                    addr + (i * PMM_PAGE_SIZE));
        }
        pmm_clear_bit(start_page + i);
    }
    
    pmm_free_page_count += count;
    
    return PMM_SUCCESS;
}

// Reserve a memory region
int pmm_reserve_region(uint32_t start, uint32_t size) {
    if (!pmm_initialized) {
        return PMM_ERROR_INVALID;
    }
    
    uint32_t start_page = pmm_addr_to_page(start);
    uint32_t page_count = (size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    
    // Adjust for our managed range
    if (start >= PMM_MANAGED_START) {
        start_page -= pmm_addr_to_page(PMM_MANAGED_START);
        for (uint32_t i = 0; i < page_count && (start_page + i) < pmm_total_pages; i++) {
            if (!pmm_test_bit(start_page + i)) {
                pmm_set_bit(start_page + i);
                pmm_free_page_count--;
            }
        }
    }
    
    return PMM_SUCCESS;
}

// Mark a region as available
int pmm_mark_available(uint32_t start, uint32_t size) {
    if (!pmm_initialized) {
        return PMM_ERROR_INVALID;
    }
    
    uint32_t start_page = pmm_addr_to_page(start);
    uint32_t page_count = (size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    
    // Adjust for our managed range
    if (start >= PMM_MANAGED_START) {
        start_page -= pmm_addr_to_page(PMM_MANAGED_START);
        for (uint32_t i = 0; i < page_count && (start_page + i) < pmm_total_pages; i++) {
            if (pmm_test_bit(start_page + i)) {
                pmm_clear_bit(start_page + i);
                pmm_free_page_count++;
            }
        }
    }
    
    return PMM_SUCCESS;
}

// Get PMM statistics
pmm_stats_t pmm_get_stats(void) {
    pmm_stats_t stats = {0};
    
    if (pmm_initialized) {
        stats.total_pages = pmm_total_pages;
        stats.free_pages = pmm_free_page_count;
        stats.used_pages = pmm_total_pages - pmm_free_page_count;
        stats.bitmap_size = pmm_bitmap_size * sizeof(uint32_t);
        stats.last_allocated_page = pmm_last_allocated;
    }
    
    return stats;
}

// Print PMM statistics
void pmm_print_stats(void) {
    if (!pmm_initialized) {
        kprintf("PMM: Not initialized\n");
        return;
    }
    
    pmm_stats_t stats = pmm_get_stats();
    
    kprintf("PMM Statistics:\n");
    kprintf("  Total pages: %u (%u KB)\n", stats.total_pages, 
            (stats.total_pages * PMM_PAGE_SIZE) / 1024);
    kprintf("  Free pages: %u (%u KB)\n", stats.free_pages, 
            (stats.free_pages * PMM_PAGE_SIZE) / 1024);
    kprintf("  Used pages: %u (%u KB)\n", stats.used_pages, 
            (stats.used_pages * PMM_PAGE_SIZE) / 1024);
    kprintf("  Bitmap size: %u bytes\n", stats.bitmap_size);
    kprintf("  Last allocated: page %u\n", stats.last_allocated_page);
    kprintf("  Memory utilization: %u%%\n", 
            (stats.used_pages * 100) / stats.total_pages);
}

// Print memory map
void pmm_print_memory_map(void) {
    kprintf("PMM Memory Map:\n");
    for (uint32_t i = 0; i < pmm_region_count; i++) {
        const char* type_str;
        switch (pmm_regions[i].type) {
            case PMM_REGION_RESERVED: type_str = "Reserved"; break;
            case PMM_REGION_AVAILABLE: type_str = "Available"; break;
            case PMM_REGION_KERNEL: type_str = "Kernel"; break;
            default: type_str = "Unknown"; break;
        }
        
        kprintf("  0x%08x - 0x%08x (%u KB) %s\n",
                pmm_regions[i].start_address,
                pmm_regions[i].start_address + pmm_regions[i].size - 1,
                pmm_regions[i].size / 1024,
                type_str);
    }
}

// Get free memory in bytes
uint32_t pmm_get_free_memory(void) {
    return pmm_initialized ? pmm_free_page_count * PMM_PAGE_SIZE : 0;
}

// Get used memory in bytes
uint32_t pmm_get_used_memory(void) {
    return pmm_initialized ? (pmm_total_pages - pmm_free_page_count) * PMM_PAGE_SIZE : 0;
}

// Check if a page is allocated
int pmm_is_page_allocated(void* page) {
    if (!pmm_initialized || page == NULL) {
        return 1; // Assume allocated if we can't check
    }
    
    uint32_t addr = (uint32_t)page;
    if (addr < PMM_MANAGED_START || addr >= (PMM_MANAGED_START + PMM_MANAGED_SIZE)) {
        return 1; // Outside our managed range
    }
    
    uint32_t page_num = (addr - PMM_MANAGED_START) / PMM_PAGE_SIZE;
    return pmm_test_bit(page_num);
}

// Dump bitmap for debugging
void pmm_dump_bitmap(uint32_t start_page, uint32_t count) {
    if (!pmm_initialized) {
        kprintf("PMM: Not initialized\n");
        return;
    }
    
    kprintf("PMM Bitmap dump (pages %u-%u):\n", start_page, start_page + count - 1);
    
    for (uint32_t i = 0; i < count && (start_page + i) < pmm_total_pages; i++) {
        if (i % 32 == 0) {
            kprintf("\n%04u: ", start_page + i);
        }
        kprintf("%c", pmm_test_bit(start_page + i) ? 'X' : '.');
    }
    kprintf("\n");
}

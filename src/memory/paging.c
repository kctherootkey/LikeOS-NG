#include "memory/paging.h"
#include "lib/kprintf.h"
#include <stdint.h>

#define NULL ((void*)0)

// Global paging structures
static pdpt_t* pdpt;
static page_directory_t* page_directories[PDPT_ENTRIES];
static physical_memory_manager_t pmm;

// Memory layout (adjusted to match kernel.ld and bootloader)
#define KERNEL_START 0x8000      // Kernel starts at 32KB (matches kernel.ld and bootloader)
#define KERNEL_END   0x20000     // Kernel ends at 128KB (giving 96KB for kernel)
#define HEAP_START   0x20000     // Heap starts right after kernel
#define HEAP_SIZE    0x1000000   // 16MB heap

// Physical memory management with free list
typedef struct free_page {
    struct free_page* next;
} free_page_t;

static free_page_t* free_list_head = NULL;
static uint32_t next_free_page = HEAP_START + HEAP_SIZE;
static uint32_t total_pages_allocated = 0;
static uint32_t total_pages_freed = 0;

// Initialize the free list with a pool of pages
static void init_free_list(void) {
    kprintf("Initializing physical memory free list...\n");
    
    // Start with 1024 pages (4MB) in the free list
    uint32_t initial_pool_size = 64;
    uint32_t pool_start = next_free_page;
    
    for (uint32_t i = 0; i < initial_pool_size; i++) {
        uint32_t page_addr = pool_start + (i * PAGE_SIZE);
        free_page_t* page = (free_page_t*)page_addr;
        
        // Zero the page first
        uint32_t* ptr = (uint32_t*)page_addr;
        for (int j = 0; j < PAGE_SIZE / 4; j++) {
            ptr[j] = 0;
        }
        
        // Link it into the free list
        page->next = free_list_head;
        free_list_head = page;
    }
    
    next_free_page += initial_pool_size * PAGE_SIZE;
    kprintf("Free list initialized with %u pages\n", initial_pool_size);
}

static void* allocate_from_free_list(void) {
    if (free_list_head == NULL) {
        // Free list is empty, allocate a new page from the pool
        void* page = (void*)next_free_page;
        next_free_page += PAGE_SIZE;
        
        // Zero the page
        uint32_t* ptr = (uint32_t*)page;
        for (int i = 0; i < PAGE_SIZE / 4; i++) {
            ptr[i] = 0;
        }
        
        total_pages_allocated++;
        return page;
    }
    
    // Take a page from the free list
    free_page_t* page = free_list_head;
    free_list_head = page->next;
    
    // Zero the page before returning it
    uint32_t* ptr = (uint32_t*)page;
    for (int i = 0; i < PAGE_SIZE / 4; i++) {
        ptr[i] = 0;
    }
    
    total_pages_allocated++;
    return (void*)page;
}

static void add_to_free_list(void* page_addr) {
    if (page_addr == NULL) return;
    
    // Ensure the page is page-aligned
    uint32_t addr = (uint32_t)page_addr;
    if (addr & (PAGE_SIZE - 1)) {
        kprintf("Warning: Freeing non-aligned page at 0x%x\n", addr);
        return;
    }
    
    free_page_t* page = (free_page_t*)page_addr;
    
    // Add to the front of the free list
    page->next = free_list_head;
    free_list_head = page;
    
    total_pages_freed++;
}

// Get memory statistics
void get_memory_stats(void) {
    kprintf("Memory Statistics:\n");
    kprintf("  Total pages allocated: %u\n", total_pages_allocated);
    kprintf("  Total pages freed: %u\n", total_pages_freed);
    kprintf("  Pages in use: %u\n", total_pages_allocated - total_pages_freed);
    
    // Count free pages in the list
    uint32_t free_count = 0;
    free_page_t* current = free_list_head;
    while (current != NULL) {
        free_count++;
        current = current->next;
        // Prevent infinite loops in case of corruption
        if (free_count > 10000) {
            kprintf("  Free pages: >10000 (list may be corrupted)\n");
            return;
        }
    }
    kprintf("  Free pages available: %u\n", free_count);
}

static uint64_t get_cr3(void) {
    uint64_t cr3;
    __asm__ __volatile__("mov %%cr3, %%eax; mov %%eax, %0" : "=m"(cr3) : : "eax");
    return cr3;
}

static void set_cr3(uint64_t cr3) {
    __asm__ __volatile__("mov %0, %%eax; mov %%eax, %%cr3" : : "m"(cr3) : "eax");
}

static void enable_pae(void) {
    uint32_t cr4;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x20;  // Set PAE bit (bit 5)
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(cr4));
}

static void enable_paging_bit(void) {
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // Set PG bit (bit 31)
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
}

void paging_init(void) {
    kprintf("Initializing PAE paging...\n");
    
    // Initialize the physical memory free list first
    init_free_list();
    
    // Allocate PDPT (must be 32-byte aligned)
    pdpt = (pdpt_t*)allocate_from_free_list();
    if (((uint32_t)pdpt) & 0x1F) {
        kprintf("ERROR: PDPT not properly aligned!\n");
        return;
    }
    
    // Initialize PDPT entries
    for (int i = 0; i < PDPT_ENTRIES; i++) {
        page_directories[i] = (page_directory_t*)allocate_from_free_list();
        
        pdpt->entries[i] = (uint64_t)(uint32_t)page_directories[i] | PAGE_PRESENT;
        
        kprintf("PDPT[%d] -> 0x%x\n", i, (uint32_t)page_directories[i]);
    }
    
    kprintf("PDPT allocated at 0x%x\n", (uint32_t)pdpt);
}

void setup_identity_mapping(void) {
    kprintf("Setting up identity mapping for first 32MB...\n");
    
    // Map first 32MB (8192 pages) identity mapped to cover our memory needs
    // This covers kernel, heap, and free list areas
    for (uint32_t addr = 0; addr < 0x2000000; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    kprintf("Identity mapping complete (32MB mapped).\n");
}

int map_page(uint32_t virtual_addr, uint64_t physical_addr, uint32_t flags) {
    virtual_addr_t vaddr;
    vaddr.raw = virtual_addr;
    
    // Get page directory
    page_directory_t* pd = page_directories[vaddr.pdpt_index];
    if (!pd) {
        kprintf("ERROR: No page directory for PDPT index %d\n", vaddr.pdpt_index);
        return -1;
    }
    
    // Check if page table exists
    if (!(pd->entries[vaddr.pd_index] & PAGE_PRESENT)) {
        // Allocate new page table
        page_table_t* pt = (page_table_t*)allocate_from_free_list();
        pd->entries[vaddr.pd_index] = (uint64_t)(uint32_t)pt | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
    }
    
    // Get page table
    page_table_t* pt = (page_table_t*)(uint32_t)(pd->entries[vaddr.pd_index] & 0xFFFFF000);
    
    // Set page table entry
    pt->entries[vaddr.pt_index] = physical_addr | flags;
    
    // Invalidate TLB entry
    __asm__ __volatile__("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    return 0;
}

int unmap_page(uint32_t virtual_addr) {
    virtual_addr_t vaddr;
    vaddr.raw = virtual_addr;
    
    page_directory_t* pd = page_directories[vaddr.pdpt_index];
    if (!pd || !(pd->entries[vaddr.pd_index] & PAGE_PRESENT)) {
        return -1;  // Page not mapped
    }
    
    page_table_t* pt = (page_table_t*)(uint32_t)(pd->entries[vaddr.pd_index] & 0xFFFFF000);
    pt->entries[vaddr.pt_index] = 0;
    
    // Invalidate TLB entry
    __asm__ __volatile__("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    return 0;
}

uint64_t get_physical_addr(uint32_t virtual_addr) {
    virtual_addr_t vaddr;
    vaddr.raw = virtual_addr;
    
    page_directory_t* pd = page_directories[vaddr.pdpt_index];
    if (!pd || !(pd->entries[vaddr.pd_index] & PAGE_PRESENT)) {
        return 0;  // Page not mapped
    }
    
    page_table_t* pt = (page_table_t*)(uint32_t)(pd->entries[vaddr.pd_index] & 0xFFFFF000);
    if (!(pt->entries[vaddr.pt_index] & PAGE_PRESENT)) {
        return 0;  // Page not mapped
    }
    
    return (pt->entries[vaddr.pt_index] & 0xFFFFF000) | vaddr.offset;
}

void enable_pae_paging(void) {
    kprintf("Enabling PAE paging...\n");
    
    // Disable paging first
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000;  // Clear PG bit
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
    
    // Enable PAE
    enable_pae();
    
    // Set CR3 to point to PDPT
    set_cr3((uint64_t)(uint32_t)pdpt);
    
    // Enable paging
    enable_paging_bit();
    
    kprintf("PAE paging enabled successfully!\n");
    kprintf("CR3 set to 0x%x\n", (uint32_t)pdpt);
}

void* allocate_physical_page(void) {
    return allocate_from_free_list();
}

void free_physical_page(void* page) {
    add_to_free_list(page);
}

void setup_kernel_heap(void) {
    kprintf("Setting up kernel heap at 0x%x (size: 0x%x)\n", HEAP_START, HEAP_SIZE);
    
    // Map heap area
    for (uint32_t addr = HEAP_START; addr < HEAP_START + HEAP_SIZE; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
}

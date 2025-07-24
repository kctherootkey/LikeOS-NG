#include "paging.h"
#include "kprintf.h"
#include <stdint.h>

// Global paging structures
static pdpt_t* pdpt;
static page_directory_t* page_directories[PDPT_ENTRIES];
static physical_memory_manager_t pmm;

// Memory layout (adjusted to match kernel.ld and bootloader)
#define KERNEL_START 0x8000      // Kernel starts at 32KB (matches kernel.ld and bootloader)
#define KERNEL_END   0x20000     // Kernel ends at 128KB (giving 96KB for kernel)
#define HEAP_START   0x20000     // Heap starts right after kernel
#define HEAP_SIZE    0x1000000   // 16MB heap

// Simple physical memory allocator
static uint32_t next_free_page = HEAP_START + HEAP_SIZE;

static void* allocate_physical_page_simple(void) {
    void* page = (void*)next_free_page;
    next_free_page += PAGE_SIZE;
    
    // Zero the page
    uint32_t* ptr = (uint32_t*)page;
    for (int i = 0; i < PAGE_SIZE / 4; i++) {
        ptr[i] = 0;
    }
    
    return page;
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
    
    // Allocate PDPT (must be 32-byte aligned)
    pdpt = (pdpt_t*)allocate_physical_page_simple();
    if (((uint32_t)pdpt) & 0x1F) {
        kprintf("ERROR: PDPT not properly aligned!\n");
        return;
    }
    
    // Initialize PDPT entries
    for (int i = 0; i < PDPT_ENTRIES; i++) {
        page_directories[i] = (page_directory_t*)allocate_physical_page_simple();
        
        pdpt->entries[i] = (uint64_t)(uint32_t)page_directories[i] | PAGE_PRESENT;
        
        kprintf("PDPT[%d] -> 0x%x\n", i, (uint32_t)page_directories[i]);
    }
    
    kprintf("PDPT allocated at 0x%x\n", (uint32_t)pdpt);
}

void setup_identity_mapping(void) {
    kprintf("Setting up identity mapping for first 4MB...\n");
    
    // Map first 4MB (1024 pages) identity mapped
    for (uint32_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    kprintf("Identity mapping complete.\n");
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
        page_table_t* pt = (page_table_t*)allocate_physical_page_simple();
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
    return allocate_physical_page_simple();
}

void free_physical_page(void* page) {
    // Simple implementation - in a real kernel you'd maintain a free list
    (void)page;
}

void setup_kernel_heap(void) {
    kprintf("Setting up kernel heap at 0x%x (size: 0x%x)\n", HEAP_START, HEAP_SIZE);
    
    // Map heap area
    for (uint32_t addr = HEAP_START; addr < HEAP_START + HEAP_SIZE; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
}

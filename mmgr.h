#ifndef __MMGR_H__
#define __MMGR_H__

#include "stdint.h"

typedef uint32_t physical_memory;
typedef uint32_t virtual_memory;

typedef physical_memory* page_table;
typedef page_table* page_directory;

// location of page directory
#define PAGE_DIRECTORY_LOCATION     0x9C000
// location of identity-mapped 0th page
#define PAGE_TABLE_0_LOCATION       0x9D000
// location of higher-half kernel mapped page (3G+)
#define PAGE_TABLE_768_LOCATION     0x9E000

// size of page directory (constant on platform, x86 for now)
#define PAGE_DIRECTORY_SIZE         1024
// size of page table (constant on platform, x86 for now)
#define PAGE_TABLE_SIZE             1024

// all entries must be aligned to this value
#define PAGING_ALIGN                0x1000

// 4k blocks
#define MMGR_BLOCK_SIZE 4096
// 4k pages
#define MMGR_PAGE_SIZE  4096
// bitmap size for 4G memory - 4G blocks, 32bits in each bitmap size (integer)
#define MMGR_PHYS_BITMAP_SIZE (uint32_t)(1 + (4294967295LL/MMGR_BLOCK_SIZE))/32

// page directory entry flags
enum PD_Entry_Flags
{
    I86_PDE_PRESENT	        = 1,
    I86_PDE_WRITABLE        = 2,
    I86_PDE_USER            = 4,
    I86_PDE_PWT             = 8,
    I86_PDE_PCD             = 0x10,
    I86_PDE_ACCESSED        = 0x20,
    I86_PDE_DIRTY           = 0x40,
    I86_PDE_4MB             = 0x80,
    I86_PDE_CPU_GLOBAL      = 0x100,
    I86_PDE_LV4_GLOBAL      = 0x200
};

// page table entry flags
enum PT_Entry_Flags
{
    I86_PTE_PRESENT         = 1,
    I86_PTE_WRITABLE        = 2,
    I86_PTE_USER            = 4,
    I86_PTE_WRITETHOUGH     = 8,
    I86_PTE_NOT_CACHEABLE   = 0x10,
    I86_PTE_ACCESSED        = 0x20,
    I86_PTE_DIRTY           = 0x40,
    I86_PTE_PAT             = 0x80,
    I86_PTE_CPU_GLOBAL      = 0x100,
    I86_PTE_LV4_GLOBAL      = 0x200,
};

class MemoryManager
{
    public:
        MemoryManager();

        // initializes paging manager
        void Initialize();

        // sets physical memory amount in bytes
        void SetPhysicalMemory(unsigned int amount);
        // retrieves total physical memory
        unsigned int GetPhysicalMemory();

        // retrieves free memory in bytes
        uint32_t GetFreeMemory();

        // allocates memory block
        void* mem_alloc(uint32_t amount);
        // frees memory block
        void mem_free(void* addr);

    protected:
        // marks physical block as used
        void SetPhysBitmapBit(uint32_t bit);
        // marks physical block as unused
        void ClearPhysBitmapBit(uint32_t bit);
        // is physical block used?
        bool GetPhysBitmapBit(uint32_t bit);
        // retrieves first free physical block
        uint32_t GetPhysBitmapFirstFree();

        // allocates physical block
        physical_memory AllocBlock();
        // frees physical block
        void FreeBlock(physical_memory addr);

        // allocates single page
        virtual_memory AllocPage();
        // allocates continuous page space of pageCount pages
        virtual_memory AllocPage(uint32_t pageCount);
        // allocates new page table at specified index
        physical_memory AllocPageTable(uint32_t directory_index);
        // allocates new page at specified directory and table index
        virtual_memory AllocPage(uint32_t directory_index, uint32_t table_index);
        // frees page
        void FreePage(virtual_memory addr);
        // flushes TLB at specified address
        void FlushTLB(physical_memory addr);

    private:
        // page directory
        page_directory m_page_directory;
        // available bytes of physical memory
        unsigned int m_physical_memory;
        // total block count
        uint32_t m_physical_block_count;
        // free block count
        uint32_t m_physical_block_free_count;
        // bitmap of physical memory usage
        uint32_t m_physical_bitmap[MMGR_PHYS_BITMAP_SIZE];

        // last page address assigned
        physical_memory m_last_page_addr;

        // beginning of "free" memory
        uint32_t m_nonkernel_memory_begin;
};

extern MemoryManager sMemMgr;

#endif

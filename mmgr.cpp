#include "mmgr.h"
#include "support.h"
#include "std.h"

#include "console.h"

MemoryManager sMemMgr;

void* operator new(uint32_t len)
{
    return (void*)sMemMgr.mem_alloc(len);
}

void operator delete(void *ptr)
{
    sMemMgr.mem_free(ptr);
}

void operator delete[](void *ptr)
{
    ::operator delete(ptr);
}

void* operator new[](uint32_t len)
{
    return ::operator new(len);
}

MemoryManager::MemoryManager()
{
    m_physical_memory = 0;
}

void MemoryManager::Initialize()
{
    m_page_directory = (page_directory)(PAGE_DIRECTORY_LOCATION);

    // align to block size
    m_nonkernel_memory_begin = MMGR_BLOCK_SIZE * (1 + (get_kernel_physical_end() / MMGR_BLOCK_SIZE));

    // clear bitmap
    memset(m_physical_bitmap, 0, MMGR_PHYS_BITMAP_SIZE * sizeof(uint32_t));

    m_last_page_addr = (physical_memory)PAGE_TABLE_768_LOCATION;

    uint32_t i;

    // reserve kernel physical memory forever
    for (i = 0; i < m_nonkernel_memory_begin / (MMGR_BLOCK_SIZE*32); i++)
        m_physical_bitmap[i] = 0xFFFFFFFF;

    // skip page table 0
    for (i = 1; i < 767; i++)
        m_page_directory[i] = (page_table)0xFFFFF000;
    // skip page table 768
    for (i = 769; i < PAGE_DIRECTORY_SIZE; i++)
        m_page_directory[i] = (page_table)0xFFFFF000;
}

void MemoryManager::SetPhysicalMemory(unsigned int amount)
{
    m_physical_memory = amount;

    // calculate total block count
    m_physical_block_count = amount / MMGR_BLOCK_SIZE;
    // and total free block count - automatically reduce by kernel size
    m_physical_block_free_count = m_physical_block_count - m_nonkernel_memory_begin / (MMGR_BLOCK_SIZE*32);
}

unsigned int MemoryManager::GetPhysicalMemory()
{
    return m_physical_memory;
}

uint32_t MemoryManager::GetFreeMemory()
{
    return m_physical_block_free_count * MMGR_BLOCK_SIZE;
}

void MemoryManager::SetPhysBitmapBit(uint32_t bit)
{
    m_physical_bitmap[bit/32] |= (1 << (bit%32));
}

void MemoryManager::ClearPhysBitmapBit(uint32_t bit)
{
    m_physical_bitmap[bit/32] &= ~(1 << (bit%32));
}

bool MemoryManager::GetPhysBitmapBit(uint32_t bit)
{
    return (m_physical_bitmap[bit/32] & (1 << (bit%32)));
}

uint32_t MemoryManager::GetPhysBitmapFirstFree()
{
    uint32_t i, j;
    // go through all blocks
    for (i = 0; i < m_physical_block_count; i++)
    {
        // if the block is not fully occupied
        if (m_physical_bitmap[i] != 0xFFFFFFFF)
		{
            // fo through all bits
			for (j = 0; j < 32; j++)
			{
                // if the bit is free, return it
				if (!(m_physical_bitmap[i] & (1 << j)))
					return i*32+j;
            }
        }
    }

	return (uint32_t)(-1);
}

physical_memory MemoryManager::AllocBlock()
{
    // get free block
    uint32_t ffit = GetPhysBitmapFirstFree();

    if (ffit == (uint32_t)(-1))
        return (uint32_t)(-1);

    // allocate it
    SetPhysBitmapBit(ffit);

    // decrease free block count
    m_physical_block_free_count--;

    // and return valid physical address
    return ffit * MMGR_BLOCK_SIZE;
}

void MemoryManager::FreeBlock(physical_memory addr)
{
    // free block (mark as unused)
    ClearPhysBitmapBit(addr / MMGR_BLOCK_SIZE);

    // increase free block count
    m_physical_block_free_count++;
}

virtual_memory MemoryManager::AllocPage()
{
    uint32_t i, j;

    // skip page 0 automatically
    for (i = 1; i < PAGE_DIRECTORY_SIZE; i++)
    {
        // if the page is present
        if (m_page_directory[i] != (page_table)0xFFFFF000)
        {
            // retrieve proper page table address
            page_table pt = (page_table)((physical_memory)m_page_directory[i] & 0xFFFFF000);
            // go through all entries
            for (j = 0; j < PAGE_TABLE_SIZE; j++)
            {
                // if there's some free entry, allocate it
                if ((pt[j] & 0xFFFFF000) == 0xFFFFF000)
                    return AllocPage(i, j);
            }
        }
        else
        {
            // allocate new page table entry and allocate first page
            AllocPageTable(i);
            return AllocPage(i, 0);
        }
    }

    return 0;
}

virtual_memory MemoryManager::AllocPage(uint32_t pageCount)
{
    uint32_t i, j;
    uint32_t pageStart;

    if (pageCount == 1)
        return AllocPage();

    // skip page 0 automatically
    for (i = 1; i < PAGE_DIRECTORY_SIZE; i++)
    {
        // page table is allocated
        if (m_page_directory[i] != (page_table)0xFFFFF000)
        {
            page_table pt = (page_table)((physical_memory)m_page_directory[i] & 0xFFFFF000);
            pageStart = 0;
            // go through all page table entries
            for (j = 0; j < PAGE_TABLE_SIZE; j++)
            {
                // if not free, reset start counter
                if ((pt[j] & 0xFFFFF000) != 0xFFFFF000)
                    pageStart = j + 1;

                // if we found continuous pageable memory, allocate it
                if ((j + 1) - pageStart == pageCount)
                {
                    virtual_memory frst = AllocPage(i, pageStart);
                    for (j = pageStart + 1; j < pageStart + pageCount; j++)
                        AllocPage(i, j);
                    return frst;
                }
            }
        }
        else
        {
            // allocate new table and repeat procedure on this table entry
            AllocPageTable(i--);
            continue;
        }
    }

    return 0;
}

physical_memory MemoryManager::AllocPageTable(uint32_t directory_index)
{
    m_last_page_addr += 0x1000;

    // reset page table entries
    page_table pt = (page_table)(m_last_page_addr);
    for (uint32_t i = 0; i < PAGE_TABLE_SIZE; i++)
        pt[i] = 0xFFFFF000;

    // store page table to page directory
    m_page_directory[directory_index] = (page_table)((m_last_page_addr & 0xFFFFF000) | I86_PDE_PRESENT | I86_PDE_WRITABLE);

    // don't forget to flush TLB
    FlushTLB((physical_memory)m_page_directory[directory_index]);

    return m_last_page_addr;
}

virtual_memory MemoryManager::AllocPage(uint32_t directory_index, uint32_t table_index)
{
    // allocate new entry in physical memory
    physical_memory addr = (AllocBlock() & 0xFFFFF000);

    // set the entry to page table
    page_table pt = (page_table)((physical_memory)m_page_directory[directory_index] & 0xFFFFF000);
    pt[table_index] = addr | I86_PTE_PRESENT | I86_PTE_WRITABLE;

    // flush TLB
    FlushTLB(addr);

    // and build newly allocated virtual address
    return ((directory_index & 0x3FF) << 22) | ((table_index & 0x3FF) << 12);
}

void MemoryManager::FreePage(virtual_memory addr)
{
    // TODO: verify, if the page is really allocated

    // retrieve indexes
    uint32_t directory_index = addr >> 22;
    uint32_t table_index = (addr >> 12) & 0x3FF;

    // free the physical block retrieved from table
    FreeBlock((physical_memory)m_page_directory[directory_index][table_index] & 0xFFFFF000);

    // and mark page as free
    m_page_directory[directory_index][table_index] = 0xFFFFF000;

    FlushTLB(addr);
}

void MemoryManager::FlushTLB(physical_memory addr)
{
    flushtlb(addr);
}

void* MemoryManager::mem_alloc(uint32_t amount)
{
    // TODO: support memory allocations > 4MB (page table size(1024) * page size(4k))

    virtual_memory mem = AllocPage(1 + amount/MMGR_PAGE_SIZE);
    if (mem == 0)
    {
        Console::WriteLn("Not enough memory!");
        halt();
    }

    return (void*)mem;
}

void MemoryManager::mem_free(void* addr)
{
    FreePage((virtual_memory)addr);
}

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
    RestorePageDirectory();

    // align to block size
    m_nonkernel_memory_begin = MMGR_BLOCK_SIZE * (1 + (get_kernel_physical_end() / MMGR_BLOCK_SIZE));

    // clear bitmap
    memset(m_physical_bitmap, 0, MMGR_PHYS_BITMAP_SIZE * sizeof(uint32_t));

    m_last_page_addr = (physical_memory)0x40000; // TODO: better solution, this will fail at 0x98000 due to conflict with kernel page

    uint32_t i;

    // reserve identity mapped physical memory forever
    for (i = 0; i < 0x400000 / (MMGR_BLOCK_SIZE*32); i++)
        m_physical_bitmap[i] = 0xFFFFFFFF;

    // skip page table 0
    for (i = 1; i < 767; i++)
        m_page_directory[i] = (page_table)0xFFFFF000;
    // skip page table 768
    for (i = 769; i < PAGE_DIRECTORY_SIZE; i++)
        m_page_directory[i] = (page_table)0xFFFFF000;

    // this is the page table which will be mapped to userspace to position 0 (first 4MB of virtual address space)
    // it has to contain:
    // 1) exchange point at 0x2000 - 0x2FFF
    // 2) TSS segment at 0x3000
    // 3) GDTR and GDT at 0x3100 and 0x3200
    // We will now solve this by identity mapping the first 4MB, but for the future, it would be best to perform
    // some more reliable and secure mapping
    page_table pttmp = (page_table)0x9B000;
    for (i = 0; i < PAGE_TABLE_SIZE; i++)
         pttmp[i] = i*0x1000 | I86_PTE_PRESENT | I86_PTE_WRITABLE;
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
    m_page_directory[directory_index] = (page_table)((physical_memory)(pt) | I86_PDE_PRESENT | I86_PDE_WRITABLE | I86_PDE_USER);

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
    pt[table_index] = addr | I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER;

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

page_directory MemoryManager::CreatePageDirectory(bool userspace, page_directory* kernel_mapping)
{
    virtual_memory addr = (AllocPage() & 0xFFFFF000);
    uint32_t directory_index = addr >> 22;
    uint32_t table_index = (addr >> 12) & 0x3FF;

    page_directory pdir = ((page_directory)addr);

    page_table pt = (page_table)((physical_memory)m_page_directory[directory_index] & 0xFFFFF000);
    physical_memory realaddr = (pt[table_index] & 0xFFFFF000);

    // in userspace, we map "exchange point" and some other stuff to first few bytes, but just for the moment the CPL is 0
    if (userspace)
        pdir[0] = (page_table)(0x9B000 | I86_PDE_PRESENT | I86_PDE_WRITABLE);

    // mark all page tables missing
    for (int i = userspace ? 1 : 0; i < PAGE_DIRECTORY_SIZE; i++)
        pdir[i] = (page_table)0xFFFFF000;
    // map kernel to higher 1GB (again to be able to execute stuff from CPL = 0)
    pdir[768] = (page_table)(0x9E000 | I86_PDE_PRESENT | I86_PDE_WRITABLE);

    // save kernel mapping (i.e. to be able to deallocate memory after the process ends)
    *kernel_mapping = pdir;

    return (page_directory)realaddr; // return physical value for use in cr3 register
}

virtual_memory MemoryManager::AllocStackPage(page_directory kernel_mapping)
{
    // allocate kernel page
    virtual_memory addr = (AllocPage() & 0xFFFFF000);
    // retrieve physical address from page table/page directory
    uint32_t directory_index = addr >> 22;
    uint32_t table_index = (addr >> 12) & 0x3FF;

    // real stack table is mapped to this address in kernel virtual memory, so we will use this
    page_table stacktable = ((page_table)addr);

    page_table pt = (page_table)((physical_memory)m_page_directory[directory_index] & 0xFFFFF000);
    // this is the physical memory of stack page table, which will be mapped to specified position
    physical_memory realaddr = (pt[table_index] & 0xFFFFF000);

    // allocate all pages within stack page table (so the stack will be 4MB long)
    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        addr = (AllocPage() & 0xFFFFF000);      // allocate page
        directory_index = addr >> 22;           // index within kernel directory
        table_index = (addr >> 12) & 0x3FF;     // index within page table in kernel directory
        // retrieve physical address
        pt = (page_table)((physical_memory)m_page_directory[directory_index] & 0xFFFFF000);

        // resolve physical memory
        physical_memory phmem = (pt[table_index] & 0xFFFFF000);

        // map the memory
        stacktable[i] = phmem | I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER;
    }

    // set it to page table 767, which is one page table below kernel page
    kernel_mapping[767] = (page_table)(realaddr | I86_PDE_PRESENT | I86_PDE_WRITABLE | I86_PDE_USER);

    return (virtual_memory)(((767 + 1) << 22) - 1);
}

virtual_memory MemoryManager::AllocCodePage(page_directory kernel_mapping, void* use_memory)
{
    // retrieve physical memory of code mapped in kernel virtual memory
    virtual_memory addr = (virtual_memory)use_memory;
    uint32_t directory_index = addr >> 22;
    uint32_t table_index = (addr >> 12) & 0x3FF;

    page_table pt = (page_table)((physical_memory)m_page_directory[directory_index] & 0xFFFFF000);
    physical_memory code_realaddr = (pt[table_index] & 0xFFFFF000);

    // allocate page for user code, and again retrieve real address
    addr = (virtual_memory)(AllocPage() & 0xFFFFF000);
    directory_index = addr >> 22;
    table_index = (addr >> 12) & 0x3FF;

    // retrieve code pagetable in kernel virtual memory
    page_table codetable = ((page_table)addr);
    pt = (page_table)((physical_memory)m_page_directory[directory_index] & 0xFFFFF000);
    physical_memory realaddr = (pt[table_index] & 0xFFFFF000);

    // set code physical address to process virtual memory
    codetable[0] = code_realaddr | I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER;

    // and store the page table real address to page directory of specified process
    kernel_mapping[1] = (page_table)(realaddr | I86_PDE_PRESENT | I86_PDE_WRITABLE | I86_PDE_USER);

    return (virtual_memory)(1 << 22);
}

void MemoryManager::SetPageDirectory(page_directory cr3)
{
    m_page_directory = cr3;
}

void MemoryManager::RestorePageDirectory()
{
    m_page_directory = (page_directory)(PAGE_DIRECTORY_LOCATION);
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

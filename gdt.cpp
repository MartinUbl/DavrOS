#include "std.h"
#include "gdt.h"
#include "support.h"

#define MAX_DESCRIPTORS 6

// GDT descriptors
static gdt_descriptor *__gdt = (gdt_descriptor*)(0x3200);
// GDTR structure ("points" to GDT)
static gdt_table *__gdtr = (gdt_table*)(0x3100);
// TSS entry
static tss_entry_struct *tss_entry = (tss_entry_struct*)(0x3000);

// sets GDT descriptor
static void __gdt_set_descriptor(unsigned int i, unsigned long long base, unsigned long long limit, unsigned char access, unsigned char granularity)
{
	if (i >= MAX_DESCRIPTORS)
		return;

    // clean the descriptor record
	memset((void*)&__gdt[i], 0, sizeof(gdt_descriptor));

	// set limit and base addresses
	__gdt[i].baseLo	= (unsigned short)(base & 0xffff);
	__gdt[i].baseMid = (unsigned char)((base >> 16) & 0xff);
	__gdt[i].baseHi = (unsigned char)((base >> 24) & 0xff);
	__gdt[i].limit = (unsigned short)(limit & 0xffff);

	// set flags and granularity bytes
	__gdt[i].flags = access;
	__gdt[i].granularity = (unsigned char)((limit >> 16) & 0x0f);
	__gdt[i].granularity |= granularity & 0xf0;
}

extern "C" int kernel_stack_bottom;

static void __install_tss()
{
    unsigned int base = (unsigned int)tss_entry;
    unsigned int limit = sizeof(tss_entry_struct);

    // 32-bit non-busy TSS at segment 0x28
    __gdt_set_descriptor(5, base, limit, I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_ACCESS | I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL,
                                         I86_GDT_GRAND_32BIT);

    // store some important stuff
    memset(tss_entry, 0, sizeof(tss_entry_struct));
    tss_entry->ss0 = 0x10;
    tss_entry->esp0 = (unsigned int)&kernel_stack_bottom - 1;
    tss_entry->iomap_base = (unsigned short)sizeof(tss_entry_struct);
}

int __init_gdt()
{
    // "size" of GDT
    __gdtr->limit = (sizeof(gdt_descriptor) * MAX_DESCRIPTORS) - 1;
    // base for GDT
    __gdtr->base = (unsigned int)__gdt;

    // null descriptor - always needs to be present
    __gdt_set_descriptor(0, 0, 0, 0, 0);

    // kernel code segment (0x08)
    __gdt_set_descriptor(1, 0, 0xFFFFFFFF, I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
                                           I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT);
    // kernel data segment (0x10)
    __gdt_set_descriptor(2, 0, 0xFFFFFFFF, I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
                                           I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT);
    // usermode code segment (0x18)
    __gdt_set_descriptor(3, 0, 0xFFFFFFFF, I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL | I86_GDT_DESC_EXPANSION,
                                           I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT);
    // usermode data segment (0x20)
    __gdt_set_descriptor(4, 0, 0xFFFFFFFF, I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL,
                                           I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT);

    // install task switch segment (TSS) (0x28)
    __install_tss();

    // load GDT into memory
    load_gdt((unsigned int)__gdtr);

    flush_tss(0x28 | 0x03); // 0x28 is TSS segment, use privilege level 3 ("userspace")

    return 0;
}

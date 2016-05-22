#include "std.h"
#include "gdt.h"
#include "support.h"

#define MAX_DESCRIPTORS 3

// GDT descriptors
static gdt_descriptor __gdt[MAX_DESCRIPTORS];
// GDTR structure ("points" to GDT)
static gdt_table __gdtr;

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

int __init_gdt()
{
    // "size" of GDT
    __gdtr.limit = (sizeof(gdt_descriptor) * MAX_DESCRIPTORS) - 1;
    // base for GDT
    __gdtr.base = (unsigned int)__gdt;

    // null descriptor - always needs to be present
    __gdt_set_descriptor(0, 0, 0, 0, 0);

    // kernel code segment
    __gdt_set_descriptor(1, 0, 0xFFFFFFFF, I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
                                           I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);
    // kernel data segment
    __gdt_set_descriptor(2, 0, 0xFFFFFFFF, I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
                                           I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);

    // load GDT into memory
    load_gdt((unsigned int)&__gdtr);

    return 0;
}

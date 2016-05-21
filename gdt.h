#ifndef __GDT_H__
#define __GDT_H__

// access bit
#define I86_GDT_DESC_ACCESS     0x0001
// readable and writable
#define I86_GDT_DESC_READWRITE  0x0002
// expansion direction bit
#define I86_GDT_DESC_EXPANSION  0x0004
// executable code segment
#define I86_GDT_DESC_EXEC_CODE  0x0008
// code or data descriptor
#define I86_GDT_DESC_CODEDATA   0x0010
// dpl bits
#define I86_GDT_DESC_DPL        0x0060
// "in memory" bit
#define I86_GDT_DESC_MEMORY     0x0080

// gdt descriptor granularity bit flags
// masks out limitHi (High 4 bits of limit)
#define I86_GDT_GRAND_LIMITHI_MASK  0x0f
// os defined bit
#define I86_GDT_GRAND_OS            0x10
// 32bit
#define I86_GDT_GRAND_32BIT         0x40
// 4k granularity
#define I86_GDT_GRAND_4K            0x80

// GDTR structure - "points" at real GDT
typedef struct
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) gdt_table;

// GDT descriptor - one "record" in GDT table
typedef struct
{
    unsigned short limit;    // bits 0-15 of segment limit
    unsigned short baseLo;   // bits 0-23 of base address
    unsigned char  baseMid;
    unsigned char  flags;    // descriptor access flags
    unsigned char  granularity;
    unsigned char  baseHi;   // bits 24-32 of base address
} __attribute__((packed)) gdt_descriptor;

// initializes GDT
int __init_gdt();

#endif

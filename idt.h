#ifndef __IDT_H__
#define __IDT_H__

#define MAX_INTERRUPTS 256

#define I86_IDT_DESC_BIT16      0x06
#define I86_IDT_DESC_BIT32      0x0E
#define I86_IDT_DESC_RING1      0x40
#define I86_IDT_DESC_RING2      0x20
#define I86_IDT_DESC_RING3      0x60
#define I86_IDT_DESC_PRESENT    0x80

// this should be at every interrupt routine start
#define INT_ROUTINE_BEGIN() int_disable()
// this shoulw be at every interrupt routine end
#define INT_ROUTINE_END() int_enable(); \
    asm("mov %ebp, %esp"); \
    asm("pop %ebp"); \
    asm("iret");

typedef void (*int_handler)(void);

// IDT descriptor record
typedef struct
{
    unsigned short baseLo;  // bits 0-16 of interrupt routine address
    unsigned short sel;     // code selector in gdt
    unsigned char reserved; // reserved
    unsigned char flags;    // bit flags
    unsigned short baseHi;  // bits 16-32 of interrupt routine address
} __attribute__((packed)) idt_descriptor;

// IDTR structure - "points" at IDT
typedef struct
{
    unsigned short limit;   // IDT limit
    unsigned int base;      // IDT base address
} __attribute__((packed)) idtr;

// install interrupt handler (i.e. IRQ handler)
void __use_interrupt_handler(unsigned int i, int_handler handler);
// initialize IDT
int __init_idt();

#endif

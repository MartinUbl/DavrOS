#include "std.h"
#include "idt.h"
#include "support.h"
#include "framebuffer.h"

// global IDT table
static idt_descriptor __idt[MAX_INTERRUPTS];
// IDTR structure
static idtr __idtr;

// default "null" interrupt handler
static void __default_int_handler_null()
{
    INT_ROUTINE_BEGIN();

    echo("Unhandled interrupt");

    INT_ROUTINE_END();
}

// installs interrupt handler to IDT (be sure to disable interrupts before calling this function, and enable them afterwards)
static void __install_interrupt_handler(unsigned int i, unsigned short flags, unsigned short sel, int_handler inthandler)
{
    if (i >= MAX_INTERRUPTS)
        return;

    if (!inthandler)
        return;

    unsigned int base = (unsigned int)inthandler;

    __idt[i].baseLo = (unsigned short)(base & 0xffff);
    __idt[i].baseHi = (unsigned short)((base >> 16) & 0xffff);
    __idt[i].reserved = 0;
    __idt[i].flags = (unsigned char)(flags);
    __idt[i].sel = sel;
}

void __use_interrupt_handler(unsigned int i, int_handler handler)
{
    __install_interrupt_handler(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, handler);
}

int __init_idt()
{
    int i;

    // "size" of IDT
    __idtr.limit = sizeof(idt_descriptor) * MAX_INTERRUPTS - 1;
    // base address of IDT
    __idtr.base = (unsigned int)__idt;

    // clear whole IDT
    memset(&__idt, 0, sizeof(idt_descriptor) * MAX_INTERRUPTS - 1);

    // register default handlers
    for (i = 0; i < MAX_INTERRUPTS; i++)
        __install_interrupt_handler(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (int_handler)__default_int_handler_null);

    // load IDT into memory
    load_idt((unsigned int)&__idtr);

    return 0;
}

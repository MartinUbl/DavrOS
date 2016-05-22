#include "pic.h"
#include "idt.h"
#include "support.h"
#include "std.h"

// current IRQ mask (1 = disabled, 0 = enabled)
static unsigned short __current_mask = 0xFFFF;

// sends command to PIC (PIC 1 or 2 by picNum)
static void __pic_send_command(unsigned char cmd, unsigned char picNum)
{
    if (picNum > 1)
        return;

    unsigned char reg = (picNum == 1) ? I86_PIC2_REG_COMMAND : I86_PIC1_REG_COMMAND;
    outb(reg, cmd);
}

// sends data to PIC (PIC 1 or 2 by picNum)
static void __pic_send_data(unsigned char data, unsigned char picNum)
{
    if (picNum > 1)
        return;

    unsigned char reg = (picNum == 1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
    outb(reg, data);
}

// reads data from PIC (PIC 1 or 2 by picNum)
unsigned char __pic_read_data(unsigned char picNum)
{
    if (picNum > 1)
        return 0;

    unsigned char reg = (picNum == 1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
    return inb(reg);
}

// enables IRQ
void __enable_irq(unsigned short x)
{
    // update mask
    __current_mask &= ~(1 << x);
    // if the IRQ is handled by PIC2, clear (enable) IRQ 2 (PIC 2 bridge) bit also
    if (x >= 8)
        __current_mask &= ~(1 << 2);

    // send this news to both PIC
    outb(I86_PIC1_REG_IMR, __current_mask & 0xFF);
    outb(I86_PIC2_REG_IMR, (__current_mask >> 8) & 0xFF);
}

// disables IRQ
void __disable_irq(unsigned short x)
{
    // update mask
    __current_mask |= (1 << x);
    // if there's no IRQ handled by PIC2, disable its IRQ 2
    if (x >= 8 && (__current_mask & 0xFF00) == 0)
        __current_mask |= (1 << 2);

    // send this news to both PIC
    outb(I86_PIC1_REG_IMR, __current_mask & 0xFF);
    outb(I86_PIC2_REG_IMR, (__current_mask >> 8) & 0xFF);
}

// sends "end of interrupt" ("acknowledge") to PIC
void send_eoi(int irq)
{
    // if the IRQ is handled by PIC2, send him ACK
    if (irq >= 8)
        __pic_send_command(I86_PIC_OCW2_MASK_EOI, 1);

    // send ACK to PIC1 always
	__pic_send_command(I86_PIC_OCW2_MASK_EOI, 0);
}

// hooks IRQ handler
void __use_irq(unsigned short x, void (*handler)())
{
    // PIC1 IRQs
    if (x < 8)
        __use_interrupt_handler(0x20 + x, handler);
    else // PIC2 IRQs
        __use_interrupt_handler(0x28 + x, handler);
}

int __init_pic()
{
    const unsigned char base0 = 0x20;
    const unsigned char base1 = 0x28;
    unsigned char icw = 0;

    icw = icw | (I86_PIC_ICW1_MASK_INIT | I86_PIC_ICW1_INIT_YES);

    // send initialization control word 1

    __pic_send_command(icw, 0);
    __pic_send_command(icw, 1);

    // send initialization control word 2 - base for IRQs

    __pic_send_data(base0, 0);
    __pic_send_data(base1, 1);

    // send initialization control word 3 - connection between PIC 1 and PIC 2

    __pic_send_data(0x04, 0);
    __pic_send_data(0x02, 1);

	// send initialization control word 4 - this enables i86 mode

    icw = I86_PIC_ICW4_UPM_86MODE;

    __pic_send_data(icw, 0);
    __pic_send_data(icw, 1);

    return 0;
}


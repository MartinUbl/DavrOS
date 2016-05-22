#ifndef __IRQ_HANDLERS_H__
#define __IRQ_HANDLERS_H__

// pure-ASM wrappers around C/C++ handler routines
extern "C"
{
    void handle_keyboard_irq();
    void handle_pit_irq();
    void handle_floppy_irq();
}

#endif

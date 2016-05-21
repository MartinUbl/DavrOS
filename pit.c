#include "pit.h"
#include "idt.h"
#include "pic.h"
#include "support.h"

// internal PIT tick counter
static unsigned int __pit_counter = 0;

// following part is not yet used, because PIT implementation is just very small stub for now
/*

// sends command to PIT
static void __pit_send_command(unsigned char cmd)
{
    outb(I86_PIT_REG_COMMAND, cmd);
}

// sends data to PIT
static void __pit_send_data(unsigned short data, unsigned short counter)
{
    unsigned char port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0 :
                   ((counter == I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);
    outb(port, (unsigned char)data);
}

// reads data from PIT
static unsigned char __pit_read_data(unsigned short counter)
{
    unsigned char port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0 :
                         ((counter == I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);

    return inb(port);
}

*/

// PIT IRQ handling routine
static void __pit_int_handler()
{
    INT_ROUTINE_BEGIN();

    // increment counter
    __pit_counter++;

    send_eoi(0);

    INT_ROUTINE_END();
}

void __init_pit()
{
    // hook PIT handler, IRQ0
    __use_irq(0, __pit_int_handler);

    // reset counter
    __pit_counter = 0;

    // enable IRQ0
    __enable_irq(0);
}

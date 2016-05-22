#include "pit.h"
#include "idt.h"
#include "pic.h"
#include "support.h"
#include "irq_handlers.h"

// internal PIT tick counter
static unsigned int __pit_counter = 0;

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

// following part is not yet used, because PIT implementation is just very small stub for now
/*
// reads data from PIT
static unsigned char __pit_read_data(unsigned short counter)
{
    unsigned char port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0 :
                         ((counter == I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);

    return inb(port);
}
*/

void wait_ticks(unsigned int cnt)
{
    unsigned int limit = __pit_counter + cnt;
    while (__pit_counter < limit)
    {
        while (__pit_counter < limit)
            ;
    }
}

// PIT IRQ handling routine
extern "C" void __pit_irq_handler()
{
    // increment counter
    __pit_counter++;

    send_eoi(0);
}

void __init_pit()
{
    // hook PIT handler, IRQ0
    __use_irq(0, handle_pit_irq);

    // reset counter
    __pit_counter = 0;

    // enable IRQ0
    __enable_irq(0);

    // use frequency of 1000Hz (approximatelly)
    const unsigned short timerFreq = 1000;

    unsigned short divisor = (unsigned short)(1193181 / timerFreq);

    unsigned char ocw = 0;
    ocw = (ocw & ~I86_PIT_OCW_MASK_MODE) | I86_PIT_OCW_MODE_RATEGEN;
    ocw = (ocw & ~I86_PIT_OCW_MASK_RL) | I86_PIT_OCW_RL_DATA;
    ocw = (ocw & ~I86_PIT_OCW_MASK_COUNTER) | I86_PIT_OCW_COUNTER_0;
    __pit_send_command(ocw);

    __pit_send_data(divisor & 0xff, 0);
    __pit_send_data((divisor >> 8) & 0xff, 0);
}

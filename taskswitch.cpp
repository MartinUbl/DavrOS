#include "process.h"
#include "taskswitch.h"

uint32_t* exchange_point = (uint32_t*)0x2000;

void switch_to_process(PCB_t* pcb)
{
    uint32_t *regs = &pcb->regs.eax;

    for (uint32_t i = 0; i < sizeof(regs_t) / sizeof(uint32_t); i++)
        exchange_point[i] = regs[i];

    asm("ljmp $0x08, $switch_task");
}

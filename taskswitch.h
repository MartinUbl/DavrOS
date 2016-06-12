#ifndef __TASKSWITCH_H__
#define __TASKSWITCH_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

void switch_task();

void switch_to_process(PCB_t* pcb);

#ifdef __cplusplus
}
#endif

#endif

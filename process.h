#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "stdint.h"
#include "mmgr.h"

enum ProcessState
{
    PSTATE_NEW = 0,
    PSTATE_READY = 1,
    PSTATE_RUNNING = 2,
    PSTATE_BLOCKED = 3,
    PSTATE_EXITING = 4
};

typedef struct
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr3;
} __attribute__((packed)) regs_t;

typedef struct
{
    uint32_t pid;
    ProcessState state;
    regs_t regs;
    page_directory pagedir_kernel_mapping;

    //

} PCB_t;

class ProcessManager
{
    public:
        ProcessManager();

        uint32_t CreateProcess(void* codeToBeMapped);
        PCB_t* GetPCB(uint32_t pid);

    protected:

        struct PCB_list_element_t
        {
            PCB_list_element_t* next;
            PCB_t* pcb;
        };
        struct PCB_list_t
        {
            PCB_list_element_t* head;
            int count;
        };

        PCB_list_t m_processes;

        uint32_t m_nextpid;

    private:
        //
};

extern ProcessManager sProcessMgr;

#endif

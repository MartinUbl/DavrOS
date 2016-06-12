#include "mmgr.h"
#include "process.h"
#include "std.h"

#include "console.h"

ProcessManager sProcessMgr;

ProcessManager::ProcessManager()
{
    m_processes.head = nullptr;
    m_processes.count = 0;

    m_nextpid = 1;
}

uint32_t ProcessManager::CreateProcess(void* codeToBeMapped)
{
    PCB_t *pcb;
    PCB_list_element_t *listel = new PCB_list_element_t;
    listel->next = nullptr;
    pcb = new PCB_t;
    listel->pcb = pcb;

    if (m_processes.head == nullptr)
        m_processes.head = listel;
    else
    {
        listel->next = m_processes.head;
        m_processes.head = listel;
    }

    m_processes.count++;

    pcb->pid = m_nextpid++;
    pcb->state = PSTATE_NEW;
    memset(&pcb->regs, 0, sizeof(regs_t));

    pcb->regs.cr3 = (uint32_t)sMemMgr.CreatePageDirectory(true, (page_directory*)&pcb->pagedir_kernel_mapping);
    pcb->regs.esp = sMemMgr.AllocStackPage(pcb->pagedir_kernel_mapping);
    pcb->regs.eip = sMemMgr.AllocCodePage(pcb->pagedir_kernel_mapping, codeToBeMapped);

    return pcb->pid;
}

PCB_t* ProcessManager::GetPCB(uint32_t pid)
{
    PCB_list_element_t* el = m_processes.head;
    while (el != nullptr)
    {
        if (el->pcb->pid == pid)
            return el->pcb;
        el = el->next;
    }

    return nullptr;
}

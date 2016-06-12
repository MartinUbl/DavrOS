#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#ifdef __cplusplus
extern "C"
{
#endif
    // sends data to HW port
    void outb(unsigned short port, unsigned char data);
    // reads data from HW port
    char inb(unsigned short port);
    // wait for I/O to be ready
    void io_wait();
    // halt CPU
    void halt();

    // loads GDT into memory using ASM instructuons
    void load_gdt(unsigned int address);
    // loads IDT into memory using ASM instructuons
    void load_idt(unsigned int address);
    // flushes TSS segment
    void flush_tss(unsigned int segment);

    // disables interrupts
    void int_disable();
    // enables interrupts
    void int_enable();

    // loads vendor string to vendorstr (at least 12 characters long)
    void cpuinfo_load_vendor(char* vendorstr);

    // flush TLB for address
    void flushtlb(unsigned int address);

    unsigned int get_kernel_physical_start();
    unsigned int get_kernel_physical_end();
#ifdef __cplusplus
}
#endif

#endif

#ifndef __SUPPORT_H__
#define __SUPPORT_H__

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

// disables interrupts
void int_disable();
// enables interrupts
void int_enable();

#endif

#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "support.h"
#include "pit.h"
#include "keyboard.h"
#include "shell.h"
#include "floppy.h"
#include "std.h"
#include "mmgr.h"

#include "grub/multiboot.h"

extern "C" void (*__CTOR_LIST__)();

extern "C" void call_constructors()
{
    void (**constructor)() = &__CTOR_LIST__;
    int total = *(int *)constructor;
    constructor++;
    while (total)
    {
        (*constructor)();
        total--;
        constructor++;
    }
}

extern "C" void __dso_handle()
{
    // ignore dtors for now
}

extern "C" void __cxa_atexit()
{
    // ignore dtors for now
}

// kernel panic - print message and halt CPU
static void __panic()
{
    Console::WriteLn("Kernel panic, halting machine");
    halt();
}

// kernel entry point
extern "C" void kernel_loader_med(multiboot_info_t* mbt)
{
    call_constructors();

    // clear screen, put some nice messages
    Console::Clear();
    Console::WriteLn("DavrOS v0.1");
    Console::WriteLn("Educational project\n");

    sMemMgr.Initialize();

    unsigned int avail_mem = 0;

    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mbt->mmap_addr;
    while ((unsigned int)mmap < mbt->mmap_addr + mbt->mmap_length)
    {
        avail_mem += mmap->len;
        mmap = (multiboot_memory_map_t*)((unsigned int)mmap + mmap->size + sizeof(unsigned int));
    }

    sMemMgr.SetPhysicalMemory(avail_mem);

    avail_mem /= (1024*1024);

    char buf[32];
    itoa(avail_mem, buf, 10);
    Console::Write("Available physical memory: ");
    Console::Write(buf);
    Console::WriteLn(" MB");
    Console::PutChar('\n');

    // initiate kernel load routine

	int_disable();

    // init GDT
    Console::Write("Initializing GDT... ");
    if (__init_gdt() != 0)
    {
        Console::WriteLn("ERROR");
        __panic();
    }
    Console::WriteLn("OK");

    // init IDT
    Console::Write("Initializing IDT... ");
    if (__init_idt() != 0)
    {
        Console::WriteLn("ERROR");
        __panic();
    }
    Console::WriteLn("OK");

    // init PIC
    Console::Write("Initializing PIC... ");
    if (__init_pic() != 0)
    {
        Console::WriteLn("ERROR");
        __panic();
    }
    Console::WriteLn("OK");

    // init timer (PIT)
    Console::Write("Initializing PIT... ");
    __init_pit();
    Console::WriteLn("OK");

    // init keyboard driver
    Console::Write("Initializing keyboard... ");
    sKeyboard.Initialize();
    Console::WriteLn("OK");

    // we are done with interrupt-sensitive work
    int_enable();

    // init floppy driver
    Console::Write("Initializing floppy driver... ");
    if (sFloppy.Initialize() != 0)
        Console::WriteLn("ERROR");
    else
        Console::WriteLn("OK");

    // run shell
    DefaultShell shell;

    shell.Run();
}

#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "support.h"
#include "pit.h"
#include "keyboard.h"
#include "shell.h"
#include "floppy.h"

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
extern "C" void kernel_loader_med()
{
    call_constructors();

    // clear screen, put some nice messages
	Console::Clear();
	Console::WriteLn("DavrOS v0.1");
	Console::WriteLn("Educational project\n");

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

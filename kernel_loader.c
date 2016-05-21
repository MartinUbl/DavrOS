#include "framebuffer.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "support.h"
#include "pit.h"
#include "keyboard.h"
#include "shell.h"

// kernel panic - print message and halt CPU
static void __panic()
{
    echo("Kernel panic, halting machine");
    halt();
}

// kernel entry point
void kernel_loader_med()
{
    // clear screen, put some nice messages
	clearscreen();
	echo("DavrOS v0.1\n");
	echo("Educational project\n\n");

    // initiate kernel load routine

	int_disable();

    // init GDT
    echo("Initializing GDT... ");
    if (__init_gdt() != 0)
    {
        echo("ERROR\n");
        __panic();
    }
    echo("OK\n");

    // init IDT
    echo("Initializing IDT... ");
    if (__init_idt() != 0)
    {
        echo("ERROR\n");
        __panic();
    }
    echo("OK\n");

    // init PIC
    echo("Initializing PIC... ");
    if (__init_pic() != 0)
    {
        echo("ERROR\n");
        __panic();
    }
    echo("OK\n");

    // init timer (PIT)
    echo("Initializing PIT... ");
    __init_pit();
    echo("OK\n");

    // init keyboard driver
    echo("Initializing keyboard... ");
    __init_keyboard();
    echo("OK\n");

    // we are done now
    int_enable();

    // run shell
    run_shell();
}

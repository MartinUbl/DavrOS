ENTRY(loader)                /* the name of the entry label */

SECTIONS {
    . = 0xC0100000;          /* the code should be loaded at 1 MB */

    kernel_virtual_start = .;
    kernel_physical_start = . - 0xC0000000;

    .text ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000) /* align at 4 KB */
    {
        *(.text)             /* all text sections from all files */
	*(.gnu.linkonce.t.*)
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata)-0xC0000000) /* align at 4 KB */
    {
        *(.rodata*)          /* all read-only data sections from all files */
	*(.gnu.linkonce.r.*)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data)-0xC0000000) /* align at 4 KB */
    {
	__CTOR_LIST__ = .; LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2) *(.ctors) *(.init_array) LONG(0) __CTOR_END__ = .; 
	__DTOR_LIST__ = .; LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2) *(.dtors) *(.fini_array) LONG(0) __DTOR_END__ = .; 
        data = .; _data = .; __data = .;
        *(.data)             /* all data sections from all files */
    }

    .bss ALIGN (0x1000) : AT(ADDR(.bss)-0xC0000000) /* align at 4 KB */
    {
        *(COMMON)            /* all COMMON sections from all files */
        *(.bss)              /* all bss sections from all files */
	*(.gnu.linkonce.b.*)
    }

    kernel_virtual_end = .;
    kernel_physical_end = . - 0xC0000000;
}


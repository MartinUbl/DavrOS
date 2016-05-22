#include "keyboard.h"
#include "framebuffer.h"
#include "std.h"
#include "support.h"
#include "floppy.h"
#include "pit.h"

void run_shell()
{
    // endless loop...
    while (1)
    {
        // prompt
        echo("\n#> ");

        // read from user input
        char buffer[128];
        __keyboard_flush_buffer();
        gets(buffer, 128);

        // just some testing command...
        if (strcmp(buffer, "test") == 0)
            echo("Test OK\n");
        else if (strcmp(buffer, "cpuinfo") == 0)
        {
            cpuinfo_load_vendor(buffer);
            echo(buffer);
            echo("\n");
        }
        else if (strncmp(buffer, "floppy", 6) == 0)
        {
            echo("Drive A: ");
            echo(get_floppy_type(0));
            echo("\nDrive B: ");
            echo(get_floppy_type(1));
            echo("\nFAT OEM on drive A: ");

            floppy_jump_to_offset(3);
            floppy_read_bytes(8, buffer);
            buffer[8] = 0;
            echo(buffer);
            echo("\n");
        }
        else
            echo("Invalid command or filename\n");
    }
}

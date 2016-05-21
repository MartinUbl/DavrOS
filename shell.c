#include "keyboard.h"
#include "framebuffer.h"
#include "std.h"

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
        else
            echo("Invalid command or filename\n");
    }
}

#include "shell.h"
#include "keyboard.h"
#include "console.h"
#include "std.h"
#include "support.h"
#include "floppy.h"
#include "pit.h"

DefaultShell::DefaultShell()
{
    //
}

void DefaultShell::Run()
{
    // endless loop...
    while (1)
    {
        // prompt
        Console::Write("\n#> ");

        // read from user input
        char buffer[128];
        sKeyboard.FlushBuffer();
        gets(buffer, 128);

        // just some testing command...
        if (strcmp(buffer, "test") == 0)
            Console::WriteLn("Test OK");
        else if (strcmp(buffer, "cpuinfo") == 0)
        {
            cpuinfo_load_vendor(buffer);
            Console::WriteLn(buffer);
        }
        else if (strncmp(buffer, "floppy", 6) == 0)
        {
            Console::Write("Drive A: ");
            Console::WriteLn(sFloppy.GetFloppyDriveType(0));
            Console::Write("Drive B: ");
            Console::WriteLn(sFloppy.GetFloppyDriveType(1));
            Console::Write("FAT OEM on drive A: ");

            sFloppy.SeekTo(3);
            sFloppy.ReadBytes(buffer, 8);
            buffer[8] = 0;
            Console::WriteLn(buffer);
        }
        else
            Console::WriteLn("Invalid command or filename");
    }
}

#include "filesystem.h"
#include "fat12.h"
#include "shell.h"
#include "keyboard.h"
#include "console.h"
#include "std.h"
#include "support.h"
#include "floppy.h"
#include "pit.h"
#include "mmgr.h"

static const char* shell_knownLocationTypesStr[] = {
    "?", "floppy"
};

DefaultShell::DefaultShell()
{
    m_locationType = SLT_NONE;
}

void DefaultShell::Run()
{
    // endless loop...
    while (1)
    {
        // prompt
        Console::PutChar('\n');
        Console::Write(shell_knownLocationTypesStr[m_locationType]);
        Console::Write(":> ");

        // read from user input
        char buffer[128];
        sKeyboard.FlushBuffer();
        Console::ReadLn(buffer, 128);

        // just some testing command...
        if (strcmp(buffer, "test") == 0)
            Console::WriteLn("Test OK");
        else if (strcmp(buffer, "cpuinfo") == 0)
        {
            cpuinfo_load_vendor(buffer);
            Console::WriteLn(buffer);
        }
        else if (strcmp(buffer, "free") == 0)
        {
            itoa(sMemMgr.GetFreeMemory() / (1024*1024), buffer, 10);
            Console::Write("Free memory: ");
            Console::Write(buffer);
            Console::WriteLn(" MB");
        }
        else if (strncmp(buffer, "floppy", 6) == 0)
        {
            if (sFloppy.HasFloppyInSlot(0))
            {
                Console::Write("Drive A: ");
                Console::WriteLn(sFloppy.GetFloppyDriveType(0));

                Console::WriteLn("Reading FAT12 filesystem...");

                m_currentFS = new FAT12Reader();
                m_currentFS->Initialize(&sFloppy);

                Console::WriteLn("Switched to floppy location");
                m_locationType = SLT_FLOPPY;
            }
        }
        else if (strncmp(buffer, "ls", 2) == 0)
        {
            if (!m_currentFS)
            {
                Console::WriteLn("No location selected");
                continue;
            }
            else
            {
                DirectoryEntry** dirlist = m_currentFS->GetDirectoryList();
                int i = 0;
                while (dirlist[i] != nullptr)
                {
                    Console::Write(dirlist[i]->name);
                    Console::Write("   ");
                    Console::Write(dirlist[i]->size_bytes);
                    Console::WriteLn("B");

                    i++;
                }
            }
        }
        else if (strncmp(buffer, "cat", 3) == 0)
        {
            if (!m_currentFS)
            {
                Console::WriteLn("No location selected");
                continue;
            }
            else
            {
                char* out = m_currentFS->ReadFile(&buffer[4]);
                if (!out)
                    Console::WriteLn("File not found!");
                else
                    Console::WriteLn(out);
            }
        }
        else
            Console::WriteLn("Invalid command or filename");
    }
}

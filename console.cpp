#include "console.h"
#include "framebuffer.h"

Console::Console()
{
    // disabled
}

void Console::PutChar(char c)
{
    sFramebuffer.PutChar(c);
}

void Console::Write(const char* str)
{
    sFramebuffer.Echo(str);
}

void Console::WriteLn(const char* str)
{
    sFramebuffer.Echo(str);
    sFramebuffer.PutChar('\n');
}

void Console::Clear()
{
    sFramebuffer.ClearScreen();
}

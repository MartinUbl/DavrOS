#include "console.h"
#include "keyboard.h"
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

void Console::ReadLn(char* buffer, int maxlen)
{
    int ptr = 0;
    while (ptr != maxlen - 2) // -2 due to zero at the end
    {
        // store character to buffer from keyboard buffer
        buffer[ptr] = sKeyboard.AwaitKey();
        // backspace character moves cursor back by one character
        if (buffer[ptr] == '\b')
        {
            if (ptr > 0)
            {
                Console::PutChar('\b');
                ptr--;
            }
            continue;
        }
        // endline and zero character ends reading
        else if (buffer[ptr] == '\n' || buffer[ptr] == '\0')
        {
            Console::PutChar(buffer[ptr]);
            ptr++;
            break;
        }

        // print read character, and move on
        Console::PutChar(buffer[ptr]);
        ptr++;
    }

    // zero-terminate the string
    buffer[ptr] = '\0';
}

void Console::Clear()
{
    sFramebuffer.ClearScreen();
}

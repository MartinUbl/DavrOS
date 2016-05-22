#ifndef __CONSOLE_H__
#define __CONSOLE_H__

// Console static class for managing console output
class Console
{
    public:
        // writes single character onto console
        static void PutChar(char c);
        // writes string onto console
        static void Write(const char* str);
        // writes string onto console and terminates line
        static void WriteLn(const char* str);
        // clears screen
        static void Clear();

    private:
        Console();
};

#endif

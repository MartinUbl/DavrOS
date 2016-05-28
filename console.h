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
        // writes string onto console
        static void WriteN(const char* str, int maxlen);
        // writes string onto console and terminates line
        static void WriteNLn(const char* str, int maxlen);
        // writes integer onto console
        static void Write(int num, int base = 10);
        // writes integer onto console and terminates line
        static void WriteLn(int num, int base = 10);
        // reads line from keyboard input
        static void ReadLn(char* buffer, int maxlength);
        // clears screen
        static void Clear();

    private:
        Console();
};

#endif

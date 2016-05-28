#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#define FRAMEBUFFER_ADDRESS 0x000B8000

#define FRAMEBUFFER_WIDTH 80
#define FRAMEBUFFER_HEIGHT 25

class Framebuffer
{
    public:
        Framebuffer();

        // puts character to output
        void PutChar(char c);
        // writes zero-terminated string
        void Echo(const char* str);
        // writes zero-terminated string or maximum of maxlen characters from string
        void Echo(const char* str, int maxlen);
        // clears screen and resets cursor position to 0;0
        void ClearScreen();

    protected:
        // move cursor to raw position
        void MoveCursor(unsigned short pos);
        // updates cursor position according to __cursor_row and __cursor_col variables
        void UpdateCursor();
        // rolls framebuffer by one line higher
        void Roll();
        // puts character to specific location
        void PutChar_raw(char c, int col, int row);

    private:
        // framebuffer address
        char* m_framebuffer;
        // column for new output
        int m_cursor_col;
        // row for new outputs
        int m_cursor_row;
};

extern Framebuffer sFramebuffer;

#endif

#include "support.h"
#include "basic_io.h"
#include "framebuffer.h"

// framebuffer address
static char* __framebuffer = (char*)(0x000B8000);
// column for new output
static int __cursor_col = 0;
// row for new outputs
static int __cursor_row = 0;

// move cursor to raw position
static void __move_cursor(unsigned short pos)
{
	outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
	outb(FB_DATA_PORT,    ((pos >> 8) & 0x00FF));
	outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
	outb(FB_DATA_PORT,    pos & 0x00FF);
}

// updates cursor position according to __cursor_row and __cursor_col variables
static void __update_cursor()
{
	__move_cursor(__cursor_row*FRAMEBUFFER_WIDTH + __cursor_col);
}

// rolls framebuffer by one line higher
static void __roll_framebuffer()
{
    int i, j;

    // roll buffer by one line
    for (i = 0; i < FRAMEBUFFER_HEIGHT - 1; i++)
    {
        for (j = 0; j < FRAMEBUFFER_WIDTH; j++)
        {
            __framebuffer[i * FRAMEBUFFER_WIDTH * 2 + j * 2] = __framebuffer[(i+1) * FRAMEBUFFER_WIDTH * 2 + j * 2];
            __framebuffer[i * FRAMEBUFFER_WIDTH * 2 + j * 2 + 1] = __framebuffer[(i+1) * FRAMEBUFFER_WIDTH * 2 + j * 2 + 1];
        }
    }

    // clear last line
    for (j = 0; j < FRAMEBUFFER_WIDTH; j++)
    {
        __framebuffer[(FRAMEBUFFER_HEIGHT - 1) * FRAMEBUFFER_WIDTH * 2 + j * 2] = ' ';
        __framebuffer[(FRAMEBUFFER_HEIGHT - 1) * FRAMEBUFFER_WIDTH * 2 + j * 2 + 1] = 0x07;
    }
}

// puts character to specific location
static void __putchar_raw(char c, int col, int row)
{
	__framebuffer[row * FRAMEBUFFER_WIDTH * 2 + col * 2] = c;
	__framebuffer[row * FRAMEBUFFER_WIDTH * 2 + col * 2 + 1] = 0x07;
}

void putchar(char c)
{
	switch (c)
	{
        // zero character - do not print
        case '\0':
            return;
        // newline - move cursor and reset column
		case '\n':
			__cursor_row++;
			__cursor_col = 0;
			// if we are below last line, roll framebuffer
			if (__cursor_row == FRAMEBUFFER_HEIGHT)
            {
                __roll_framebuffer();
                __cursor_row--;
            }
			__update_cursor();
			return;
        // backspace - erase character
        case '\b':
            // if at the beginning of line, move to the end of previous
            if (__cursor_col == 0)
            {
                // do not move back when on the first line
                if (__cursor_row == 0)
                    return;
                __cursor_col = FRAMEBUFFER_WIDTH - 1;
                __cursor_row--;
            }
            else // otherwise just move back by one character
            {
                __cursor_col--;
            }

            // put empty character and update cursor
            __putchar_raw(' ', __cursor_col, __cursor_row);
            __update_cursor();

            return;
	}

    // put character
	__putchar_raw(c, __cursor_col++, __cursor_row);
	// if we moved too far to exceed line, move to next line and reset column
	if (__cursor_col == FRAMEBUFFER_WIDTH)
	{
		__cursor_col = 0;
		__cursor_row++;

        // if we are below last line, roll framebuffer
		if (__cursor_row == FRAMEBUFFER_HEIGHT)
		{
            __roll_framebuffer();
            __cursor_row--;
		}
	}
	__update_cursor();
}

void echo(const char* str)
{
	int i = 0;
	// write until we reach zero character
	while (str[i] != '\0')
		putchar(str[i++]);
}

void clearscreen()
{
	int i, j;
	// for each place in framebuffer, put empty character
	for (i = 0; i < FRAMEBUFFER_WIDTH; i++)
	{
		for (j = 0; j < FRAMEBUFFER_HEIGHT; j++)
		{
			__putchar_raw(' ', i, j);
		}
	}
	// reset cursor position
	__cursor_row = 0;
	__cursor_col = 0;
	__move_cursor(0);
}

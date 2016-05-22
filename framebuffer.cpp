#include "support.h"
#include "basic_io.h"
#include "framebuffer.h"

Framebuffer sFramebuffer;

Framebuffer::Framebuffer()
{
    m_cursor_row = 0;
    m_cursor_col = 0;

    m_framebuffer = (char*)FRAMEBUFFER_ADDRESS;
}

void Framebuffer::MoveCursor(unsigned short pos)
{
	outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
	outb(FB_DATA_PORT,    ((pos >> 8) & 0x00FF));
	outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
	outb(FB_DATA_PORT,    pos & 0x00FF);
}

void Framebuffer::UpdateCursor()
{
	MoveCursor(m_cursor_row * FRAMEBUFFER_WIDTH + m_cursor_col);
}

void Framebuffer::Roll()
{
    int i, j;

    // roll buffer by one line
    for (i = 0; i < FRAMEBUFFER_HEIGHT - 1; i++)
    {
        for (j = 0; j < FRAMEBUFFER_WIDTH; j++)
        {
            m_framebuffer[i * FRAMEBUFFER_WIDTH * 2 + j * 2] = m_framebuffer[(i+1) * FRAMEBUFFER_WIDTH * 2 + j * 2];
            m_framebuffer[i * FRAMEBUFFER_WIDTH * 2 + j * 2 + 1] = m_framebuffer[(i+1) * FRAMEBUFFER_WIDTH * 2 + j * 2 + 1];
        }
    }

    // clear last line
    for (j = 0; j < FRAMEBUFFER_WIDTH; j++)
    {
        m_framebuffer[(FRAMEBUFFER_HEIGHT - 1) * FRAMEBUFFER_WIDTH * 2 + j * 2] = ' ';
        m_framebuffer[(FRAMEBUFFER_HEIGHT - 1) * FRAMEBUFFER_WIDTH * 2 + j * 2 + 1] = 0x07;
    }
}

void Framebuffer::PutChar_raw(char c, int col, int row)
{
	m_framebuffer[row * FRAMEBUFFER_WIDTH * 2 + col * 2] = c;
	m_framebuffer[row * FRAMEBUFFER_WIDTH * 2 + col * 2 + 1] = 0x07;
}

void Framebuffer::PutChar(char c)
{
	switch (c)
	{
        // zero character - do not print
        case '\0':
            return;
        // newline - move cursor and reset column
		case '\n':
			m_cursor_row++;
			m_cursor_col = 0;
			// if we are below last line, roll framebuffer
			if (m_cursor_row == FRAMEBUFFER_HEIGHT)
            {
                Roll();
                m_cursor_row--;
            }
			UpdateCursor();
			return;
        // backspace - erase character
        case '\b':
            // if at the beginning of line, move to the end of previous
            if (m_cursor_col == 0)
            {
                // do not move back when on the first line
                if (m_cursor_row == 0)
                    return;
                m_cursor_col = FRAMEBUFFER_WIDTH - 1;
                m_cursor_row--;
            }
            else // otherwise just move back by one character
            {
                m_cursor_col--;
            }

            // put empty character and update cursor
            PutChar_raw(' ', m_cursor_col, m_cursor_row);
            UpdateCursor();

            return;
	}

    // put character
	PutChar_raw(c, m_cursor_col++, m_cursor_row);
	// if we moved too far to exceed line, move to next line and reset column
	if (m_cursor_col == FRAMEBUFFER_WIDTH)
	{
		m_cursor_col = 0;
		m_cursor_row++;

        // if we are below last line, roll framebuffer
		if (m_cursor_row == FRAMEBUFFER_HEIGHT)
		{
            Roll();
            m_cursor_row--;
		}
	}
	UpdateCursor();
}

void Framebuffer::Echo(const char* str)
{
	int i = 0;
	// write until we reach zero character
	while (str[i] != '\0')
		PutChar(str[i++]);
}

void Framebuffer::ClearScreen()
{
	int i, j;
	// for each place in framebuffer, put empty character
	for (i = 0; i < FRAMEBUFFER_WIDTH; i++)
	{
		for (j = 0; j < FRAMEBUFFER_HEIGHT; j++)
		{
			PutChar_raw(' ', i, j);
		}
	}
	// reset cursor position
	m_cursor_row = 0;
	m_cursor_col = 0;
	MoveCursor(0);
}

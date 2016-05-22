#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

// port for keyboard state control
#define KEYBOARD_STATE_PORT 0x64
// port for keyboard data (input - scancodes, output - LEDs)
#define KEYBOARD_DATA_PORT  0x60

// scroll lock flag
#define KB_LED_SCROLLLOCK   (unsigned char)0x01
// num lock flag
#define KB_LED_NUMLOCK      (unsigned char)0x02
// caps lock flag
#define KB_LED_CAPSLOCK     (unsigned char)0x04

// size of buffer for keyboard input
#define KEYBOARD_BUFFER_SIZE 32

class KeyboardHandler
{
    public:
        KeyboardHandler();

        void Initialize();

        void SignalScancode(char scancode);

        int IsLEDOn(unsigned char led);
        void ToggleLED(unsigned char led);
        void ClearLED(unsigned char led);
        void SetLED(unsigned char led);
        void FlushBuffer();

        char AwaitKey();

    protected:
        unsigned char ConvertScancodeToCharacter(unsigned char scancode, int* pressed);
        void UpdateLEDs();

    private:
        // is SHIFT being held?
        int m_shift_on;
        // is CTRL being held?
        int m_ctrl_on;
        // is ALT being held?
        int m_alt_on;
        // keyboard LEDs
        int m_kb_state_lock;

        // buffer for keyboard input
        char m_keyboard_buffer[KEYBOARD_BUFFER_SIZE];
        // index in keyboard buffer (first free position)
        int m_keyboard_buffer_ptr;
};

extern KeyboardHandler sKeyboard;

// reads string using keyboard input
void gets(char* buffer, int maxlen);

#endif

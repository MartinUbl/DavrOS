#include "keyboard.h"
#include "pic.h"
#include "idt.h"
#include "support.h"
#include "keymap.h"
#include "irq_handlers.h"

#include "console.h"

KeyboardHandler sKeyboard;

KeyboardHandler::KeyboardHandler()
{
    m_shift_on = 0;
    m_ctrl_on = 0;
    m_alt_on = 0;
    m_kb_state_lock = 0;
    m_keyboard_buffer_ptr = 0;
}

// converts scancode to character according to current key map
unsigned char KeyboardHandler::ConvertScancodeToCharacter(unsigned char scancode, int* pressed)
{
    // extract original scancode
    int origScancode = scancode & 0x7F;     // lower 7 bits
    // is key being pressed or released?
    int press = (scancode & 0x80) ? 1 : 0;  // highest bit (1 = press, 0 = release)

    // set flag back
    *pressed = press;

    // handle special scancodes
    switch (origScancode)
    {
        case 0x36: // left shift
        case 0x2A: // right shift
            m_shift_on = !press;
            return 0;
        case 0x1D: // CTRL key
            m_ctrl_on = !press;
            return 0;
        case 0x38: // ALT key
            m_alt_on = !press;
            return 0;
        case 0x3A: // capslock
            ToggleLED(KB_LED_CAPSLOCK);
            return 0;
        case 0x45: // numlock
            ToggleLED(KB_LED_NUMLOCK);
            return 0;
        case 0x46: // scrolllock
            ToggleLED(KB_LED_SCROLLLOCK);
            return 0;
    }

    // select proper keymap

    // shift key
    if (m_shift_on)
    {
        // shift+capslock
        if (IsLEDOn(KB_LED_CAPSLOCK))
            return keyMapCapsShifted[origScancode];
        // just shift
        return keyMapShifted[origScancode];
    }

    // capslock map
    if (IsLEDOn(KB_LED_CAPSLOCK))
        return keyMapCapsNormal[origScancode];

    // normal
    return keyMap[origScancode];
}

void KeyboardHandler::SignalScancode(char scancode)
{
    int press;
    char convchar = (char)ConvertScancodeToCharacter(scancode, &press);

    // if the character is being pressed, and is not any kind of special character, and
    // the buffer is still not empty, put the newly acquired character to buffer
    if (press && convchar != 0 && m_keyboard_buffer_ptr != KEYBOARD_BUFFER_SIZE - 1)
        m_keyboard_buffer[m_keyboard_buffer_ptr++] = convchar;
}

// keyboard IRQ1 handler
extern "C" void __keyboard_irq_handler()
{
    // read scancode
    char scancode = inb(KEYBOARD_DATA_PORT);
    // send it to keyboard handler
    sKeyboard.SignalScancode(scancode);

    // acknowledge PIC about interrupt being handled
    send_eoi(0);
}

char KeyboardHandler::AwaitKey()
{
    while (m_keyboard_buffer_ptr == 0)
    {
        while (m_keyboard_buffer_ptr == 0)
            ;
    }

    return m_keyboard_buffer[--m_keyboard_buffer_ptr];
}

// send current LED state to keyboard data port
void KeyboardHandler::UpdateLEDs()
{
    outb(KEYBOARD_DATA_PORT, m_kb_state_lock);
}

void KeyboardHandler::ClearLED(unsigned char led)
{
    m_kb_state_lock &= ~(led);

    UpdateLEDs();
}

void KeyboardHandler::SetLED(unsigned char led)
{
    m_kb_state_lock |= led;

    UpdateLEDs();
}

int KeyboardHandler::IsLEDOn(unsigned char led)
{
    if ((m_kb_state_lock & led) != 0)
        return 1;
    else
        return 0;
}

void KeyboardHandler::ToggleLED(unsigned char led)
{
    if (IsLEDOn(led))
        ClearLED(led);
    else
        SetLED(led);
}

void KeyboardHandler::FlushBuffer()
{
    m_keyboard_buffer_ptr = 0;

    // following loop clears all characters on input in keyboard hardware buffer
    unsigned char temp;
    do
    {
        temp = inb(KEYBOARD_STATE_PORT);
        // 0x01 = there is something in my buffer
        if ((temp & 0x01) != 0)
        {
            // read from buffer and do not use it
            inb(KEYBOARD_DATA_PORT);
            continue;
        }
    }
    while ((temp & 0x02) != 0);
}

void KeyboardHandler::Initialize()
{
    // reset state and buffer
    m_kb_state_lock = 0;
    m_keyboard_buffer_ptr = 0;

    // hook IRQ 1
    __use_irq(1, handle_keyboard_irq);

    // enable keyboard IRQ 1
    __enable_irq(1);

    // send LED state
    UpdateLEDs();
}

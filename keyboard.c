#include "keyboard.h"
#include "pic.h"
#include "idt.h"
#include "support.h"

#include "framebuffer.h"

// is SHIFT being held?
static int __shift_on = 0;
// is CTRL being held?
static int __ctrl_on = 0;
// is ALT being held?
static int __alt_on = 0;

// keyboard LEDs
static int __kb_state_lock = 0;

// size of buffer for keyboard input
#define KEYBOARD_BUFFER_SIZE 32

// buffer for keyboard input
static char __keyboard_buffer[KEYBOARD_BUFFER_SIZE];
// index in keyboard buffer (first free position)
static int __keyboard_buffer_ptr = 0;

// key map for normal typing
static unsigned char keyMap[] =
{
    0,0x1B,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','z','u','i','o','p','[',']','\n',0x80,
    'a','s','d','f','g','h','j','k','l',';',047,0140,0x80,
    0134,'y','x','c','v','b','n','m',',','.','/',0x80,
    '*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,'7','8','9',0x80,'4','5','6',0x80,
    '1','2','3','0',0177
};

// shifted key map
static unsigned char keyMapShifted[] =
{
    0,033,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
	'Q','W','E','R','T','Z','U','I','O','P','{','}','\n',0x80,
	'A','S','D','F','G','H','J','K','L',':',042,'~',0x80,
	'|','Y','X','C','V','B','N','M','<','>','_',0x80,
	'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,'7','8','9',0x80,'4','5','6',0x80,
	'1','2','3','0',177
};

// caps-lock key map
static unsigned char keyMapCapsNormal[] =
{
    0,0x1B,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'Q','W','E','R','T','Z','U','I','O','P','[',']','\n',0x80,
    'A','S','D','F','G','H','J','K','L',';',047,0140,0x80,
    '|','Y','X','C','V','B','N','M',',','.','/',0x80,
    '*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,'0',0177
};

// caps-lock + shift key map
static unsigned char keyMapCapsShifted[] =
{
	0,033,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
	'q','w','e','r','t','z','u','i','o','p','{','}','\n',0x80,
	'a','s','d','f','g','h','j','k','l',':',042,'~',0x80,
	0134,'y','x','c','v','b','n','m','<','>','?',0x80,
	'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,'7','8','9',0x80,'4','5','6',0x80,
	'1','2','3','0',177
};

// converts scancode to character according to current key map
static unsigned char __convert_scancode_to_char(unsigned char scancode, int* pressed)
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
            __shift_on = press;
            return 0;
        case 0x1D: // CTRL key
            __ctrl_on = press;
            return 0;
        case 0x38: // ALT key
            __alt_on = press;
            return 0;
        case 0x3A: // capslock
            __keyboard_toggle_led(KB_LED_CAPSLOCK);
            return 0;
        case 0x45: // numlock
            __keyboard_toggle_led(KB_LED_NUMLOCK);
            return 0;
        case 0x46: // scrolllock
            __keyboard_toggle_led(KB_LED_SCROLLLOCK);
            return 0;
    }

    // select proper keymap

    // shift key
    if (__shift_on)
    {
        // shift+capslock
        if (__keyboard_is_led_on(KB_LED_CAPSLOCK))
            return keyMapCapsShifted[origScancode];
        // just shift
        return keyMapShifted[origScancode];
    }

    // capslock map
    if (__keyboard_is_led_on(KB_LED_CAPSLOCK))
        return keyMapCapsNormal[origScancode];

    // normal
    return keyMap[origScancode];
}

// keyboard IRQ1 handler
static void __keyboard_irq_handler()
{
    INT_ROUTINE_BEGIN();

    // read scancode and convert it
    int press;
    char scancode = inb(KEYBOARD_DATA_PORT);
    char convchar = (char)__convert_scancode_to_char(scancode, &press);

    // if the character is being pressed, and is not any kind of special character, and
    // the buffer is still not empty, put the newly acquired character to buffer
    if (press && convchar != 0 && __keyboard_buffer_ptr != KEYBOARD_BUFFER_SIZE - 1)
        __keyboard_buffer[__keyboard_buffer_ptr++] = convchar;

    // acknowledge PIC about interrupt being handled
    send_eoi(0);

    INT_ROUTINE_END();
}

void gets(char* buffer, int maxlen)
{
    int ptr = 0;
    while (ptr != maxlen - 2) // -2 due to zero at the end
    {
        // nested loops due to waiting for interrupts and probably not so secure returning to next instruction
        // this may need some fixes to avoid nested waiting
        while (__keyboard_buffer_ptr == 0)
        {
            while (__keyboard_buffer_ptr == 0)
                ;
        }

        // store character to buffer from keyboard buffer
        buffer[ptr] = __keyboard_buffer[--__keyboard_buffer_ptr];
        // backspace character moves cursor back by one character
        if (buffer[ptr] == '\b')
        {
            if (ptr > 0)
            {
                putchar('\b');
                ptr--;
            }
            continue;
        }
        // endline and zero character ends reading
        else if (buffer[ptr] == '\n' || buffer[ptr] == '\0')
        {
            putchar(buffer[ptr]);
            ptr++;
            break;
        }

        // print read character, and move on
        putchar(buffer[ptr]);
        ptr++;
    }

    // zero-terminate the string
    buffer[ptr] = '\0';
}

// send current LED state to keyboard data port
static void __keyboard_update_leds()
{
    outb(KEYBOARD_DATA_PORT, __kb_state_lock);
}

void __keyboard_clear_led(unsigned char led)
{
    __kb_state_lock &= ~(led);

    __keyboard_update_leds();
}

void __keyboard_set_led(unsigned char led)
{
    __kb_state_lock |= led;

    __keyboard_update_leds();
}

int __keyboard_is_led_on(unsigned char led)
{
    if ((__kb_state_lock & led) != 0)
        return 1;
    else
        return 0;
}

void __keyboard_toggle_led(unsigned char led)
{
    if (__keyboard_is_led_on(led))
        __keyboard_clear_led(led);
    else
        __keyboard_set_led(led);
}

void __keyboard_flush_buffer()
{
    __keyboard_buffer_ptr = 0;

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

void __init_keyboard()
{
    // reset state and buffer
    __kb_state_lock = 0;
    __keyboard_buffer_ptr = 0;

    // hook IRQ 1
    __use_irq(1, __keyboard_irq_handler);

    // enable keyboard IRQ 1
    __enable_irq(1);

    // send LED state
    __keyboard_update_leds();
}

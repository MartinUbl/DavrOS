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

// is keyboard LED on?
int __keyboard_is_led_on(unsigned char led);
// cleas keyboard LED
void __keyboard_clear_led(unsigned char led);
// switch keyboard LED on
void __keyboard_set_led(unsigned char led);
// toggle keyboard LED
void __keyboard_toggle_led(unsigned char led);

// initializes and installs keyboard driver
void __init_keyboard();

// flushes keyboard hardware and software buffer
void __keyboard_flush_buffer();

// reads string using keyboard input
void gets(char* buffer, int maxlen);

#endif

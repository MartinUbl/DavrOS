#ifndef __PIT_H__
#define __PIT_H__

#define I86_PIT_REG_COUNTER0    0x40
#define I86_PIT_REG_COUNTER1    0x41
#define I86_PIT_REG_COUNTER2    0x42
#define I86_PIT_REG_COMMAND     0x43

#define I86_PIT_OCW_MASK_BINCOUNT       0x01
#define I86_PIT_OCW_MASK_MODE           0x0E
#define I86_PIT_OCW_MASK_RL             0x30
#define I86_PIT_OCW_MASK_COUNTER        0xC0

// operation commands
#define I86_PIT_OCW_BINCOUNT_BINARY     0x00
#define I86_PIT_OCW_BINCOUNT_BCD        0x01

// when setting counter mode
#define I86_PIT_OCW_MODE_TERMINALCOUNT  0x00
#define I86_PIT_OCW_MODE_ONESHOT        0x02
#define I86_PIT_OCW_MODE_RATEGEN        0x04
#define I86_PIT_OCW_MODE_SQUAREWAVEGEN  0x06
#define I86_PIT_OCW_MODE_SOFTWARETRIG   0x08
#define I86_PIT_OCW_MODE_HARDWARETRIG   0x0A

// when setting data transfer
#define I86_PIT_OCW_RL_LATCH            0x00
#define I86_PIT_OCW_RL_LSBONLY          0x10
#define I86_PIT_OCW_RL_MSBONLY          0x20
#define I86_PIT_OCW_RL_DATA             0x30

// when setting the counter we are working with
#define I86_PIT_OCW_COUNTER_0           0x00
#define I86_PIT_OCW_COUNTER_1           0x40
#define I86_PIT_OCW_COUNTER_2           0x80

// waits for specified amount of timer ticks
void wait_ticks(unsigned int cnt);

// initializes and installs PIT IRQ handler
void __init_pit();

#endif

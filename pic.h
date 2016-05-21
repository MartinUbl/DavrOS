#ifndef __PIC_H__
#define __PIC_H__

// PIC IRQ numbers

// first PIC
#define I86_PIC_IRQ_TIMER	    0
#define I86_PIC_IRQ_KEYBOARD    1
#define I86_PIC_IRQ_SERIAL2	    3
#define I86_PIC_IRQ_SERIAL1	    4
#define I86_PIC_IRQ_PARALLEL2   5
#define I86_PIC_IRQ_DISKETTE    6
#define I86_PIC_IRQ_PARALLEL1   7

// second PIC
#define I86_PIC_IRQ_CMOSTIMER   0
#define I86_PIC_IRQ_CGARETRACE  1
#define I86_PIC_IRQ_AUXILIARY   4
#define I86_PIC_IRQ_FPU         5
#define I86_PIC_IRQ_HDC         6

// PIC commands

// 2 bit masked
#define I86_PIC_OCW2_MASK_L1        0x01
#define I86_PIC_OCW2_MASK_L2        0x02
#define I86_PIC_OCW2_MASK_L3        0x04
#define I86_PIC_OCW2_MASK_EOI       0x20
#define I86_PIC_OCW2_MASK_SL        0x40
#define I86_PIC_OCW2_MASK_ROTATE    0x80

// 3 bit masked
#define I86_PIC_OCW3_MASK_RIS       0x01
#define I86_PIC_OCW3_MASK_RIR       0x02
#define I86_PIC_OCW3_MASK_MODE      0x04
#define I86_PIC_OCW3_MASK_SMM       0x20
#define I86_PIC_OCW3_MASK_ESMM      0x40
#define I86_PIC_OCW3_MASK_D7        0x80

// PIC port addresses

// PIC 1 register port addresses
#define I86_PIC1_REG_COMMAND        0x20
#define I86_PIC1_REG_STATUS	        0x20
#define I86_PIC1_REG_DATA           0x21
#define I86_PIC1_REG_IMR            0x21

// PIC 2 register port addresses
#define I86_PIC2_REG_COMMAND        0xA0
#define I86_PIC2_REG_STATUS         0xA0
#define I86_PIC2_REG_DATA           0xA1
#define I86_PIC2_REG_IMR            0xA1

// initialization control word bit masks

// control word 1
#define I86_PIC_ICW1_MASK_IC4       0x01
#define I86_PIC_ICW1_MASK_SNGL      0x02
#define I86_PIC_ICW1_MASK_ADI       0x04
#define I86_PIC_ICW1_MASK_LTIM      0x08
#define I86_PIC_ICW1_MASK_INIT      0x10

// control words 2 and 3 do not require init bit masks

// control word 4
#define I86_PIC_ICW4_MASK_UPM       0x01
#define I86_PIC_ICW4_MASK_AEOI      0x02
#define I86_PIC_ICW4_MASK_MS        0x04
#define I86_PIC_ICW4_MASK_BUF       0x08
#define I86_PIC_ICW4_MASK_SFNM      0x10

// command 1 control bits

#define I86_PIC_ICW1_IC4_EXPECT             0x01
#define I86_PIC_ICW1_IC4_NO                 0x00
#define I86_PIC_ICW1_SNGL_YES               0x02
#define I86_PIC_ICW1_SNGL_NO                0x00
#define I86_PIC_ICW1_ADI_CALLINTERVAL4      0x04
#define I86_PIC_ICW1_ADI_CALLINTERVAL8      0x00
#define I86_PIC_ICW1_LTIM_LEVELTRIGGERED    0x08
#define I86_PIC_ICW1_LTIM_EDGETRIGGERED     0x00
#define I86_PIC_ICW1_INIT_YES               0x10
#define I86_PIC_ICW1_INIT_NO                0x00

// command 4 control bits

#define I86_PIC_ICW4_UPM_86MODE             0x01
#define I86_PIC_ICW4_UPM_MCSMODE            0x00
#define I86_PIC_ICW4_AEOI_AUTOEOI           0x02
#define I86_PIC_ICW4_AEOI_NOAUTOEOI         0x00
#define I86_PIC_ICW4_MS_BUFFERMASTER        0x04
#define I86_PIC_ICW4_MS_BUFFERSLAVE         0x00
#define I86_PIC_ICW4_BUF_MODEYES            0x08
#define I86_PIC_ICW4_BUF_MODENO             0x00
#define I86_PIC_ICW4_SFNM_NESTEDMODE        0x10
#define I86_PIC_ICW4_SFNM_NOTNESTED         0x00

// initializes PIC
int __init_pic();

// reads data from PIC
unsigned char __pic_read_data(unsigned char picNum);
// hooks IRQ handler
void __use_irq(unsigned short x, void (*handler)());
// enables IRQ
void __enable_irq(unsigned short x);
// disables IRQ
void __disable_irq(unsigned short x);
// sends PIC "acknowledge" at the end of IRQ handling routine
void send_eoi(int irq);

#endif

#include "support.h"
#include "dma.h"

#define LOW_BYTE_16(x) (x & 0x00FF)
#define HI_BYTE_16(x)  ((x & 0xFF00) >> 8)

// quick access arrays of ports/registers for each DMA channel
unsigned char dma_mask_reg[8] = { 0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4 };
unsigned char dma_mode_reg[8] = { 0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6 };
unsigned char dma_clear_reg[8] = { 0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8 };

unsigned char dma_page_port[8] = { 0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A };
unsigned char dma_addr_port[8] = { 0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC };
unsigned char dma_count_port[8] = { 0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE };

void dma_transfer_init(unsigned char channel, unsigned char page, unsigned int offset, unsigned int length, unsigned char mode)
{
    // DMA init procedure should not be interrupted by anything - disable interrupts
    int_disable();

    // mask this DMA channel
    outb(dma_mask_reg[channel], 0x04 | channel);

    outb(dma_clear_reg[channel], 0xFF);
    // send DMA buffer address (big endian - fisrt low byte, then high byte)
    outb(dma_addr_port[channel], LOW_BYTE_16(offset));
    outb(dma_addr_port[channel], HI_BYTE_16(offset));
    // send physical page of DMA buffer
    outb(dma_page_port[channel], page);

    outb(dma_clear_reg[channel], 0xFF);

    // length of data (big endian)
    outb(dma_count_port[channel], LOW_BYTE_16(length));
    outb(dma_count_port[channel], HI_BYTE_16(length));

    // send mode byte
    outb(dma_mode_reg[channel], mode);

    // clear the mask of this DMA channel
    outb(dma_mask_reg[channel], channel);

    int_enable();
}

void dma_init(unsigned char channel, void* address, unsigned int length, int read)
{
	unsigned char page = 0, mode = 0;
	unsigned int offset = 0;

	if (read == 1)
		mode = 0x44;  // 01:0:0:01:00 = single / inc / no-auto / to-mem / channel
	else
		mode = 0x48;  // 01:0:0:10:00 = single / inc / no-auto / from-mem / channel
    mode += channel;

	page = ((unsigned int)address) >> 16;
	offset = ((unsigned int)address) & 0xFFFF;
	length--;

	dma_transfer_init(channel, page, offset, length, mode);
}

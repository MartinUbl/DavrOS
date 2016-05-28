#ifndef __DMA_H__
#define __DMA_H__

// floppy uses channel 2
#define DMA_CHANNEL_FLOPPY 2

#ifdef __cplusplus
extern "C"
{
#endif

    void dma_init(unsigned char channel, void* address, unsigned int length, int read);

#ifdef __cplusplus
}
#endif

#endif

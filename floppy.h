#ifndef __FLOPPY_H__
#define __FLOPPY_H__

// floppy ports
#define FLOPPY_DOR_PORT     0x03F2
#define FLOPPY_MSR_PORT     0x03F4
#define FLOPPY_FIFO_PORT    0x03F5
#define FLOPPY_CCR_PORT     0x03F7

// floppy commands
#define FLOPPY_CMD_SPECIFY              3
#define FLOPPY_CMD_WRITE_DATA           5
#define FLOPPY_CMD_READ_DATA            6
#define FLOPPY_CMD_RECALIBRATE          7
#define FLOPPY_CMD_SENSE_INTERRUPT      8
#define FLOPPY_CMD_SEEK                 15

// floppy DMA buffer size (512 bytes = 1 sector) (original: 0x4800)
#define FLOPPY_DMALEN       0x200

// maximum detectable floppy types
#define MAX_FLOPPY_TYPES    6

// 1.44M floppy geometry

// heads per drive
#define DG144_HEADS         2
// number of tracks
#define DG144_TRACKS        80
// sectors per track
#define DG144_SPT           18
// gap3 while formatting
#define DG144_GAP3FMT       0x54
// gap3 while r/w
#define DG144_GAP3RW        0x1B

// retrieves floppy drive type
const char* get_floppy_type(int slot);

// jumps to specified offset (in bytes)
int floppy_jump_to_offset(int offset);
// reads specified byte count from floppy
int floppy_read_bytes(int count, char* target);
// writes specifies byte count onto floppy
int floppy_write_bytes(int count, char* source);

// initializes floppy driver
int __init_floppy();

#endif

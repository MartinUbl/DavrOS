#ifndef __FLOPPY_H__
#define __FLOPPY_H__

#include "datasource.h"

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

// floppy DMA buffer size (512 bytes = 1 sector)
#define FLOPPY_DMALEN       512

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

class FloppyHandler : public BlockDataSource
{
    public:
        FloppyHandler();
        // retrieves floppy drive type
        const char* GetFloppyDriveType(int slot);
        // is floppy drive present in this slot?
        bool HasFloppyInSlot(int slot);
        // Resets IRQ wait flag
        void ResetIRQState();

        // Initializes floppy drive
        int Initialize();

        // jumps to offset on floppy (does not seek now)
        int SeekTo(int offset);
        // retrieves current position
        int GetPosition();
        // reads bytes from current location on floppy
        int ReadBytes(char* target, int count);
        // writes bytes to current location on floppy
        int WriteBytes(char* source, int count);

    protected:
        // detects floppy drives present in system
        void DetectDrives();
        // start floppy motor
        void MotorOn();
        // stop floppy motor
        void MotorOff();
        // send command to floppy
        void WriteCmd(char cmd);
        // reads data from floppy
        unsigned char ReadData();
        // sends data to floppy and awaits status bytes
        void CheckInterrupt(int *st0, int *cyl);
        // seek to specified cylinder on specified head
        int Seek(int cyli, int head);
        // calibrates floppy, seeks to sector 0
        int Calibrate();
        // resets floppy state (at startup)
        int Reset();
        // init DMA channel for floppy transfer; read = 1 for reading, 0 for writing
        void DMAInit(int read);
        // reads/writes onto specific location using DMA
        int DoTrack(int cyl, int read);
        // reads track into DMA buffer
        int ReadTrack(int cyl);
        // writes track from DMA buffer
        int WriteTrack(int cyl);

    private:
        // IRQ status
        int m_floppy_status;
        // current absolute offset
        int m_floppy_current_offset;
        // current track
        int m_floppy_current_track;
        // current offset within one track
        int m_floppy_current_track_offset;
        // stored floppy drive types
        int m_floppy_drives_present[2];

        int m_seek_track;
        bool m_seek_read;
};

extern FloppyHandler sFloppy;

#endif

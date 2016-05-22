#include "floppy.h"
#include "pic.h"
#include "idt.h"
#include "support.h"
#include "pit.h"
#include "irq_handlers.h"

#include "console.h"

FloppyHandler sFloppy;

// seek status
static const char* __floppy_seek_status[] = { "normal", "error", "invalid", "drive" };
// sector use status
static const char* __floppy_sector_status[] = { 0, "error", "invalid command", "drive not ready" };

// DMA buffer aligned to 32K
static char floppy_dmabuf[FLOPPY_DMALEN] __attribute__((aligned(0x8000)));

// floppy drive types, supported only 1.44MB
static const char* __floppy_drive_types[MAX_FLOPPY_TYPES] = {
    "No floppy drive.",
    "360KB 5.25in",
    "1.2MB 5.25in",
    "720KB 3.5in",
    "1.44MB 3.5in",
    "2.88MB 3.5in"
};

// invalid floppy slot text
static const char* __floppy_drive_invalid_slot = "Invalid floppy slot";
// invalid drive text
static const char* __floppy_drive_unknown = "Unknown floppy drive";

FloppyHandler::FloppyHandler()
{
    m_floppy_status = 0;
    m_floppy_current_offset = 0;
    m_floppy_current_track = 0;
    m_floppy_current_track_offset = 0;
}

void FloppyHandler::DetectDrives()
{
    unsigned char c;
    outb(0x70, 0x10);
    c = inb(0x71);

    m_floppy_drives_present[0] = c >> 4;
    m_floppy_drives_present[1] = c & 0xF;
}

const char* FloppyHandler::GetFloppyDriveType(int slot)
{
    if (slot < 0 || slot > 1)
        return __floppy_drive_invalid_slot;

    if (m_floppy_drives_present[slot] < 0 || m_floppy_drives_present[slot] >= MAX_FLOPPY_TYPES)
        return __floppy_drive_unknown;

    return __floppy_drive_types[m_floppy_drives_present[slot]];
}

void FloppyHandler::ResetIRQState()
{
    m_floppy_status = 0;
}

// floppy IRQ6 handler
extern "C" void __floppy_irq_handler()
{
    sFloppy.ResetIRQState();

    send_eoi(6);
}

void FloppyHandler::MotorOn()
{
   outb(FLOPPY_DOR_PORT, 0x1C);
}

void FloppyHandler::MotorOff()
{
   outb(FLOPPY_DOR_PORT, 0x00);
}

void FloppyHandler::WriteCmd(char cmd)
{
    int i;

    // max. 5 seconds timeout
    for (i = 0; i < 500; i++)
    {
        wait_ticks(10);
        if (inb(FLOPPY_MSR_PORT) & 0x80)
        {
            outb(FLOPPY_FIFO_PORT, cmd);
            return;
        }
    }

    Console::WriteLn("floppy_write_cmd: timeout");
}

unsigned char FloppyHandler::ReadData()
{
    int i;

    // max. 5 seconds timeout
    for (i = 0; i < 500; i++)
    {
        wait_ticks(10);
        if (inb(FLOPPY_MSR_PORT) & 0x80)
            return inb(FLOPPY_FIFO_PORT);
    }

    Console::WriteLn("floppy_read_data: timeout");
    return 0;
}

void FloppyHandler::CheckInterrupt(int *st0, int *cyl)
{
    WriteCmd(FLOPPY_CMD_SENSE_INTERRUPT);

    *st0 = ReadData();
    *cyl = ReadData();
}

int FloppyHandler::Seek(int cyli, int head)
{
    int i, st0, cyl; // set to bogus cylinder

    MotorOn();

    for (i = 0; i < 10; i++)
    {
        // set flag to 1
        m_floppy_status = 1;

        // move to desired cylinder
        WriteCmd(FLOPPY_CMD_SEEK);
        // 1. byte - bit 0,1 = drive, bit 2 = head
        WriteCmd(head << 2);
        // 2. byte - cylinder number
        WriteCmd(cyli);

        // wait for interrupt
        while (m_floppy_status == 1)
        {
            while (m_floppy_status == 1)
                ;
        }
        CheckInterrupt(&st0, &cyl);

        // not ready
        if (st0 & 0xC0)
        {
            Console::Write("floppy_seek: status = ");
            Console::WriteLn(__floppy_seek_status[st0 >> 6]);
            continue;
        }

        // have we reached desired cylinder?
        if (cyl == cyli)
        {
            MotorOff();
            return 0;
        }

    }

    Console::WriteLn("floppy_seek: 10 retries exhausted");
    MotorOff();
    return -1;
}

int FloppyHandler::Calibrate()
{
    int i, st0, cyl = -1;

    MotorOn();

    // 10 attempts to calibrate
    for (i = 0; i < 10; i++)
    {
        // set flag to 1
        m_floppy_status = 1;

        // move head to cylinder 0
        WriteCmd(FLOPPY_CMD_RECALIBRATE);
        WriteCmd(0); // argument is drive, we only support 0

        // wait for interrupt
        while (m_floppy_status == 1)
        {
            while (m_floppy_status == 1)
                ;
        }
        CheckInterrupt(&st0, &cyl);

        // not ready, device busy, missing or failing
        if (st0 & 0xC0)
        {
            Console::Write("floppy_calibrate: status = ");
            Console::WriteLn(__floppy_seek_status[st0 >> 6]);
            continue;
        }

        // found cylinder 0 ? then we are done
        if (!cyl)
        {
            MotorOff();
            return 0;
        }
    }

    Console::WriteLn("floppy_calibrate: 10 retries exhausted");
    MotorOff();
    return -1;
}

int FloppyHandler::Reset()
{
    int st0, cyl;

    // set the flag
    m_floppy_status = 1;

    // send reset bytes
    outb(FLOPPY_DOR_PORT, 0x00);
    outb(FLOPPY_DOR_PORT, 0x0C);

    // wait for the interrupt
    while (m_floppy_status == 1)
    {
        while (m_floppy_status == 1)
            ;
    }

    // wait for interrupt
    CheckInterrupt(&st0, &cyl);

    // transfer speed = 500kb/s
    outb(FLOPPY_CCR_PORT, 0x00);

    // reset steprate, unload time, load time, no-DMA
    WriteCmd(FLOPPY_CMD_SPECIFY);
    WriteCmd(0xDF);
    WriteCmd(0x02);

    // finally, recalibrate
    if (Calibrate())
        return -1;

    return 0;
}

void FloppyHandler::DMAInit(int read)
{
    unsigned char mode;

    union
    {
        unsigned char b[4];
        unsigned long l;
    } a, c; // address and count

    a.l = (unsigned long)(floppy_dmabuf);
    c.l = (unsigned int)FLOPPY_DMALEN - 1; // -1 because of DMA counting

    if (read == 1)
        mode = 0x46;    // 01:0:0:01:10 = single/inc/no-auto/to-mem/chan2
    else
        mode = 0x4A;    // 01:0:0:10:10 = single/inc/no-auto/from-mem/chan2

    outb(0x0a, 0x06);   // mask DMA channel 2

    outb(0x0c, 0xff);   // reset flip-flop
    outb(0x04, a.b[0]); //  - address low byte
    outb(0x04, a.b[1]); //  - address high byte

    outb(0x81, a.b[2]); // external page register

    outb(0x0c, 0xff);   // reset flip-flop
    outb(0x05, c.b[0]); //  - count low byte
    outb(0x05, c.b[1]); //  - count high byte

    outb(0x0b, mode);   // set mode (see above)

    outb(0x0a, 0x02);   // unmask chan 2
}

int FloppyHandler::DoTrack(int cyl, int read)
{
    unsigned char cmd;

    // Read is MT:MF:SK:0:0:1:1:0, write MT:MF:0:0:1:0:1
    // where MT = multitrack, MF = MFM mode, SK = skip deleted
    //
    // Specify multitrack and MFM mode
    static const int flags = 0xC0;
    if (read == 1)
        cmd = FLOPPY_CMD_READ_DATA | flags;
    else
        cmd = FLOPPY_CMD_WRITE_DATA | flags;

    // seek both heads
    if (Seek(cyl, 0) || Seek(cyl, 1))
        return -1;

    int i;
    // 20 retries
    for (i = 0; i < 20; i++)
    {
        MotorOn();

        // set the flag
        m_floppy_status = 1;

        // init dma for specified transfer mode
        DMAInit(read);

        wait_ticks(100);        // give some time (100ms) to settle after the seeks

        WriteCmd(cmd);  // set above for current direction
        WriteCmd(0);    // 0:0:0:0:0:HD:US1:US0 = head and drive
        WriteCmd(cyl);  // cylinder
        WriteCmd(0);    // first head (should match with above)
        WriteCmd(1);    // first sector, strangely counts from 1
        WriteCmd(2);    // bytes/sector, 128*2^x (x=2 -> 512)
        WriteCmd(18);   // number of tracks to operate on
        WriteCmd(0x1B); // GAP3 length, 27 is default for 3.5"
        WriteCmd(0xFF); // data length (0xFF if B/S != 0)

        // wait for the interrupt
        while (m_floppy_status == 1)
        {
            while (m_floppy_status == 1)
                ;
        }

        // first read status information
        unsigned char st0, st1, st2, /*rcy, rhe, rse,*/ bps;
        st0 = ReadData();
        st1 = ReadData();
        st2 = ReadData();
        // skip additional cylinder/head/sector data
        /*rcy = */ReadData();
        /*rhe = */ReadData();
        /*rse = */ReadData();
        // bytes per sector, should be what we programmed in
        bps = ReadData();

        int error = 0;

        if (st0 & 0xC0)
        {
            Console::Write("floppy_do_sector: status = ");
            Console::WriteLn(__floppy_sector_status[st0 >> 6]);
            error = 1;
        }
        if (st1 & 0x80)
        {
            Console::WriteLn("floppy_do_sector: end of cylinder");
            error = 1;
        }
        if (st0 & 0x08)
        {
            Console::WriteLn("floppy_do_sector: drive not ready");
            error = 1;
        }
        if (st1 & 0x20)
        {
            Console::WriteLn("floppy_do_sector: CRC error");
            error = 1;
        }
        if (st1 & 0x10)
        {
            Console::WriteLn("floppy_do_sector: controller timeout");
            error = 1;
        }
        if (st1 & 0x04)
        {
            Console::WriteLn("floppy_do_sector: no data found");
            error = 1;
        }
        if ((st1|st2) & 0x01)
        {
            Console::WriteLn("floppy_do_sector: no address mark found");
            error = 1;
        }
        if (st2 & 0x40)
        {
            Console::WriteLn("floppy_do_sector: deleted address mark");
            error = 1;
        }
        if (st2 & 0x20)
        {
            Console::WriteLn("floppy_do_sector: CRC error in data");
            error = 1;
        }
        if (st2 & 0x10)
        {
            Console::WriteLn("floppy_do_sector: wrong cylinder");
            error = 1;
        }
        if (st2 & 0x04)
        {
            Console::WriteLn("floppy_do_sector: uPD765 sector not found");
            error = 1;
        }
        if (st2 & 0x02)
        {
            Console::WriteLn("floppy_do_sector: bad cylinder");
            error = 1;
        }
        if (bps != 0x2)
        {
            Console::WriteLn("floppy_do_sector: wanted 512B/sector, got another");
            error = 1;
        }
        if (st1 & 0x02)
        {
            Console::WriteLn("floppy_do_sector: not writable");
            error = 2;
        }

        // no error = we successfully tracked to desired position
        if (!error)
        {
            MotorOff();
            return 0;
        }

        if(error > 1)
        {
            Console::WriteLn("floppy_do_sector: fatal error occurred, not retrying");
            MotorOff();
            return -2;
        }
    }

    Console::WriteLn("floppy_do_sector: 20 retries exhausted");
    MotorOff();
    return -1;
}

int FloppyHandler::ReadTrack(int cyl)
{
    return DoTrack(cyl, 1);
}

int FloppyHandler::WriteTrack(int cyl)
{
    return DoTrack(cyl, 0);
}

int FloppyHandler::SeekTo(int offset)
{
    // maximum offset on 1.44M floppy
    if (offset >= 1509904)
        return -1;

    m_floppy_current_track = offset / FLOPPY_DMALEN;
    m_floppy_current_offset = offset;
    m_floppy_current_track_offset = offset % FLOPPY_DMALEN;

    return 0;
}

int FloppyHandler::ReadBytes(char* target, int count)
{
    int i;

    // read current track into DMA
    ReadTrack(m_floppy_current_track);

    // secure offset
    if (m_floppy_current_offset + count >= 1509904)
        return -1;

    // read while there's something to read
    for (i = 0; i < count; i++)
    {
        // copy from buffer
        target[i] = floppy_dmabuf[m_floppy_current_track_offset++];

        // if we are at the end of buffer
        if (m_floppy_current_track_offset >= FLOPPY_DMALEN)
        {
            // move to next track, reset offset, and read next track
            m_floppy_current_track++;
            m_floppy_current_track_offset = 0;
            ReadTrack(m_floppy_current_track);
        }

        // move absolute offset
        m_floppy_current_offset++;
    }

    return 0;
}

int FloppyHandler::WriteBytes(char* source, int count)
{
    int i;

    // read track to not lose data
    ReadTrack(m_floppy_current_track);

    // secure count
    if (m_floppy_current_offset + count >= 1509904)
        return -1;

    // write while there's something to write
    for (i = 0; i < count; i++)
    {
        // copy to DMA buffer
        floppy_dmabuf[m_floppy_current_track_offset++] = source[i];

        // if we are at the end of buffer
        if (m_floppy_current_track_offset >= FLOPPY_DMALEN)
        {
            // flush buffer to floppy
            WriteTrack(m_floppy_current_track);
            // move to next track, reset track offset
            m_floppy_current_track++;
            m_floppy_current_track_offset = 0;
            // and refresh buffer data to not lose any
            ReadTrack(m_floppy_current_track);
        }

        // move to next
        m_floppy_current_offset++;
    }

    // flush DMA buffer to floppy
    WriteTrack(m_floppy_current_track);

    return 0;
}

int FloppyHandler::Initialize()
{
    m_floppy_status = 0;

    __use_irq(6, handle_floppy_irq);

    // enable IRQ 6
    __enable_irq(6);

    DetectDrives();

    // reset floppy state
    if (Reset() != 0)
        return -1;

    return 0;
}

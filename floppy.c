#include "floppy.h"
#include "pic.h"
#include "idt.h"
#include "support.h"
#include "pit.h"

#include "framebuffer.h"

// IRQ status
static int __floppy_status = 0;

// current absolute offset
static int __floppy_current_offset = 0;
// current track
static int __floppy_current_track = 0;
// current offset within one track
static int __floppy_current_track_offset = 0;

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

// stored floppy drive types
static int __floppy_drives_present[2];

// detects floppy drives present in system
static void __floppy_detect()
{
    unsigned char c;
    outb(0x70, 0x10);
    c = inb(0x71);

    __floppy_drives_present[0] = c >> 4;
    __floppy_drives_present[1] = c & 0xF;
}

// retrieves floppy drive type
const char* get_floppy_type(int slot)
{
    if (slot < 0 || slot > 1)
        return __floppy_drive_invalid_slot;

    if (__floppy_drives_present[slot] < 0 || __floppy_drives_present[slot] >= MAX_FLOPPY_TYPES)
        return __floppy_drive_unknown;

    return __floppy_drive_types[__floppy_drives_present[slot]];
}

// floppy IRQ6 handler
static void __floppy_irq_handler()
{
    INT_ROUTINE_BEGIN();

    __floppy_status = 0;

    send_eoi(6);

    INT_ROUTINE_END();
}

// start floppy motor
static void __floppy_motor_on()
{
   outb(FLOPPY_DOR_PORT, 0x1C);
}

// stop floppy motor
static void __floppy_motor_off()
{
   outb(FLOPPY_DOR_PORT, 0x00);
}

// send command to floppy
static void __floppy_write_cmd(char cmd)
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

    echo("floppy_write_cmd: timeout\n");
}

// reads data from floppy
unsigned char __floppy_read_data()
{

    int i;

    // max. 5 seconds timeout
    for (i = 0; i < 500; i++)
    {
        wait_ticks(10);
        if (inb(FLOPPY_MSR_PORT) & 0x80)
            return inb(FLOPPY_FIFO_PORT);
    }

    echo("floppy_read_data: timeout\n");
    return 0;
}

// sends data to floppy and awaits status bytes
static void __floppy_check_interrupt(int *st0, int *cyl)
{
    __floppy_write_cmd(FLOPPY_CMD_SENSE_INTERRUPT);

    *st0 = __floppy_read_data();
    *cyl = __floppy_read_data();
}

// seek to specified cylinder on specified head
static int __floppy_seek(int cyli, int head)
{
    int i, st0, cyl; // set to bogus cylinder

    __floppy_motor_on();

    for (i = 0; i < 10; i++)
    {
        // set flag to 1
        __floppy_status = 1;

        // move to desired cylinder
        __floppy_write_cmd(FLOPPY_CMD_SEEK);
        // 1. byte - bit 0,1 = drive, bit 2 = head
        __floppy_write_cmd(head << 2);
        // 2. byte - cylinder number
        __floppy_write_cmd(cyli);

        // wait for interrupt
        while (__floppy_status == 1)
        {
            while (__floppy_status == 1)
                ;
        }
        __floppy_check_interrupt(&st0, &cyl);

        // not ready
        if (st0 & 0xC0)
        {
            echo("floppy_seek: status = ");
            echo(__floppy_seek_status[st0 >> 6]);
            echo("\n");
            continue;
        }

        // have we reached desired cylinder?
        if (cyl == cyli)
        {
            __floppy_motor_off();
            return 0;
        }

    }

    echo("floppy_seek: 10 retries exhausted\n");
    __floppy_motor_off();
    return -1;
}

// calibrates floppy, seeks to sector 0
static int __floppy_calibrate()
{
    int i, st0, cyl = -1;

    __floppy_motor_on();

    // 10 attempts to calibrate
    for (i = 0; i < 10; i++)
    {
        // set flag to 1
        __floppy_status = 1;

        // move head to cylinder 0
        __floppy_write_cmd(FLOPPY_CMD_RECALIBRATE);
        __floppy_write_cmd(0); // argument is drive, we only support 0

        // wait for interrupt
        while (__floppy_status == 1)
        {
            while (__floppy_status == 1)
                ;
        }
        __floppy_check_interrupt(&st0, &cyl);

        // not ready, device busy, missing or failing
        if (st0 & 0xC0)
        {
            echo("floppy_calibrate: status = ");
            echo(__floppy_seek_status[st0 >> 6]);
            echo("\n");
            continue;
        }

        // found cylinder 0 ? then we are done
        if (!cyl)
        {
            __floppy_motor_off();
            return 0;
        }
    }

    echo("floppy_calibrate: 10 retries exhausted\n");
    __floppy_motor_off();
    return -1;
}

// resets floppy state (at startup)
static int __floppy_reset()
{
    int st0, cyl;

    // set the flag
    __floppy_status = 1;

    // send reset bytes
    outb(FLOPPY_DOR_PORT, 0x00);
    outb(FLOPPY_DOR_PORT, 0x0C);

    // wait for the interrupt
    while (__floppy_status == 1)
    {
        while (__floppy_status == 1)
            ;
    }

    // wait for interrupt
    __floppy_check_interrupt(&st0, &cyl);

    // transfer speed = 500kb/s
    outb(FLOPPY_CCR_PORT, 0x00);

    // reset steprate, unload time, load time, no-DMA
    __floppy_write_cmd(FLOPPY_CMD_SPECIFY);
    __floppy_write_cmd(0xDF);
    __floppy_write_cmd(0x02);

    // finally, recalibrate
    if (__floppy_calibrate())
        return -1;

    return 0;
}

// init DMA channel for floppy transfer; read = 1 for reading, 0 for writing
static void __floppy_dma_init(int read)
{
    unsigned char mode;

    union
    {
        unsigned char b[4];
        unsigned long l;
    } a, c; // address and count

    a.l = (unsigned int)&floppy_dmabuf;
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

// reads/writes onto specific location using DMA
static int __floppy_do_track(int cyl, int read)
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
    if (__floppy_seek(cyl, 0) || __floppy_seek(cyl, 1))
        return -1;

    int i;
    // 20 retries
    for (i = 0; i < 20; i++)
    {
        __floppy_motor_on();

        // set the flag
        __floppy_status = 1;

        // init dma for specified transfer mode
        __floppy_dma_init(read);

        wait_ticks(100);        // give some time (100ms) to settle after the seeks

        __floppy_write_cmd(cmd);  // set above for current direction
        __floppy_write_cmd(0);    // 0:0:0:0:0:HD:US1:US0 = head and drive
        __floppy_write_cmd(cyl);  // cylinder
        __floppy_write_cmd(0);    // first head (should match with above)
        __floppy_write_cmd(1);    // first sector, strangely counts from 1
        __floppy_write_cmd(2);    // bytes/sector, 128*2^x (x=2 -> 512)
        __floppy_write_cmd(18);   // number of tracks to operate on
        __floppy_write_cmd(0x1B); // GAP3 length, 27 is default for 3.5"
        __floppy_write_cmd(0xFF); // data length (0xFF if B/S != 0)

        // wait for the interrupt
        while (__floppy_status == 1)
        {
            while (__floppy_status == 1)
                ;
        }

        // first read status information
        unsigned char st0, st1, st2, /*rcy, rhe, rse,*/ bps;
        st0 = __floppy_read_data();
        st1 = __floppy_read_data();
        st2 = __floppy_read_data();
        // skip additional cylinder/head/sector data
        /*rcy = */__floppy_read_data();
        /*rhe = */__floppy_read_data();
        /*rse = */__floppy_read_data();
        // bytes per sector, should be what we programmed in
        bps = __floppy_read_data();

        int error = 0;

        if (st0 & 0xC0)
        {
            echo("floppy_do_sector: status = ");
            echo(__floppy_sector_status[st0 >> 6]);
            echo("\n");
            error = 1;
        }
        if (st1 & 0x80)
        {
            echo("floppy_do_sector: end of cylinder\n");
            error = 1;
        }
        if (st0 & 0x08)
        {
            echo("floppy_do_sector: drive not ready\n");
            error = 1;
        }
        if (st1 & 0x20)
        {
            echo("floppy_do_sector: CRC error\n");
            error = 1;
        }
        if (st1 & 0x10)
        {
            echo("floppy_do_sector: controller timeout\n");
            error = 1;
        }
        if (st1 & 0x04)
        {
            echo("floppy_do_sector: no data found\n");
            error = 1;
        }
        if ((st1|st2) & 0x01)
        {
            echo("floppy_do_sector: no address mark found\n");
            error = 1;
        }
        if (st2 & 0x40)
        {
            echo("floppy_do_sector: deleted address mark\n");
            error = 1;
        }
        if (st2 & 0x20)
        {
            echo("floppy_do_sector: CRC error in data\n");
            error = 1;
        }
        if (st2 & 0x10)
        {
            echo("floppy_do_sector: wrong cylinder\n");
            error = 1;
        }
        if (st2 & 0x04)
        {
            echo("floppy_do_sector: uPD765 sector not found\n");
            error = 1;
        }
        if (st2 & 0x02)
        {
            echo("floppy_do_sector: bad cylinder\n");
            error = 1;
        }
        if (bps != 0x2)
        {
            echo("floppy_do_sector: wanted 512B/sector, got another\n");
            error = 1;
        }
        if (st1 & 0x02)
        {
            echo("floppy_do_sector: not writable\n");
            error = 2;
        }

        // no error = we successfully tracked to desired position
        if (!error)
        {
            __floppy_motor_off();
            return 0;
        }

        if(error > 1)
        {
            echo("floppy_do_sector: fatal error occurred, not retrying\n");
            __floppy_motor_off();
            return -2;
        }
    }

    echo("floppy_do_sector: 20 retries exhausted\n");
    __floppy_motor_off();
    return -1;
}

// reads track into DMA buffer
static int __floppy_read_track(int cyl)
{
    return __floppy_do_track(cyl, 1);
}

// writes track from DMA buffer
static int __floppy_write_track(int cyl)
{
    return __floppy_do_track(cyl, 0);
}

// jumps to offset on floppy (does not seek now)
int floppy_jump_to_offset(int offset)
{
    // maximum offset on 1.44M floppy
    if (offset >= 1509904)
        return -1;

    __floppy_current_track = offset / FLOPPY_DMALEN;
    __floppy_current_offset = offset;
    __floppy_current_track_offset = offset % FLOPPY_DMALEN;

    return 0;
}

// reads bytes from current location on floppy
int floppy_read_bytes(int count, char* target)
{
    int i;

    // read current track into DMA
    __floppy_read_track(__floppy_current_track);

    // secure offset
    if (__floppy_current_offset + count >= 1509904)
        return -1;

    // read while there's something to read
    for (i = 0; i < count; i++)
    {
        // copy from buffer
        target[i] = floppy_dmabuf[__floppy_current_track_offset++];

        // if we are at the end of buffer
        if (__floppy_current_track_offset >= FLOPPY_DMALEN)
        {
            // move to next track, reset offset, and read next track
            __floppy_current_track++;
            __floppy_current_track_offset = 0;
            __floppy_read_track(__floppy_current_track);
        }

        // move absolute offset
        __floppy_current_offset++;
    }

    return 0;
}

// writes bytes to current location on floppy
int floppy_write_bytes(int count, char* source)
{
    int i;

    // read track to not lose data
    __floppy_read_track(__floppy_current_track);

    // secure count
    if (__floppy_current_offset + count >= 1509904)
        return -1;

    // write while there's something to write
    for (i = 0; i < count; i++)
    {
        // copy to DMA buffer
        floppy_dmabuf[__floppy_current_track_offset++] = source[i];

        // if we are at the end of buffer
        if (__floppy_current_track_offset >= FLOPPY_DMALEN)
        {
            // flush buffer to floppy
            __floppy_write_track(__floppy_current_track);
            // move to next track, reset track offset
            __floppy_current_track++;
            __floppy_current_track_offset = 0;
            // and refresh buffer data to not lose any
            __floppy_read_track(__floppy_current_track);
        }

        // move to next
        __floppy_current_offset++;
    }

    // flush DMA buffer to floppy
    __floppy_write_track(__floppy_current_track);

    return 0;
}

int __init_floppy()
{
    __floppy_status = 0;

    __use_irq(6, __floppy_irq_handler);

    // enable IRQ 6
    __enable_irq(6);

    __floppy_detect();

    // reset floppy state
    if (__floppy_reset() != 0)
        return -1;

    return 0;
}

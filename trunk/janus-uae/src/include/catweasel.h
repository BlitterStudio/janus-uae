#ifndef __CATWEASEL_H__
#define __CATWEASEL_H__


#ifdef CATWEASEL

extern struct catweasel_contr cwc;
extern int catweasel_read_keyboard (uae_u8 *keycode);
extern int catweasel_init (void);
extern void catweasel_free (void);
extern uae_u32 catweasel_do_bget (uaecptr addr);
extern void catweasel_do_bput (uaecptr addr, uae_u32 b);
extern int catweasel_read_joystick (int stick, uae_u8 *dir, uae_u8 *buttons);
extern void catweasel_hsync (void);

typedef struct catweasel_drive {
    struct catweasel_contr *contr; /* The controller this drive belongs to */
    int number;                    /* Drive number: 0 or 1 */
    int type;                      /* 0 = not present, 1 = 3.5" */
    int track;                     /* current r/w head position (0..79) */
    int diskindrive;               /* 0 = no disk, 1 = disk in drive */
    int wprot;                     /* 0 = not, 1 = write protected */
    unsigned char sel;
    unsigned char mot;
} catweasel_drive;

typedef struct catweasel_contr {
    int type;                      /* see CATWEASEL_TYPE_* defines below */
    int iobase;                    /* 0 = not present (factory default is 0x320) */
    void (*msdelay)(int ms);       /* microseconds delay routine, provided by host program */
    catweasel_drive drives[2];     /* at most two drives on each controller */
    int control_register;          /* contents of control register */
    unsigned char crm_sel0;        /* bit masks for the control / status register */
    unsigned char crm_sel1;
    unsigned char crm_mot0;
    unsigned char crm_mot1;
    unsigned char crm_dir;
    unsigned char crm_step;
    unsigned char srm_trk0;
    unsigned char srm_dchg;
    unsigned char srm_writ;
    unsigned char srm_dskready;
    int io_sr;                     /* IO port of control / status register */
    int io_mem;                    /* IO port of memory register */
} catweasel_contr;

#define CATWEASEL_TYPE_NONE  -1
#define CATWEASEL_TYPE_MK1    1
#define CATWEASEL_TYPE_MK3    3
#define CATWEASEL_TYPE_MK4    4

/* Initialize a Catweasel controller; c->iobase and c->msdelay must have
   been initialized -- msdelay might be used */
void catweasel_init_controller(catweasel_contr *c);

/* Reset the controller */
void catweasel_free_controller(catweasel_contr *c);

/* Set current drive select mask */
void catweasel_select(catweasel_contr *c, int dr0, int dr1);

/* Start/stop the drive's motor */
void catweasel_set_motor(catweasel_drive *d, int on);

/* Move the r/w head */
int catweasel_step(catweasel_drive *d, int dir);

/* Check for a disk change and update d->diskindrive
   -- msdelay might be used. Returns 1 == disk has been changed */
int catweasel_disk_changed(catweasel_drive *d);

/* Check if disk in selected drive is write protected. */
int catweasel_write_protected(catweasel_drive *d);

/* Read data -- msdelay will be used */
int catweasel_read(catweasel_drive *d, int side, int clock, int time);

/* Write data -- msdelay will be used. If time == -1, the write will
   be started at the index pulse and stopped at the next index pulse,
   or earlier if the Catweasel RAM contains a 128 end byte.  The
   function returns after the write has finished. */
int catweasel_write(catweasel_drive *d, int side, int clock, int time);

int catweasel_fillmfm (catweasel_drive *d, uae_u16 *mfm, int side, int clock, int rawmode);

int catweasel_diskready(catweasel_drive *d);
int catweasel_track0(catweasel_drive *d);
#define CW_VENDOR       0xe159
#define CW_DEVICE       0x0001
#define CW_MK3_SUBVENDOR    0x1212
#define CW_MK3_SUBDEVICE    0x0002
#define CW_MK4_SUBVENDOR1   0x5213
#define CW_MK4_SUBVENDOR2   0x5200
#define CW_MK4_SUBDEVICE1   0x0002
#define CW_MK4_SUBDEVICE2   0x0003

// generic registers 
#define CW_DATA_DIRECTION   0x02
#define CW_SELECT_BANK      0x03
#define CW_PORT_OUT_DIR     CW_SELECT_BANK
#define CW_PORT_AUX     0x05
#define CW_PORT_IN_DIR      0x07

// values for CW_SELECT_BANK 
#define CW_BANK_RESETFPGA   0x00
#define CW_BANK_FIFO        0x01
#define CW_BANK_FLOPPY      0x41
#define CW_BANK_IO      0x81
#define CW_BANK_IRQ     0xC1

// registers in FLOPPY bank 
#define CW_FLOPPY_JOYDAT    0xC0
#define CW_FLOPPY_JOYBUT    0xC8
#define CW_FLOPPY_JOYBUT_DIR    0xCC
#define CW_FLOPPY_KEYDAT    0xD0
#define CW_FLOPPY_KEYSTATUS 0xD4
#define CW_FLOPPY_SIDDAT    0xD8
#define CW_FLOPPY_SIDADDR   0xDC
#define CW_FLOPPY_MEMORY    0xE0
#define CW_FLOPPY_RESETPOINTER  0xE4
#define CW_FLOPPY_CONTROL   0xE8
#define CW_FLOPPY_OPTION    0xEC
#define CW_FLOPPY_START_A   0xF0
#define CW_FLOPPY_START_B   0xF4
#define CW_FLOPPY_IRQ       0xFC

// registers in IO bank 
#define CW_IO_MOUSEY1       0xC0
#define CW_IO_MOUSEX1       0xC4
#define CW_IO_MOUSEY2       0xC8
#define CW_IO_MOUSEX2       0xCC
#define CW_IO_BUTTON        0xD0

// registers in CW_BANK_IRQ 
#define CW_IRQ_R0       0xC0
#define CW_IRQ_R1       0xC4
#define CW_IRQ_M0       0xC8
#define CW_IRQ_M1       0xCC
// bits in registers in CW_BANK_IRQ 
#define CW_IRQ_MK3FLOPPY    0x01
#define CW_IRQ_INDEX        0x02
#define CW_IRQ_FLOPPY_START 0x04
#define CW_IRQ_FLOPPY_END   0x08
#define CW_IRQ_SID_FIFO_HALF_EMPTY  0x10
#define CW_IRQ_SID_READ     0x20
#define CW_IRQ_MEM_EQUAL    0x40
#define CW_IRQ_KEYBOARD     0x80
#define CW_IRQ_SID_FIFO_EMPTY   0x01
#define CW_IRQ_SID_FEEDBACK 0x02

#endif

#endif

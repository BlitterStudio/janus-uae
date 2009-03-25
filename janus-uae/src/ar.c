/*
 * UAE Action Replay 1/2/3 and HRTMon support
 *
 * (c) 2000-2002 Toni Wilen <twilen@arabuusimiehet.com>
 * (c) 2003 Mark Cox <markcox@email.com>
 *
 * Toni's unofficial UAE patches are located at:
 * http://www.arabuusimiehet.com/twilen/uae/
 *
 * HRTMon:
 *
 * HRTMon support is tested with version 2.25 + patch.
 * More information about HRTMon can be found from
 * http://dumbo.cryogen.ch/hrtmon/
 *
 * Action Replay 2/3:
 *
 * Tested with AR3 ROM version 3.09 (10/13/91) and AR2 2.12 (12/24/90)
 *
 * Found to work for the following roms by Mark Cox:
 * (Yes the date format is inconsistent, i just copied it straight from the rom)
 * 1.15
 * 2.14 22/02/91 dd/mm/yy
 * 3.09 10/13/91 mm/dd/yy
 * 3.17 12/17/91 mm/dd/yy
 *
 * This patch also makes AR3 compatible with KickStart's other than 1.3
 * (ROM checksum error is normal with KS != 1.3)
 * NOTE: AR has problems with 68020+ processors.
 * For maximum compatibility select 68000/68010 and A500 speed from UAE
 * options.
 *
 * How to rip Action Replay 1/2/3 ROM:
 *
 * Find A500 with AR1/2/3, press 'freeze'-button
 *
 * type following:
 *
 * AR1:
 * lord olaf<RETURN>
 *
 * AR2 or AR3:
 * may<RETURN>
 * the<RETURN>
 * force<RETURN>
 * be<RETURN>
 * with<RETURN>
 * you<RETURN>
 * new<RETURN> (AR3 only)
 *
 * AR1: 64K ROM is visible at 0xf00000-0xf0ffff
 * and 16K RAM at 0x9fc000-0x9fffff
 * AR2: 128K ROM is visible at 0x400000-0x41ffff
 * AR3: 256K ROM is visible at 0x400000-0x43ffff
 * and 64K RAM at 0x440000-0x44ffff
 *
 * following command writes ROM to disk:
 *
 * AR1: sm ar1.rom,f00000 f10000
 * AR2: sm ar2.rom,400000 420000
 * AR3: sm ar3.rom,400000 440000
 *
 * NOTE: I (mark) could not get the action replay 1 dump to work as above.
 * (also, it will only dump to the action replay special disk format)
 * To dump the rom i had to :
 * 1. Boot the a500 and start a monitor (e.g. cmon).
 * 2. Use the monitor to allocate 64k memory.
 * 3. Enter the action replay.
 * 4. Enter sysop mode.
 * 5. Copy the rom into the address the monitor allocated.
 * 6. Exit the action replay.
 * 7. Save the ram from the monitor to disk.
 *
 * I DO NOT REPLY MAILS ASKING FOR ACTION REPLAY ROMS!
 *
 * AR2/3 hardware notes (not 100% correct..)
 *
 * first 8 bytes of ROM are not really ROM, they are
 * used to read/write cartridge's hardware state
 *
 * 0x400000: hides cartridge ROM/RAM when written
 * 0x400001: read/write HW state
 *  3 = reset (read-only)
 *  2 = sets HW to activate when breakpoint condition is detected
 *  1 = ???
 *  0 = freeze pressed
 * 0x400002/0x400003: mirrors 0x400000/0x400001
 * 0x400006/0x400007: when written to, turns chip-ram overlay off
 *
 * breakpoint condition is detected when CPU first accesses
 * chip memory below 1024 bytes and then reads CIA register
 * $BFE001.
 *
 * cartridge hardware also snoops CPU accesses to custom chip
 * registers (DFF000-DFF1FE). All CPU custom chip accesses are
 * saved to RAM at 0x44f000-0x44f1ff. Note that emulated AR3 also
 * saves copper's custom chip accesses. This fix stops programs
 * that try to trick AR by using copper to update write-only
 * custom registers.
 *
 * 30.04.2001 - added AR2 support
 * 21.07.2001 - patch updated
 * 29.07.2002 - added AR1 support
 * 11.03.2003 - added AR1 breakpoint support, checksum support and fixes. (Mark Cox)
 *
 */

/* AR2/3 'NORES' info.
 * On ar2 there is a 'nores' command,
 * on ar3, it is accessible using the mouse.
 * This command will not work using the current infrastructure,
 * so don't use it 8).
 */

/* AR1 Breakpoint info.
 * 1.15 If a breakpoint occurred. Its address is stored at 9fe048.
 * The 5 breakpoint entries each consisting of 6 bytes are stored at 9fe23e.
 * Each entry contains the breakpoint long word followed by 2 bytes of the original contents of memory
 * that is replaced by a trap instruction in mem.
 * So the table finishes at 9fe25c.
 */

/* How AR1 is entered on reset:
 * In the kickstart (1.3) there is the following code:
 * I have marked the important lines:
 *
 * fc00e6 lea f00000,a1	; address where AR1 rom is located.
 * fc00ec cmpa.l a1,a0
 * fc00ee beq fc00fe.s
 * fc00f0 lea C(pc), a5
 * fc00f4 cmpi.w #1111,(a1)		; The first word of the AR1 rom is set to 1111.
 * fc00f8 bne fc00fe.s
 * fc00fa jmp 2(a1)						; This is the entry point of the rom.
 */

 /* Flag info:
	* AR3:'ARON'. This is unset initially. It is set the first time you enter the AR via a freeze.
	* It enables you to keep the keyboard buffer and such.
	* If this flag is unset, the keyboard buffer is cleared, the breakpoints are deleted and ... */

 /* AR3:'PRIN'. This flag is unset initially. It is set at some point and when you switch to the 2nd screen
	* for the first time it displays all the familiar text. Then unsets 'PRIN'.
	*/

#ifdef ACTION_REPLAY

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "zfile.h"
#include "ar.h"
#include "savestate.h"

//#define DEBUG
#ifdef DEBUG
#define write_log_debug write_log
#else
#define write_log_debug(...) do {;} while(0)

#endif


#define ARMODE_FREEZE 0 /* AR2/3 The action replay 'freeze' button has been pressed.  */
#define ARMODE_BREAKPOINT_AR2 2 /* AR2: The action replay is activated via a breakpoint. */
#define ARMODE_BREAKPOINT_ACTIVATED 1
#define ARMODE_BREAKPOINT_AR3_RESET_AR2 3 /* AR2: The action replay is activated after a reset. */
                                          /* AR3: The action replay is activated by a breakpoint. */

/* HRTMon baseaddress, can be freely changed */
#define HRTMON_BASE 0x980000

uae_u8 ar_custom[2*256];

int hrtmon_flag = ACTION_REPLAY_INACTIVE;

static uae_u8 *hrtmemory = 0;
static uae_u8 *armemory_rom = 0, *armemory_ram = 0;

static uae_u32 hrtmem_mask;
static uae_u8 *hrtmon_custom;
uae_u32 hrtmem_start, hrtmem_size;

static uae_u32 hrtmem_lget (uaecptr) REGPARAM;
static uae_u32 hrtmem_wget (uaecptr) REGPARAM;
static uae_u32 hrtmem_bget (uaecptr) REGPARAM;
static void  hrtmem_lput (uaecptr, uae_u32) REGPARAM;
static void  hrtmem_wput (uaecptr, uae_u32) REGPARAM;
static void  hrtmem_bput (uaecptr, uae_u32) REGPARAM;
static int  hrtmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *hrtmem_xlate (uaecptr addr) REGPARAM;
static void hrtmon_unmap_banks(void);

void check_prefs_changed_carts(int in_memory_reset);
int action_replay_unload(int in_memory_reset);

static uae_u32 REGPARAM2 hrtmem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    m = (uae_u32 *)(hrtmemory + addr);
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 hrtmem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    m = (uae_u16 *)(hrtmemory + addr);
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 hrtmem_bget (uaecptr addr)
{
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    return hrtmemory[addr];
}

static void REGPARAM2 hrtmem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    m = (uae_u32 *)(hrtmemory + addr);
    do_put_mem_long (m, l);
}

static void REGPARAM2 hrtmem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    m = (uae_u16 *)(hrtmemory + addr);
    do_put_mem_word (m, (uae_u16)w);
}

static void REGPARAM2 hrtmem_bput (uaecptr addr, uae_u32 b)
{
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    hrtmemory[addr] = b;
}

static int REGPARAM2 hrtmem_check (uaecptr addr, uae_u32 size)
{
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    return (addr + size) <= hrtmem_size;
}

static uae_u8 REGPARAM2 *hrtmem_xlate (uaecptr addr)
{
    addr -= hrtmem_start & hrtmem_mask;
    addr &= hrtmem_mask;
    return hrtmemory + addr;
}

addrbank hrtmem_bank = {
    hrtmem_lget, hrtmem_wget, hrtmem_bget,
    hrtmem_lput, hrtmem_wput, hrtmem_bput,
    hrtmem_xlate, hrtmem_check, NULL
};

static void copyfromamiga (uae_u8 *dst, uaecptr src, int len)
{
    while(len--) {
	*dst++ = (uae_u8) get_byte (src);
	src++;
    }
}

static void copytoamiga (uaecptr dst, const uae_u8 *src, int len)
{
    while (len--) {
	put_byte (dst, *src++);
	dst++;
    }
}

int action_replay_flag = ACTION_REPLAY_INACTIVE;
static int ar_rom_file_size;

/* Use this for relocating AR? */
static int ar_rom_location;
/*static*/ int armodel;
static uae_u8 artemp[4]; /* Space to store the 'real' level 7 interrupt */
static uae_u8 armode;

static uae_u32 arrom_start, arrom_size, arrom_mask;
static uae_u32 arram_start, arram_size, arram_mask;

static int ar_wait_pop = 0; /* bool used by AR1 when waiting for the program counter to exit it's ram. */
uaecptr wait_for_pc = 0;	/* The program counter that we wait for. */

/* returns true if the Program counter is currently in the AR rom. */
int is_ar_pc_in_rom()
{
    uaecptr pc = m68k_getpc (&regs) & 0xFFFFFF;
    return pc >= arrom_start && pc < arrom_start+arrom_size;
}

/* returns true if the Program counter is currently in the AR RAM. */
int is_ar_pc_in_ram()
{
    uaecptr pc = m68k_getpc (&regs) & 0xFFFFFF;
    return pc >= arram_start && pc < arram_start+arram_size;
}


/* flag writing == 1 for writing memory, 0 for reading from memory. */
STATIC_INLINE int ar3a (uaecptr addr, uae_u8 b, int writing)
{
    uaecptr pc;
/*	if ( addr < 8 ) //|| writing ) */
/*	{ */
/*		if ( writing ) */
/*   write_log_debug("ARSTATUS armode:%d, Writing %d to address %p, PC=%p\n", armode, b, addr, m68k_getpc (&regs)); */
/*		else */
/*    write_log_debug("ARSTATUS armode:%d, Reading %d from address %p, PC=%p\n", armode, armemory_rom[addr], addr, m68k_getpc (&regs)); */
/*	}	 */

    if (armodel == 1 ) /* With AR1. It is always a read. Actually, it is a strobe on exit of the AR.
			  * but, it is also read during the checksum routine.	*/
    {
    	if ( addr < 2)
    	{
	    if ( is_ar_pc_in_rom() )
	    {
		if ( ar_wait_pop )
		{
		    action_replay_flag = ACTION_REPLAY_WAIT_PC;
/*		    write_log_debug("SP %p\n", m68k_areg (&regs, 7));  */
/*		    write_log_debug("SP+2 %p\n", m68k_areg (&regs, 7)+2 );  */
/*		    write_log_debug("(SP+2) %p\n", longget (m68k_areg (&regs,7)+2));  */
		    ar_wait_pop = 0;
		    /* We get (SP+2) here, as the first word on the stack is the status register. */
		    /* We want the following long, which is the return program counter. */
		    wait_for_pc = longget (m68k_areg (&regs, 7)+2); /* Get (SP+2) */
    		    set_special (&regs, SPCFLAG_ACTION_REPLAY);

    		    pc = m68k_getpc (&regs);
/*      	    write_log_debug("Action Replay marked as ACTION_REPLAY_WAIT_PC, PC=%p\n",pc);*/
		}
		else
		{
    		    uaecptr pc = m68k_getpc (&regs);
/*      	    write_log_debug("Action Replay marked as IDLE, PC=%p\n",pc);*/
		    action_replay_flag = ACTION_REPLAY_IDLE;
		}
	    }
	}
	/* This probably violates the hide_banks thing except ar1 doesn't use that yet .*/
	return armemory_rom[addr];
    }

#ifdef ACTION_REPLAY_HIDE_CARTRIDGE
    if (addr >= 8 )
	return armemory_rom[addr];

    if (action_replay_flag != ACTION_REPLAY_ACTIVE)
	return 0;
#endif

    if (!writing) /* reading */
    {
	if (addr == 1) /* This is necessary because we don't update rom location 0 every time we change armode */
	    return armode;
	else
	    return armemory_rom[addr];
    }
    /* else, we are writing */
    else if (addr == 1) {
        armode = b;
        if(armode >= 2)
	{
	    if ( armode == ARMODE_BREAKPOINT_AR2 )
	    {
            	write_log("AR2: exit with breakpoint(s) active\n"); /* Correct for AR2 */
	    }
	    else if ( armode == ARMODE_BREAKPOINT_AR3_RESET_AR2 )
	    {
		write_log("AR3: exit waiting for breakpoint.\n"); /* Correct for AR3 (waiting for breakpoint)*/
	    }
	    else
	    {
		write_log("AR2/3: mode(%d) > 3 this shouldn't happen.\n", armode);
	    }
	} else {
            write_log("AR: exit with armode(%d)\n", armode);
        }
        set_special (&regs, SPCFLAG_ACTION_REPLAY);
        action_replay_flag = ACTION_REPLAY_HIDE;
    } else if (addr == 6) {
        copytoamiga (regs.vbr + 0x7c, artemp, 4);
        write_log ("AR: chipmem returned\n");
    }
    return 0;
}

void REGPARAM2 chipmem_lput_actionreplay1 (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    if (addr == 0x60 && !is_ar_pc_in_rom())
	action_replay_chipwrite ();
    m = (uae_u32 *)(chipmemory + addr);
    do_put_mem_long (m, l);
}
void REGPARAM2 chipmem_wput_actionreplay1 (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    if (addr == 0x62 && !is_ar_pc_in_rom())
	action_replay_chipwrite ();
    m = (uae_u16 *)(chipmemory + addr);
    do_put_mem_word (m, w);
}
void REGPARAM2 chipmem_bput_actionreplay1 (uaecptr addr, uae_u32 b)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    if (addr >= 0x60 && addr <= 0x63 && !is_ar_pc_in_rom())
	action_replay_chipwrite();
    chipmemory[addr] = b;
}
void REGPARAM2 chipmem_lput_actionreplay23 (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);
    do_put_mem_long (m, l);
    if (addr >= 0x40 && addr < 0x200 && action_replay_flag == ACTION_REPLAY_WAITRESET)
        action_replay_chipwrite();
}
void REGPARAM2 chipmem_wput_actionreplay23 (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u16 *)(chipmemory + addr);
    do_put_mem_word (m, w);
    if (addr >= 0x40 && addr < 0x200 && action_replay_flag == ACTION_REPLAY_WAITRESET)
        action_replay_chipwrite();
}


static uae_u32 arram_lget (uaecptr) REGPARAM;
static uae_u32 arram_wget (uaecptr) REGPARAM;
static uae_u32 arram_bget (uaecptr) REGPARAM;
static void  arram_lput (uaecptr, uae_u32) REGPARAM;
static void  arram_wput (uaecptr, uae_u32) REGPARAM;
static void  arram_bput (uaecptr, uae_u32) REGPARAM;
static int  arram_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *arram_xlate (uaecptr addr) REGPARAM;

static uae_u32 arrom_lget (uaecptr) REGPARAM;
static uae_u32 arrom_wget (uaecptr) REGPARAM;
static uae_u32 arrom_bget (uaecptr) REGPARAM;
static void  arrom_lput (uaecptr, uae_u32) REGPARAM;
static void  arrom_wput (uaecptr, uae_u32) REGPARAM;
static void  arrom_bput (uaecptr, uae_u32) REGPARAM;
static int  arrom_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *arrom_xlate (uaecptr addr) REGPARAM;
static void action_replay_unmap_banks(void);

static uae_u32 action_replay_calculate_checksum(void);
static uae_u8* get_checksum_location(void);
static void disable_rom_test(void);

static uae_u32 REGPARAM2 arram_lget (uaecptr addr)
{
    uae_u32 *m;
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    addr -= arram_start;
    addr &= arram_mask;
    m = (uae_u32 *)(armemory_ram + addr);
    if (strncmp("T8", (char*)m, 2) == 0)
    	write_log_debug("Reading T8 from addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("LAME", (char*)m, 4) == 0)
	write_log_debug("Reading LAME from addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("RES1", (char*)m, 4) == 0)
	write_log_debug("Reading RES1 from addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("ARON", (char*)m, 4) == 0)
	write_log_debug("Reading ARON from addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("KILL", (char*)m, 4) == 0)
	write_log_debug("Reading KILL from addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("BRON", (char*)m, 4) == 0)
	write_log_debug("Reading BRON from addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("PRIN", (char*)m, 4) == 0)
	write_log_debug("Reading PRIN from addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 arram_wget (uaecptr addr)
{
    uae_u16 *m;
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    addr -= arram_start;
    addr &= arram_mask;
    m = (uae_u16 *)(armemory_ram + addr);
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 arram_bget (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    addr -= arram_start;
    addr &= arram_mask;
    return armemory_ram[addr];
}

void REGPARAM2 arram_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;

    addr -= arram_start;
    addr &= arram_mask;
    m = (uae_u32 *)(armemory_ram + addr);
    if (strncmp("T8", (char*)m, 2) == 0)
	write_log_debug("Writing T8 to addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("LAME", (char*)m, 4) == 0)
	write_log_debug("Writing LAME to addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("RES1", (char*)m, 4) == 0)
	write_log_debug("Writing RES1 to addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("ARON", (char*)m, 4) == 0)
	write_log_debug("Writing ARON to addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("KILL", (char*)m, 4) == 0)
	write_log_debug("Writing KILL to addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("BRON", (char*)m, 4) == 0)
	write_log_debug("Writing BRON to addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    if (strncmp("PRIN", (char*)m, 4) == 0)
	write_log_debug("Writing PRIN to addr %08.08x PC=%p\n", addr, m68k_getpc (&regs));
    do_put_mem_long (m, l);
}

void REGPARAM2 arram_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;

#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    addr -= arram_start;
    addr &= arram_mask;
    m = (uae_u16 *)(armemory_ram + addr);
    do_put_mem_word (m, w);
}

void REGPARAM2 arram_bput (uaecptr addr, uae_u32 b)
{
    addr -= arram_start;
    addr &= arram_mask;
    armemory_ram[addr] = b;
}

static int REGPARAM2 arram_check (uaecptr addr, uae_u32 size)
{
    addr -= arram_start;
    addr &= arram_mask;
    return (addr + size) <= arram_size;
}

static uae_u8 REGPARAM2 *arram_xlate (uaecptr addr)
{
    addr -= arram_start;
    addr &= arram_mask;
    return armemory_ram + addr;
}

static uae_u32 REGPARAM2 arrom_lget (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    addr -= arrom_start;
    addr &= arrom_mask;
    return (ar3a (addr, 0, 0) << 24) | (ar3a (addr + 1, 0, 0) << 16) | (ar3a (addr + 2, 0, 0) << 8) | ar3a (addr + 3, 0, 0);
}

static uae_u32 REGPARAM2 arrom_wget (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    addr -= arrom_start;
    addr &= arrom_mask;
    return (ar3a (addr, 0, 0) << 8) | ar3a (addr + 1, 0, 0);
}

static uae_u32 REGPARAM2 arrom_bget (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    addr -= arrom_start;
    addr &= arrom_mask;
    return ar3a (addr, 0, 0);
}

static void REGPARAM2 arrom_lput (uaecptr addr, uae_u32 l)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    addr -= arrom_start;
    addr &= arrom_mask;
    ar3a (addr + 0,(uae_u8)(l >> 24), 1);
    ar3a (addr + 1,(uae_u8)(l >> 16), 1);
    ar3a (addr + 2,(uae_u8)(l >> 8), 1);
    ar3a (addr + 3,(uae_u8)(l >> 0), 1);
}

static void REGPARAM2 arrom_wput (uaecptr addr, uae_u32 w)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    addr -= arrom_start;
    addr &= arrom_mask;
    ar3a (addr + 0,(uae_u8)(w >> 8), 1);
    ar3a (addr + 1,(uae_u8)(w >> 0), 1);
}

static void REGPARAM2 arrom_bput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    addr -= arrom_start;
    addr &= arrom_mask;
    ar3a (addr, b, 1);
}

static int REGPARAM2 arrom_check (uaecptr addr, uae_u32 size)
{
    addr -= arrom_start;
    addr &= arrom_mask;
    return (addr + size) <= arrom_size;
}

static uae_u8 REGPARAM2 *arrom_xlate (uaecptr addr)
{
    addr -= arrom_start;
    addr &= arrom_mask;
    return armemory_rom + addr;
}

static addrbank arrom_bank = {
    arrom_lget, arrom_wget, arrom_bget,
    arrom_lput, arrom_wput, arrom_bput,
    arrom_xlate, arrom_check, NULL
};
static addrbank arram_bank = {
    arram_lget, arram_wget, arram_bget,
    arram_lput, arram_wput, arram_bput,
    arram_xlate, arram_check, NULL
};

static void action_replay_unmap_banks()
{
    if(!armemory_rom)
        return;

    map_banks (&dummy_bank, arrom_start >> 16 , arrom_size >> 16, 0);
    map_banks (&dummy_bank, arram_start >> 16 , arram_size >> 16, 0);
}

void action_replay_map_banks()
{
    if(!armemory_rom)
        return;

    map_banks (&arrom_bank, arrom_start >> 16, arrom_size >> 16, 0);
    map_banks (&arram_bank, arram_start >> 16, arram_size >> 16, 0);
}

static void hide_cart(int hide)
{
#ifdef ACTION_REPLAY_HIDE_CARTRIDGE
    if(hide) {
        action_replay_unmap_banks();
    } else {
        action_replay_map_banks();
    }
#endif
}

/*extern void Interrupt (int nr);*/

/* Cartridge activates itself by overlaying its rom
 * over chip-ram and then issuing IRQ 7
 *
 * I just copy IRQ vector 7 from ROM to chip RAM
 * instead of fully emulating cartridge's behaviour.
 */

static void action_replay_go (void)
{
    hide_cart (0);
    memcpy (armemory_ram + 0xf000, ar_custom, 2 * 256);
    action_replay_flag = ACTION_REPLAY_ACTIVE;
    set_special (&regs, SPCFLAG_ACTION_REPLAY);
    copyfromamiga (artemp, regs.vbr + 0x7c, 4);
    copytoamiga (regs.vbr + 0x7c, armemory_rom + 0x7c, 4);
    Interrupt (7);
}

static void action_replay_go1 (int irq)
{
    hide_cart (0);
    action_replay_flag = ACTION_REPLAY_ACTIVE;

    memcpy (armemory_ram + 0xf000, ar_custom, 2 * 256);
    Interrupt (7);
}

static void hrtmon_go (int mode)
{
    memcpy (hrtmon_custom, ar_custom, 2 * 256);
    hrtmon_flag = ACTION_REPLAY_ACTIVE;
    set_special (&regs, SPCFLAG_ACTION_REPLAY);
    put_long (regs.vbr + 0x7c, hrtmem_start + 8 + (mode ? 4 : 0));
    Interrupt (7);
}

void hrtmon_enter (void)
{
    if (!hrtmemory) return;
    write_log("HRTMON: freeze\n");
    hrtmon_go(1);
}

void action_replay_enter(void)
{
    if (!armemory_rom) return;
    if (armodel == 1) {
	write_log("AR1: Enter PC:%p\n", m68k_getpc (&regs));
        action_replay_go1 (7);
        unset_special (&regs, SPCFLAG_ACTION_REPLAY);
        return;
    }
    if (action_replay_flag == ACTION_REPLAY_DORESET) {
        write_log("AR2/3: reset\n");
        armode = ARMODE_BREAKPOINT_AR3_RESET_AR2;
    }
    else if (armode == ARMODE_FREEZE) {
        write_log("AR2/3: activated (freeze)\n");
    }
    else if (armode >= 2)
    {
	if ( armode == ARMODE_BREAKPOINT_AR2 )
	{
	    write_log("AR2: activated (breakpoint)\n");
	}
	else if ( armode == ARMODE_BREAKPOINT_AR3_RESET_AR2 )
	{
	    write_log("AR3: activated (breakpoint)\n");
	}
	else
	{
	    write_log("AR2/3: mode(%d) > 3 this shouldn't happen.\n", armode);
	}
	armode = ARMODE_BREAKPOINT_ACTIVATED;
    }
    action_replay_go();
}

void check_prefs_changed_carts(int in_memory_reset)
{
    if (strcmp (currprefs.cartfile, changed_prefs.cartfile) != 0)
    {
	write_log("Cartridge ROM Prefs changed.\n");
        if (action_replay_unload(in_memory_reset))
    	{
	    memcpy (currprefs.cartfile, changed_prefs.cartfile, sizeof currprefs.cartfile);
	    #ifdef ACTION_REPLAY
    	    action_replay_load();
    	    action_replay_init(1);
    	    #endif
    	    #ifdef ACTION_REPLAY_HRTMON
    	    hrtmon_load(1);
    	    #endif
	}
    }
}

void action_replay_reset(void)
{
    if (action_replay_flag == ACTION_REPLAY_INACTIVE)
	return;
    write_log_debug("action_replay_reset()\n");

    if (savestate_state == STATE_RESTORE) {
        if (regs.pc >= arrom_start && regs.pc <= arrom_start + arrom_size) {
	    action_replay_flag = ACTION_REPLAY_ACTIVE;
	    hide_cart (0);
	} else {
	    action_replay_flag = ACTION_REPLAY_IDLE;
            hide_cart (1);
	}
	return;
    }

    if (armodel == 1 )
    {
	/* We need to mark it as active here, because the kickstart rom jumps directly into it. */
	action_replay_flag = ACTION_REPLAY_ACTIVE;
    }
    else
    {
	write_log_debug("Setting flag to ACTION_REPLAY_WAITRESET\n");
	write_log_debug("armode == %d\n", armode);
    	action_replay_flag = ACTION_REPLAY_WAITRESET;
    }
}

void action_replay_ciaread(void)
{
    if (armodel < 2)
        return;
    if (action_replay_flag != ACTION_REPLAY_IDLE) return;
    if (action_replay_flag == ACTION_REPLAY_INACTIVE) return;
    if (armode < 2) /* If there are no active breakpoints*/ return;
    if (m68k_getpc (&regs) >= 0x200) return;
    action_replay_flag = ACTION_REPLAY_ACTIVATE;
    set_special (&regs, SPCFLAG_ACTION_REPLAY);
}

int action_replay_freeze(void)
{
    if(action_replay_flag == ACTION_REPLAY_IDLE)
    {
	if (armodel == 1)
	{
	    action_replay_chipwrite();
	}
	else
	{
	    action_replay_flag = ACTION_REPLAY_ACTIVATE;
	    set_special (&regs, SPCFLAG_ACTION_REPLAY);
	    armode = ARMODE_FREEZE;
    	}
	return 1;
    } else if(hrtmon_flag == ACTION_REPLAY_IDLE) {
        hrtmon_flag = ACTION_REPLAY_ACTIVATE;
        set_special (&regs, SPCFLAG_ACTION_REPLAY);
	return 1;
    }
    return 0;
}

void action_replay_chipwrite(void)
{
    if (armodel > 1)
    {
        action_replay_flag = ACTION_REPLAY_DORESET;
        set_special (&regs, SPCFLAG_ACTION_REPLAY);
    }
    else
    {
	/* copy 0x60 addr info to level 7 */
	/* This is to emulate the 0x60 interrupt. */
    	copyfromamiga (artemp, regs.vbr + 0x60, 4);
    	copytoamiga (regs.vbr + 0x7c, artemp, 4);
	ar_wait_pop = 1; /* Wait for stack to pop. */

	action_replay_flag = ACTION_REPLAY_ACTIVATE;
	set_special (&regs, SPCFLAG_ACTION_REPLAY);
    }
}

void action_replay_hide(void)
{
    hide_cart(1);
    action_replay_flag = ACTION_REPLAY_IDLE;
}

void hrtmon_hide(void)
{
    hrtmon_flag = ACTION_REPLAY_IDLE;
/*  write_log_debug("HRTMON: exit\n"); */
}

void hrtmon_breakenter(void)
{
    hrtmon_flag = ACTION_REPLAY_HIDE;
    set_special (&regs, SPCFLAG_ACTION_REPLAY);
/*    write_log_debug("HRTMON: In hrtmon routine.\n"); */
}


/* Disabling copperlist processing:
 * On: ar317 an rts at 41084c does it.
 * On: ar214: an rts at 41068e does it.
 */


/* Original AR3 only works with KS 1.3
 * this patch fixes that problem.
 */

static uae_u8 ar3patch1[]={0x20,0xc9,0x51,0xc9,0xff,0xfc};
static uae_u8 ar3patch2[]={0x00,0xfc,0x01,0x44};

static void action_replay_patch(void)
{
    unsigned int off1,off2;
    uae_u8 *kickmem = kickmemory;

    if (armodel != 3 || !kickmem)
        return;
    if (!memcmp (kickmem, kickmem + 262144, 262144)) off1 = 262144; else off1 = 0;
    for (;;) {
        if (!memcmp (kickmem + off1, ar3patch1, sizeof (ar3patch1)) || off1 == 524288 - sizeof (ar3patch1)) break;
        off1++;
    }
    off2 = 0;
    for(;;) {
        if (!memcmp (armemory_rom + off2, ar3patch2, sizeof(ar3patch2)) || off2 == ar_rom_file_size - sizeof (ar3patch2)) break;
        off2++;
    }
    if (off1 == 524288 - sizeof (ar3patch1) || off2 == ar_rom_file_size - sizeof (ar3patch2))
	return;
    armemory_rom[off2 + 0] = (uae_u8)((off1 + kickmem_start + 2) >> 24);
    armemory_rom[off2 + 1] = (uae_u8)((off1 + kickmem_start + 2) >> 16);
    armemory_rom[off2 + 2] = (uae_u8)((off1 + kickmem_start + 2) >> 8);
    armemory_rom[off2 + 3] = (uae_u8)((off1 + kickmem_start + 2) >> 0);
    write_log ("AR ROM patched for KS2.0+\n");
}

/* Returns 0 if the checksum is OK.
 * Else, it returns the calculated checksum.
 * Note: Will be wrong if the checksum is zero, but i'll take my chances on that not happenning ;)
 */
static uae_u32 action_replay_calculate_checksum()
{
	uae_u32* checksum_end;
	uae_u32* checksum_start;
	uae_u8   checksum_start_offset[] = {0, 0, 4, 0x7c};
	uae_u32 checksum = 0;
	uae_u32 stored_checksum;

	/* All models: The checksum is calculated right upto the long checksum in the rom.
	 * AR1: The checksum starts at offset 0.
	 * AR1: The checksum is the last non-zero long in the rom.
	 * AR2: The checksum starts at offset 4.
	 * AR2: The checksum is the last Long in the rom.
	 * AR3: The checksum starts at offset 0x7c.
	 * AR3: The checksum is the last Long in the rom.
	 *
   * Checksums: (This is a good way to compare roms. I have two with different md5sums,
	 * but the same checksum, so the difference must be in the first four bytes.)
	 * 3.17 0xf009bfc9
	 * 3.09 0xd34d04a7
	 * 2.14 0xad839d36
	 * 2.14 0xad839d36
	 * 1.15 0xee12116
   */

	if (!armemory_rom)
	    return 0; /* If there is no rom then i guess the checksum is ok */

	checksum_start = (uae_u32*)&armemory_rom[checksum_start_offset[armodel]];
	checksum_end = (uae_u32*)&armemory_rom[ar_rom_file_size];

	/* Search for first non-zero Long starting from the end of the rom. */
	/* Assume long alignment, (will always be true for AR2 and AR3 and the AR1 rom i've got). */
	/* If anyone finds an AR1 rom with a word-aligned checksum, then this code will have to be modified. */
	while (! *(--checksum_end) );

	if ( armodel == 1)
	{
	  uae_u16* rom_ptr_word;
	  uae_s16  sign_extended_word;

		rom_ptr_word = (uae_u16*)checksum_start;
		while ( rom_ptr_word != (uae_u16*)checksum_end )
		{
				sign_extended_word = (uae_s16)do_get_mem_word (rom_ptr_word);
				/* When the word is cast on the following line, it will get sign-extended. */
			  checksum += (uae_u32)sign_extended_word;
				rom_ptr_word++;
		}
	}
	else
	{
	  uae_u32* rom_ptr_long;

		rom_ptr_long = checksum_start;
		while ( rom_ptr_long != checksum_end )
		{
			  checksum += do_get_mem_long (rom_ptr_long);
				rom_ptr_long++;
		}
	}

	stored_checksum = do_get_mem_long(checksum_end);

	return checksum == stored_checksum ? 0 : checksum;
}

/* Returns 0 on error. */
static uae_u8* get_checksum_location()
{
	uae_u32* checksum_end;

	/* See action_replay_calculate_checksum() for checksum info. */

	if (!armemory_rom)
		return 0;

	checksum_end = (uae_u32*)&armemory_rom[ar_rom_file_size];

	/* Search for first non-zero Long starting from the end of the rom. */
	while (! *(--checksum_end) );

	return (uae_u8*)checksum_end;
}


/* Replaces the existing cart checksum with a correct one. */
/* Useful if you want to patch the rom. */
static void action_replay_fixup_checksum(uae_u32 new_checksum)
{
	uae_u32* checksum = (uae_u32*)get_checksum_location();
	if ( checksum )
	{
		do_put_mem_long(checksum, new_checksum);
	}
	else
	{
		write_log("Unable to locate Checksum in ROM.\n");
	}
	return;
}

/* Longword search on word boundary
 * the search_value is assumed to already be in the local endian format
 * return 0 on failure
 */
static uae_u8* find_absolute_long(uae_u8* start_addr, uae_u8* end_addr, uae_u32 search_value)
{
	uae_u8* addr;

	for ( addr = start_addr; addr < end_addr; )
	{
		if ( do_get_mem_long((uae_u32*)addr) == search_value )
		{
/*			write_log_debug("Found %p at offset %p.\n", search_value, addr - start_addr);*/
			return addr;
		}
		addr+=2;
	}
	return 0;
}

/* word search on word boundary
 * the search_addr is assumed to already be in the local endian format
 * return 0 on failure
 * Currently only tested where the address we are looking for is AFTER the instruction.
 * Not sure it works with negative offsets.
 */
static uae_u8* find_relative_word(uae_u8* start_addr, uae_u8* end_addr, uae_u16 search_addr)
{
	uae_u8* addr;

	for ( addr = start_addr; addr < end_addr; )
	{
		if ( do_get_mem_word((uae_u16*)addr) == (uae_u16)(search_addr - (uae_u16)(addr-start_addr)) )
		{
/*			write_log_debug("Found %p at offset %p.\n", search_addr, addr - start_addr);*/
			return addr;
		}
		addr+=2;
	}
	return 0;
}

/* Disable rom test */
/* This routine replaces the rom-test routine with a 'rts'.
 * It does this in a 'safe' way, by searching for a reference to the checksum
 * and only disables it if the surounding bytes are what it expects.
 */

static void disable_rom_test()
{
    uae_u8* addr;

    uae_u8* start_addr = armemory_rom;
    uae_u8* end_addr = get_checksum_location();

/*
 * To see what the routine below is doing. Here is some code from the Action replay rom where it does the
 * checksum test.
 * AR1:
 * F0D4D0 6100 ???? bsr.w   calc_checksum ; calculate the checksum
 * F0D4D4 41FA 147A lea     (0xf0e950,PC),a0   ; load the existing checksum.
 * ; do a comparison.
 * AR2:
 * 40EC92 6100 ???? bsr.w   calc_checksum
 * 40EC96 41F9 0041 FFFC lea (0x41fffc),a0
 */

	if ( armodel == 1)
	{
	  uae_u16 search_value_rel = end_addr - start_addr;
		addr = find_relative_word(start_addr, end_addr, search_value_rel);

		if ( addr )
		{
			if ( do_get_mem_word((uae_u16*)(addr-6)) == 0x6100 && /* bsr.w */
					 do_get_mem_word((uae_u16*)(addr-2)) == 0x41fa )  /* lea relative */
	 		{
				write_log("Patching to disable ROM TEST.\n");
	 			do_put_mem_word((uae_u16*)(addr-6), 0x4e75); /* rts */
			}
		}
	}
	else
	{
	  uae_u32 search_value_abs = arrom_start + end_addr - start_addr;
		addr = find_absolute_long(start_addr, end_addr, search_value_abs);

		if ( addr )
		{
			if ( do_get_mem_word((uae_u16*)(addr-6)) == 0x6100 && /* bsr.w */
					 do_get_mem_word((uae_u16*)(addr-2)) == 0x41f9 )  /* lea absolute */
	 		{
				write_log("Patching to disable ROM TEST.\n");
	 			do_put_mem_word((uae_u16*)(addr-6), 0x4e75); /* rts */
			}
		}
	}
}

/* After we have calculated the checksum, and verified the rom is ok,
 * we can do two things.
 * 1. (optionally)Patch it and then update the checksum.
 * 2. Remove the checksum check and (optionally) patch it.
 * I have chosen to use no.2 here, because it should speed up the Action Replay slightly (and it was fun).
 */
static void action_replay_checksum_info(void)
{
    if (!armemory_rom)
	return;
    if ( action_replay_calculate_checksum() == 0 )
    {
 	write_log("Action Replay Checksum is OK.\n");
    }
    else
    {
	write_log("Action Replay Checksum is INVALID.\n");
    }
    disable_rom_test();
}



static void action_replay_setbanks (void)
{
    if (!savestate_state && chipmem_bank.lput == chipmem_lput) {
        switch (armodel)
        {
            case 2:
            case 3:
            if (currprefs.cpu_cycle_exact)
                chipmem_bank.wput = chipmem_wput_actionreplay23;
            chipmem_bank.lput = chipmem_lput_actionreplay23;
            break;
            case 1:
            chipmem_bank.bput = chipmem_bput_actionreplay1;
            chipmem_bank.wput = chipmem_wput_actionreplay1;
            chipmem_bank.lput = chipmem_lput_actionreplay1;
            break;
        }
   }
}

static void action_replay_unsetbanks (void)
{
    chipmem_bank.bput = chipmem_bput;
    chipmem_bank.wput = chipmem_wput;
    chipmem_bank.lput = chipmem_lput;
}

/* param to allow us to unload the cart. Currently we know it is safe if we are doing a reset to unload it.*/
int action_replay_unload(int in_memory_reset)
{
	static const char *state[] =
	{
		"ACTION_REPLAY_WAIT_PC",
		"ACTION_REPLAY_INACTIVE",
		"ACTION_REPLAY_WAITRESET",
		"0",
		"ACTION_REPLAY_IDLE",
		"ACTION_REPLAY_ACTIVATE",
		"ACTION_REPLAY_ACTIVE",
		"ACTION_REPLAY_DORESET",
		"ACTION_REPLAY_HIDE",
	};

	write_log_debug("Action Replay State:(%s) Hrtmon State:(%s)\n", state[action_replay_flag+3],state[hrtmon_flag+3] );

	if ( armemory_rom && armodel == 1 )
	{
		if ( is_ar_pc_in_ram() || is_ar_pc_in_rom() || action_replay_flag == ACTION_REPLAY_WAIT_PC )
		{
			write_log("Can't Unload Action Replay 1. It is Active.\n");
			return 0;
		}
	}
	else
	{
		if ( action_replay_flag != ACTION_REPLAY_IDLE && action_replay_flag != ACTION_REPLAY_INACTIVE )
		{
			write_log("Can't Unload Action Replay. It is Active.\n");
			return 0; /* Don't unload it whilst it's active, or it will crash the amiga if not the emulator */
		}
		if ( hrtmon_flag != ACTION_REPLAY_IDLE && hrtmon_flag != ACTION_REPLAY_INACTIVE )
		{
			write_log("Can't Unload Hrtmon. It is Active.\n");
			return 0; /* Don't unload it whilst it's active, or it will crash the amiga if not the emulator */
		}
	}

	unset_special (&regs, SPCFLAG_ACTION_REPLAY); /* This shouldn't be necessary here, but just in case. */
	action_replay_flag = ACTION_REPLAY_INACTIVE;
	hrtmon_flag = ACTION_REPLAY_INACTIVE;
	action_replay_unsetbanks();
	action_replay_unmap_banks();
	hrtmon_unmap_banks();
	/* Make sure you unmap everything before you call action_replay_cleanup() */
	action_replay_cleanup();
	return 1;
}


int action_replay_load(void)
{
    struct zfile *f;

    armodel = 0;
    action_replay_flag = ACTION_REPLAY_INACTIVE;
    write_log_debug("Entered action_replay_load()\n");
    /* Don't load a rom if one is already loaded. Use action_replay_unload() first. */
    if (armemory_rom || hrtmemory)
    {
	write_log("action_replay_load() ROM already loaded.\n");
        return 0;
    }

    if (strlen(currprefs.cartfile) == 0)
        return 0;
    f = zfile_fopen(currprefs.cartfile,"rb");
    if (!f) {
        write_log("failed to load '%s' Action Replay ROM\n", currprefs.cartfile);
        return 0;
    }
    zfile_fseek(f, 0, SEEK_END);
    ar_rom_file_size = zfile_ftell(f);
    zfile_fseek(f, 0, SEEK_SET);
    if (ar_rom_file_size != 65536 && ar_rom_file_size != 131072 && ar_rom_file_size != 262144) {
        write_log("rom size must be 64KB (AR1), 128KB (AR2) or 256KB (AR3)\n");
        zfile_fclose(f);
        return 0;
    }
    action_replay_flag = ACTION_REPLAY_INACTIVE;
    armemory_rom = xmalloc (ar_rom_file_size);
    zfile_fread (armemory_rom, ar_rom_file_size, 1, f);
    zfile_fclose (f);
    if (ar_rom_file_size == 65536) {
        armodel = 1;
        arrom_start = 0xf00000;
        arrom_size = 0x10000;
         /* real AR1 RAM location is 0x9fc000-0x9fffff */
        arram_start = 0x9f0000;
        arram_size = 0x10000;
    } else {
        armodel = ar_rom_file_size / 131072 + 1;
        arrom_start = 0x400000;
        arrom_size = armodel == 2 ? 0x20000 : 0x40000;
        arram_start = 0x440000;
        arram_size = 0x10000;
    }
    arram_mask = arram_size - 1;
    arrom_mask = arrom_size - 1;
    armemory_ram = xcalloc (arram_size, 1);
    write_log("Action Replay %d installed at %08.8X, size %08.8X\n", armodel, arrom_start, arrom_size);
    action_replay_setbanks ();
    action_replay_version();
    return armodel;
}

void action_replay_init (int activate)
{
    if (!armemory_rom)
        return;
    hide_cart (0);
    if (armodel > 1)
        hide_cart (1);
    if (activate) {
        if (armodel > 1)
            action_replay_flag = ACTION_REPLAY_WAITRESET;
    }
}

/* This only deallocates memory, it is not suitable for unloading roms and continuing */
void action_replay_cleanup()
{
    if (armemory_rom)
	free (armemory_rom);
    if (armemory_ram)
	free (armemory_ram);
    if (hrtmemory)
    	free (hrtmemory);

    armemory_rom = 0;
    armemory_ram = 0;
    hrtmemory = 0;
}

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef struct {
        char jmps[20];
        unsigned int mon_size;
        unsigned short col0, col1;
        char right;
        char keyboard;
        char key;
        char ide;
        char a1200;
        char aga;
        char insert;
        char delay;
        char lview;
        char cd32;
        char screenmode;
        char vbr;
        char entered;
        char hexmode;
        unsigned short error_sr;
        unsigned int error_pc;
        unsigned short error_status;
        char newid[6];
        unsigned short        mon_version;
        unsigned short        mon_revision;
        unsigned int        whd_base;
        unsigned short        whd_version;
        unsigned short        whd_revision;
        unsigned int        max_chip;
        unsigned int        custom;
} HRTCFG;

static void hrtmon_configure(HRTCFG *cfg)
{
do_put_mem_word(&cfg->col0,0x000);
do_put_mem_word(&cfg->col1,0xeee);
cfg->right = 0;
cfg->key = FALSE;
cfg->ide = 0;
cfg->a1200 = 0;
cfg->aga = (currprefs.chipset_mask & 4) ? 1 : 0;
cfg->insert = TRUE;
cfg->delay = 15;
cfg->lview = FALSE;
cfg->cd32 = 0;
cfg->screenmode = 0;
cfg->vbr = TRUE;
cfg->hexmode = FALSE;
cfg->mon_size=0;
cfg->hexmode = TRUE;
do_put_mem_long(&cfg->max_chip,currprefs.chipmem_size);
hrtmon_custom = do_get_mem_long ((uae_u32*)&cfg->custom)+hrtmemory;
}

static void hrtmon_reloc (uae_u32 *mem, uae_u32 *header)
{
    uae_u32 *p;
    uae_u8 *base;
    uae_u32 len, i;

    len = do_get_mem_long (header + 7); /* Get length of file from exe header.*/
    p = mem + len + 3;
    len = do_get_mem_long (p - 2);
    for (i = 0; i < len; i++) {
	base = (uae_u8 *)((unsigned long)do_get_mem_long (p) + (unsigned long)mem);
	do_put_mem_long ((uae_u32*)base, do_get_mem_long ((uae_u32 *)base) + hrtmem_start);
	p++;
    }
}

static uae_u8 hrt_header[] = {
	0x0, 0x0, 0x3, 0xf3,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x1,
	0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0,
	0x0
};

int hrtmon_load(int activate)
{
    struct zfile *f;
    int size;
    uae_u32 header[8];
    uae_u32 id_string[2];

    /* Don't load a rom if one is already loaded. Use action_replay_unload() first. */
    if (armemory_rom)
        return 0;
    if (hrtmemory)
      return 0;

    armodel = 0;
    if (strlen(currprefs.cartfile) == 0)
        return 0;
    f=zfile_fopen(currprefs.cartfile,"rb");
    if(!f) {
        write_log("failed to load '%s' HRTMon ROM\n", currprefs.cartfile);
        return 0;
    }
    zfile_fseek(f,0,SEEK_END);
    size = zfile_ftell(f) - 8*4;
    zfile_fseek(f,0,SEEK_SET);
    if ( size < (int)(sizeof(header)+sizeof(id_string)) )
    {
	write_log("Not a Hrtmon Rom.\n");
    	zfile_fclose (f);
	return 0;
    }
    zfile_fread(header,sizeof(header),1,f);

    /* Check the header */
/*  uae_u8* ptr = (uae_u8*)&header; */
/*  for ( ; ptr < sizeof(header); ptr++) */
/*  { */
/*	if ( *ptr != *header */

    zfile_fread(id_string,sizeof(id_string),1,f);
    if (strncmp((char*)&id_string[1], "HRT!",4) != 0 )
    {
	write_log("Not a Hrtmon Rom\n");
    	zfile_fclose (f);
	return 0;
    }
    zfile_fseek(f,sizeof(header),SEEK_SET);

    hrtmem_size = size;
    hrtmem_size += 65535;
    hrtmem_size &= ~65535;
    hrtmem_mask = hrtmem_size - 1;
    hrtmem_start = HRTMON_BASE;
    hrtmemory = xmalloc (hrtmem_size);
    zfile_fread (hrtmemory,size,1,f);
    zfile_fclose (f);
    hrtmon_configure((HRTCFG*)hrtmemory);
    hrtmon_reloc((uae_u32*)hrtmemory,/*(uae_u32*)*/header);
    hrtmem_bank.baseaddr = hrtmemory;
    hrtmon_flag = ACTION_REPLAY_IDLE;
    if(!activate) hrtmon_flag = ACTION_REPLAY_INACTIVE;
    write_log("HRTMon installed at %08.8X, size %08.8X\n",hrtmem_start, hrtmem_size);
    return 1;
}

void hrtmon_map_banks()
{
    if(!hrtmemory)
        return;

    map_banks (&hrtmem_bank, hrtmem_start >> 16, hrtmem_size >> 16, hrtmem_size);
}

static void hrtmon_unmap_banks()
{
    if(!hrtmemory)
        return;

    map_banks (&dummy_bank, hrtmem_start >> 16, hrtmem_size >> 16, hrtmem_size);
}



#define AR_VER_STR_OFFSET 0x4 /* offset in the rom where the version string begins. */
#define AR_VER_STR_END 0x7c   /* offset in the rom where the version string ends. */
#define AR_VER_STR_LEN (AR_VER_STR_END - AR_VER_STR_OFFSET)
char arVersionString[AR_VER_STR_LEN+1];

/* This function extracts the version info for AR2 and AR3. */

void action_replay_version()
{
    char* tmp;
    int iArVersionMajor = -1 ;
    int iArVersionMinor = -1;
    char* pNext;
    char sArDate[11];
    *sArDate = '\0';

    if (!armemory_rom)
        return;

    if ( armodel == 1 )
	    return; /* no support yet. */

    /* Extract Version string */
    memcpy(arVersionString, armemory_rom+AR_VER_STR_OFFSET, AR_VER_STR_LEN);
    arVersionString[AR_VER_STR_LEN]= '\0';
    tmp = strchr(arVersionString, 0x0d);
    if ( tmp )
    {
        *tmp = '\0';
    }
/*    write_log_debug("Version string is : '%s'\n", arVersionString); */

    tmp = strchr(arVersionString,')');
    if ( tmp )
    {
        *tmp = '\0';
        tmp = strchr(arVersionString, '(');
        if ( tmp )
        {
            if ( *(tmp + 1 ) == 'V' )
            {
                pNext = tmp + 2;
                tmp = strchr(pNext, '.');
                if ( tmp )
                {
                    *tmp = '\0';
                    iArVersionMajor = atoi(pNext);
                    pNext = tmp+1;
                    tmp = strchr(pNext, ' ');
                    if ( tmp )
                    {
                        *tmp = '\0';
                        iArVersionMinor = atoi(pNext);
                    }
                    pNext = tmp+1;
                    strcpy(sArDate, pNext);
                }

            }
        }
    }

    if ( iArVersionMajor > 0 )
    {
        write_log("Version of cart is '%d.%.02d', date is '%s'\n", iArVersionMajor, iArVersionMinor, sArDate);
    }
}

/* This function doesn't reset the Cart memory, it is just called during a memory reset */
void action_replay_memory_reset(void)
{
    #ifdef ACTION_REPLAY
    if ( armemory_rom )
    {
	action_replay_hide();
    }
    #endif
    #ifdef ACTION_REPLAY_HRTMON
    if ( hrtmemory )
    {
    	hrtmon_hide(); /* It is never really idle */
    }
    #endif
    #ifdef ACTION_REPLAY_COMMON
    check_prefs_changed_carts(1);
    #endif
    action_replay_patch();
    action_replay_checksum_info();
}


#ifdef SAVESTATE

uae_u8 *save_action_replay (uae_u32 *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak, *dst;

    *len = 1;
    if (!armemory_ram || !armemory_rom || !armodel)
	return 0;
    *len = 1 + strlen (currprefs.cartfile) + 1 + arram_size + 256;
    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = malloc (*len);
    save_u8 (armodel);
    strcpy ((char *) dst, currprefs.cartfile);
    dst += strlen ((const char *) dst) + 1;
    memcpy (dst, armemory_ram, arram_size);
    return dstbak;
}

const uae_u8 *restore_action_replay (const uae_u8 *src)
{
    action_replay_unload (1);
    armodel = restore_u8 ();
    if (!armodel)
	return src;
    strncpy (changed_prefs.cartfile, (char *) src, 255);
    strcpy (currprefs.cartfile, changed_prefs.cartfile);
    src += strlen ((const char *) src) + 1;
    action_replay_load ();
    if (armemory_ram) {
	memcpy (armemory_ram, src, arram_size);
	memcpy (ar_custom, armemory_ram + 0xf000, 2 * 256);
	src += arram_size;
    }
    src += 256;
    return src;
}

#endif /* SAVESTATE */

#endif /* ACTION_REPLAY */

/*
 * UAE - The Un*x Amiga Emulator
 *
 * Enforcer Like Support
 *
 * Copyright 2000-2003 Bernd Roesch and Sebastian Bauer
 * Copyright 2004      Richard Drummond
 */

#ifdef ENFORCER

#include <stdlib.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "memory.h"
#include "custom.h"
#include "options.h"
#include "newcpu.h"
#include "enforcer.h"

#ifndef _WIN32
#define console_out printf
#else
#define console_out(...) do {;} while (0)
#endif

/* Configurable options */
#define ENFORCESIZE 1024
#define STACKLINES 5
#define INSTRUCTIONLINES 17

#define ISILLEGAL(addr) (addr < 4 || (addr > 4 && addr < ENFORCESIZE))

extern uae_u32 natmem_offset;

int flashscreen = 0;
static int enforcer_installed = 0;
static int enforcer_hit = 0; /* set to 1 if displaying the hit */

#define ENFORCER_BUF_SIZE 4096
static char enforcer_buf[ENFORCER_BUF_SIZE];

#define GET_PC m68k_getpc (&regs)

static addrbank saved_dummy_bank;
static addrbank saved_chipmem_bank;


/*************************************************************
 Returns the first node entry of an exec list or 0 if
 empty
*************************************************************/
static uae_u32 amiga_list_first (uae_u32 list)
{
    uae_u32 node = get_long (list);      /* lh_Head */
    if (!node)
	return 0;
    if (!get_long (node))
	return 0;   /* ln_Succ */
    return node;
}

/*************************************************************
 Returns the next node of an exec node or 0 if it was the
 last element
*************************************************************/
static uae_u32 amiga_node_next (uae_u32 node)
{
    uae_u32 next = get_long (node);    /* ln_Succ */
    if (!next)
	return 0;
    if (!get_long (next))
	return 0; /* ln_Succ */
    return next;
}

/*************************************************************
 Converts an amiga address to a native one or NULL if this
 is not possible, Size specified the number of bytes you
 want to access
*************************************************************/
static uae_u8 *amiga2native (uae_u32 aptr, int size)
{
    addrbank bank = get_mem_bank (aptr);

    /* Check if the address can be translated to native */
    if (bank.check (aptr,size)) {
	return bank.xlateaddr (aptr);
    }
    return NULL;
}

/*************************************************************
 Writes the Hunk and Offset of the given Address into buf
*************************************************************/
static int enforcer_decode_hunk_and_offset (char *buf, uae_u32 pc)
{
    uae_u32 sysbase = get_long(4);
    uae_u32 semaphore_list = sysbase + 532;

    /* First step is searching for the SegTracker semaphore */
    uae_u32 node = amiga_list_first (semaphore_list);
    while (node) {
	uae_u32 string = get_long (node + 10); /* ln_Name */
	uae_u8 *native_string = amiga2native (string, 100);

	if (native_string) {
	    if (!strcmp ((char *) native_string, "SegTracker"))
		break;
	}
	node = amiga_node_next (node);
    }

    if (node) {
	/* We have found the segtracker semaphore. Soon after the
	 * public documented semaphore structure Segtracker holds
	 * an own list of all segements. We will use this list to
	 * find out the hunk and offset (simliar to segtracker).
	 *
	 * Source of segtracker can be found at:
	 *    http://www.sinz.org/Michael.Sinz/Enforcer/SegTracker.c.html
	 */
	uae_u32 seg_list = node + 46 + 4; /* sizeof(struct SignalSemaphore) + seg find */

	node = amiga_list_first (seg_list);
	while (node) {
	    uae_u32 seg_entry = node + 12;
	    uae_u32 address, size;
	    int hunk = 0;

	    /* Go through all entries until an address is 0
	     * or the segment has been found */
	    while ((address = get_long (seg_entry))) {
		size = get_long (seg_entry + 4);

		if (pc >= address && pc < address + size) {
		    uae_u32 name, offset;
		    const char *native_name;

		    offset = pc - address - 4;
		    name = get_long (node + 8); /* ln_Name */
		    if (name) {
			native_name = (char *) amiga2native (name, 100);
			if (!native_name)
			    native_name = "Unknown";
		    } else
			native_name = "Unknown";

		    sprintf (buf, "----> %08lx - \"%s\" Hunk %04lx Offset %08lx\n", pc,
			native_name, hunk, offset);
		    return 1;
	        }
		seg_entry += 8;
		hunk++;
	    }
	    node = amiga_node_next (node);
	}
    }
    return 0;
}

/*************************************************************
 Display the enforcer hit
*************************************************************/
static void enforcer_display_hit (const char *addressmode, uae_u32 pc, uaecptr addr)
{
    uae_u32 a7;
    uae_u32 sysbase;
    uae_u32 this_task;
    uae_u32 task_name;
    const char *native_task_name;
    int i, j;
    static char buf[256], instrcode[256];
    static char lines[INSTRUCTIONLINES/2][256];
    static uaecptr bestpc_array[INSTRUCTIONLINES/2][5];
    static int bestpc_idxs[INSTRUCTIONLINES/2];
    char *enforcer_buf_ptr = enforcer_buf;
    uaecptr bestpc, pospc, nextpc, temppc;

    if (enforcer_hit) return; /* our function itself generated a hit ;), avoid endless loop */
	enforcer_hit = 1;

    if (!(sysbase = get_long (4)))
	return;
    if (!(this_task = get_long (sysbase + 276)))
	return;

    task_name = get_long (this_task + 10); /* ln_Name */
    native_task_name = (char *) amiga2native (task_name, 100);

    strcpy (enforcer_buf_ptr, "Enforcer Hit! Bad program\n");
    enforcer_buf_ptr += strlen (enforcer_buf_ptr);

    sprintf (buf, "Illegal %s: %08lx", addressmode, addr);
    sprintf (enforcer_buf_ptr,"%-48sPC: %0lx\n", buf, pc);
    enforcer_buf_ptr += strlen (enforcer_buf_ptr);

    /* Data registers */
    sprintf (enforcer_buf_ptr, "Data: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	m68k_dreg (&regs, 0), m68k_dreg (&regs, 1), m68k_dreg (&regs, 2), m68k_dreg (&regs, 3),
	m68k_dreg (&regs, 4), m68k_dreg (&regs, 5), m68k_dreg (&regs, 6), m68k_dreg (&regs, 7));
    enforcer_buf_ptr += strlen (enforcer_buf_ptr);

    /* Address registers */
    sprintf (enforcer_buf_ptr, "Addr: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	m68k_areg (&regs, 0), m68k_areg (&regs, 1), m68k_areg (&regs, 2), m68k_areg (&regs, 3),
	m68k_areg (&regs, 4), m68k_areg (&regs, 5), m68k_areg (&regs, 6), m68k_areg (&regs, 7));
    enforcer_buf_ptr += strlen(enforcer_buf_ptr);

    /* Stack */
    a7 = m68k_areg (&regs, 7);
    for (i = 0; i < 8 * STACKLINES; i++) {
	a7 -= 4;
	if (!(i % 8)) {
	    strcpy (enforcer_buf_ptr,"Stck:");
	    enforcer_buf_ptr += strlen (enforcer_buf_ptr);
	}
	sprintf (enforcer_buf_ptr," %08lx", get_long (a7));
	enforcer_buf_ptr += strlen (enforcer_buf_ptr);

	if (i%8 == 7)
	    *enforcer_buf_ptr++ = '\n';
    }

    /* Segtracker output */
    a7 = m68k_areg (&regs, 7);
    if (get_long (a7-4) != pc) {
	if (enforcer_decode_hunk_and_offset (buf, pc)) {
	    strcpy (enforcer_buf_ptr, buf);
	    enforcer_buf_ptr += strlen (enforcer_buf_ptr);
	}
    }

    for (i = 0; i < 8 * STACKLINES; i++) {
	a7 -= 4;
	if (enforcer_decode_hunk_and_offset (buf, get_long (a7))) {
	    int l = strlen (buf);

	    if (ENFORCER_BUF_SIZE - (enforcer_buf_ptr - enforcer_buf) > l + 256) {
		strcpy(enforcer_buf_ptr, buf);
		enforcer_buf_ptr += l;
	    }
	}
    }

    /* Decode the instructions around the pc where the enforcer hit was caused.
     *
     * At first, the area before the pc, this not always done correctly because
     * it's done backwards */
    temppc = pc;

    memset (bestpc_array, 0, sizeof (bestpc_array));
    for (i = 0; i < INSTRUCTIONLINES / 2; i++)
	bestpc_idxs[i] = -1;

    for (i = 0; i < INSTRUCTIONLINES / 2; i++) {
	pospc = temppc;
	bestpc = 0;

	if (bestpc_idxs[i] == -1) {
	    for (j = 0; j < 5; j++) {
		pospc -= 2;
		sm68k_disasm (buf, NULL, pospc, &nextpc);
		if (nextpc == temppc) {
		    bestpc_idxs[i] = j;
		    bestpc_array[i][j] = bestpc = pospc;
		}
	    }
	} else {
	    bestpc = bestpc_array[i][bestpc_idxs[i]];
	}

	if (!bestpc) {
	    /* there was no best pc found, so it is high probable that
	     * a former used best pc was wrong.
	     *
	     * We trace back and use the former best pc instead
	     */

	    int former_idx;
	    int leave = 0;

	    do {
		if (!i) {
		    leave = 1;
		    break;
		}
		i--;
		former_idx = bestpc_idxs[i];
		bestpc_idxs[i] = -1;
		bestpc_array[i][former_idx] = 0;

		for (j = former_idx - 1; j >= 0; j--) {
		    if (bestpc_array[i][j]) {
			bestpc_idxs[i] = j;
			break;
		    }
		}
	    } while (bestpc_idxs[i] == -1);
	    if (leave)
		break;
	    if (i)
		temppc = bestpc_array[i-1][bestpc_idxs[i-1]];
	    else temppc = pc;
		i--; /* will be increased in after continue */
	    continue;
	}

	sm68k_disasm (buf, instrcode, bestpc, NULL);
	sprintf (lines[i], "%08lx :   %-20s %s\n", bestpc, instrcode, buf);
	temppc = bestpc;
    }

    i--;
    for (; i>=0; i--) {
	strcpy (enforcer_buf_ptr,lines[i]);
	enforcer_buf_ptr += strlen (enforcer_buf_ptr);
    }

    /* Now the instruction after the pc including the pc */
    temppc = pc;
    for (i=0; i < (INSTRUCTIONLINES+1) / 2; i++) {
	sm68k_disasm (buf, instrcode, temppc, &nextpc);
	sprintf (enforcer_buf_ptr, "%08lx : %s %-20s %s\n", temppc,
		(i == 0 ? "*" : " "), instrcode, buf);
	enforcer_buf_ptr += strlen (enforcer_buf_ptr);
	temppc = nextpc;
    }

    if (!native_task_name)
	native_task_name = "Unknown";
    sprintf (enforcer_buf_ptr, "Name: \"%s\"\n\n", native_task_name);
    enforcer_buf_ptr += strlen (enforcer_buf_ptr);

    console_out (enforcer_buf);
    write_log (enforcer_buf);

    enforcer_hit = 0;
    flashscreen = 30;
}

/*
 * Replacement chipmem accessors
 */

static uae_u32  chipmem_lget2  (uaecptr addr) REGPARAM;
static uae_u32  chipmem_wget2  (uaecptr addr) REGPARAM;
static uae_u32  chipmem_bget2  (uaecptr addr) REGPARAM;
static void     chipmem_lput2  (uaecptr addr, uae_u32 l) REGPARAM;
static void     chipmem_wput2  (uaecptr addr, uae_u32 w) REGPARAM;
static void     chipmem_bput2  (uaecptr addr, uae_u32 b) REGPARAM;
static int      chipmem_check2 (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 * chipmem_xlate2 (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 chipmem_lget2 (uaecptr addr)
{
    uae_u32 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);

    if (ISILLEGAL (addr))
	enforcer_display_hit ("LONG READ from", GET_PC, addr);

    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 chipmem_wget2 (uaecptr addr)
{
    uae_u16 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u16 *)(chipmemory + addr);

    if (ISILLEGAL (addr))
	enforcer_display_hit ("WORD READ from", GET_PC, addr);

    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 chipmem_bget2 (uaecptr addr)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;

    if (ISILLEGAL (addr))
	enforcer_display_hit ("BYTE READ from", GET_PC, addr);

    return chipmemory[addr];
}

static void REGPARAM2 chipmem_lput2 (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);

    if (ISILLEGAL (addr))
	enforcer_display_hit ("LONG WRITE to", GET_PC, addr);

    do_put_mem_long (m, l);
}

static void REGPARAM2 chipmem_wput2 (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u16 *)(chipmemory + addr);

    if (ISILLEGAL (addr))
	enforcer_display_hit ("WORD WRITE to", GET_PC, addr);

    do_put_mem_word (m, w);
}

static void REGPARAM2 chipmem_bput2 (uaecptr addr, uae_u32 b)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;

    if (ISILLEGAL (addr))
	enforcer_display_hit ("BYTE WRITE to", GET_PC, addr);
    chipmemory[addr] = b;
}

static int REGPARAM2 chipmem_check2 (uaecptr addr, uae_u32 size)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    return (addr + size) <= allocated_chipmem;
}

static uae_u8 * REGPARAM2 chipmem_xlate2 (uaecptr addr)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    return chipmemory + addr;
}

static const addrbank enforcer_chipmem_bank = {
    chipmem_lget2, chipmem_wget2, chipmem_bget2,
    chipmem_lput2, chipmem_wput2, chipmem_bput2,
    chipmem_xlate2, chipmem_check2, NULL
};

/*
 * Replacement dummy memory accessors
 */
static uae_u32  dummy_lget2    (uaecptr addr) REGPARAM;
static uae_u32  dummy_wget2    (uaecptr addr) REGPARAM;
static uae_u32  dummy_bget2    (uaecptr addr) REGPARAM;
static void     dummy_lput2    (uaecptr addr, uae_u32 l) REGPARAM;
static void     dummy_wput2    (uaecptr addr, uae_u32 w) REGPARAM;
static void     dummy_bput2    (uaecptr addr, uae_u32 b) REGPARAM;
static int      dummy_check2   (uaecptr addr, uae_u32 size) REGPARAM;

static uae_u32 REGPARAM2 dummy_lget2 (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    enforcer_display_hit ("LONG READ from", GET_PC, addr);
    return 0xbadedeef;
}

#ifdef JIT
static int warned_JIT_0xF10000 = 0;
#endif

static uae_u32 REGPARAM2 dummy_wget2 (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;

    if (addr >= 0x00F10000 && addr <= 0x00F7FFFF) {
	if (!warned_JIT_0xF10000) {
	    warned_JIT_0xF10000 = 1;
	    enforcer_display_hit ("LONG READ from", GET_PC, addr);
	}
	return 0;
    }
#endif
    enforcer_display_hit ("WORD READ from", GET_PC, addr);
    return 0xbadf;
}

static uae_u32 REGPARAM2 dummy_bget2 (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    enforcer_display_hit ("BYTE READ from", GET_PC, addr);
    return 0xbadedeef;
}

static void REGPARAM2 dummy_lput2 (uaecptr addr, uae_u32 l)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    enforcer_display_hit ("LONG WRITE to", GET_PC, addr);
}

static void REGPARAM2 dummy_wput2 (uaecptr addr, uae_u32 w)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    enforcer_display_hit ("WORD WRITE to", GET_PC, addr);
}

static void REGPARAM2 dummy_bput2 (uaecptr addr, uae_u32 b)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    enforcer_display_hit ("BYTE WRITE to", GET_PC, addr);
}

static int REGPARAM2 dummy_check2 (uaecptr addr, uae_u32 size)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    enforcer_display_hit ("CHECK from ", GET_PC, addr);
    return 0;
}

const addrbank enforcer_dummy_bank = {
    dummy_lget2, dummy_wget2, dummy_bget2,
    dummy_lput2, dummy_wput2, dummy_bput2,
    default_xlate, dummy_check2, NULL
};

/*************************************************************
 enable the enforcer like support, maybe later this make MMU
 exceptions so enforcer can use it. Returns 1 if enforcer
 is enabled
*************************************************************/
int enforcer_enable (void)
{
    extern addrbank chipmem_bank, dummy_bank;

    if (!enforcer_installed) {
	saved_dummy_bank   = dummy_bank;
	saved_chipmem_bank = chipmem_bank;

	dummy_bank   = enforcer_dummy_bank;
	chipmem_bank = enforcer_chipmem_bank;

	enforcer_installed = 1;
    }
    write_log ("Enforcer enabled\n");
    return 1;
}

/*************************************************************
 Disable Enforcer like support
*************************************************************/
int enforcer_disable (void)
{
    if (enforcer_installed) {
	dummy_bank   = saved_dummy_bank;
	chipmem_bank = saved_chipmem_bank;

	enforcer_installed = 0;
    }
    return 1;
}

#endif

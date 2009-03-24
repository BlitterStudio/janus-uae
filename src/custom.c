 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Custom chip emulation
  *
  * Copyright 1995-2002 Bernd Schmidt
  * Copyright 1995 Alessandro Bissacco
  * Copyright 2000-2004 Toni Wilen
  */

//#define CUSTOM_DEBUG
#define SPRITE_DEBUG 0
#define SPRITE_DEBUG_MINY 0x6a
#define SPRITE_DEBUG_MAXY 0x70
#define SPR0_HPOS 0x15
#define MAX_SPRITES 8
#define SPRITE_COLLISIONS
#define SPEEDUP

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "uae.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "custom_private.h"
#include "newcpu.h"
#include "cia.h"
#include "disk.h"
#include "blitter.h"
#include "xwin.h"
#include "inputdevice.h"
#include "audio.h"
#include "keybuf.h"
#include "serial.h"
#include "osemu.h"
#include "autoconf.h"
#include "traps.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "savestate.h"
#include "ar.h"
#ifdef AVIOUTPUT
#include "avioutput.h"
#endif
#include "debug.h"
#include "akiko.h"
#if defined(ENFORCER)
#include "enforcer.h"
#endif
#include "hrtimer.h"
#include "sleep.h"

static void uae_abort (const char *format,...)
{
    va_list parms;
    char buffer[1000];

    va_start (parms, format);
#ifdef _WIN32
    _vsnprintf( buffer, sizeof (buffer) -1, format, parms );
#else
    vsnprintf( buffer, sizeof (buffer) -1, format, parms );
#endif
    va_end (parms);
    gui_message (buffer);
}

#if 0
void customhack_put (struct customhack *ch, uae_u16 v, unsigned int hpos)
{
    ch->v = v;
    ch->vpos = vpos;
    ch->hpos = hpos;
}

uae_u16 customhack_get (struct customhack *ch, unsigned int hpos)
{
    if (ch->vpos == vpos && ch->hpos == hpos) {
	ch->vpos = -1;
	return 0xffff;
    }
    return ch->v;
}
#endif

static uae_u16 last_custom_value;

static unsigned int total_skipped = 0;

STATIC_INLINE void sync_copper (unsigned int hpos);

/* Events */

unsigned int is_lastline;

static int rpt_did_reset;

volatile frame_time_t vsynctime, vsyncmintime;

#ifdef JIT
extern uae_u8* compiled_code;
#endif

unsigned int vpos;
int hack_vpos;
static uae_u16 lof;
static int next_lineno;
static enum nln_how nextline_how;
static int lof_changed = 0;
/* Stupid genlock-detection prevention hack.
 * We should stop calling vsync_handler() and
 * hstop_handler() completely but it is not
 * worth the trouble..
 */
static int vpos_previous, hpos_previous;
static int vpos_lpen, hpos_lpen;

static uae_u32 sprtaba[256],sprtabb[256];
static uae_u32 sprite_ab_merge[256];
/* Tables for collision detection.  */
static uae_u32 sprclx[16], clxmask[16];

/*
 * Hardware registers of all sorts.
 */

static int custom_wput_1 (unsigned int, uaecptr, uae_u32, int) REGPARAM;

uae_u16 intena,intreq;
uae_u16 dmacon;
uae_u16 adkcon; /* used by audio code */

static uae_u32 cop1lc,cop2lc,copcon;

unsigned int maxhpos = MAXHPOS_PAL;
unsigned int maxvpos = MAXVPOS_PAL;
unsigned int minfirstline = VBLANK_ENDLINE_PAL;
int vblank_hz = VBLANK_HZ_PAL, fake_vblank_hz, vblank_skip;
frame_time_t syncbase;
static int fmode;
unsigned int beamcon0, new_beamcon0;
uae_u16 vtotal = MAXVPOS_PAL, htotal = MAXHPOS_PAL;
static uae_u16 hsstop, hbstrt, hbstop, vsstop, vbstrt, vbstop, hsstrt, vsstrt, hcenter;
static int interlace_started;


/* This is but an educated guess. It seems to be correct, but this stuff
 * isn't documented well. */
struct sprite {
    uaecptr pt;
    unsigned int xpos;
    unsigned int vstart;
    unsigned int vstop;
    int armed;
    int dmastate;
    int dmacycle;
};

static struct sprite spr[MAX_SPRITES];

static unsigned int sprite_vblank_endline = VBLANK_SPRITE_PAL;

static unsigned int sprctl[MAX_SPRITES], sprpos[MAX_SPRITES];
#ifdef AGA
static uae_u16 sprdata[MAX_SPRITES][4], sprdatb[MAX_SPRITES][4];
#else
static uae_u16 sprdata[MAX_SPRITES][1], sprdatb[MAX_SPRITES][1];
#endif
static unsigned int last_sprite_point;
static int nr_armed;
static unsigned int sprite_width;
static unsigned int sprres, sprite_buffer_res;

#ifdef CPUEMU_6
uae_u8 cycle_line[256];
#endif

static uae_u32 bpl1dat;
#if 0 /* useless */
static uae_u32 bpl2dat, bpl3dat, bpl4dat, bpl5dat, bpl6dat, bpl7dat, bpl8dat;
#endif
static uae_s16 bpl1mod, bpl2mod;

static uaecptr bplpt[8];
uae_u8 *real_bplpt[8];
/* Used as a debugging aid, to offset any bitplane temporarily.  */
int bpl_off[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/*static int blitcount[256];  blitter debug */

static struct color_entry current_colors;
static unsigned int bplcon0, bplcon1, bplcon2, bplcon3, bplcon4;
static unsigned int diwstrt, diwstop, diwhigh;
static int diwhigh_written;
static unsigned int ddfstrt, ddfstop, ddfstrt_old_hpos, ddfstrt_old_vpos;
static unsigned int ddf_change;

/* The display and data fetch windows */

enum diw_states
{
    DIW_waiting_start, DIW_waiting_stop
};

unsigned int plffirstline, plflastline;
unsigned int plfstrt;
unsigned int plfstop;
static int last_diw_pix_hpos, last_ddf_pix_hpos;
static int last_decide_line_hpos, last_sprite_decide_line_hpos;
static int last_fetch_hpos;
static unsigned int last_sprite_hpos;
int diwfirstword, diwlastword;
static enum diw_states diwstate, hdiwstate, ddfstate;

/* Sprite collisions */
static unsigned int clxdat, clxcon, clxcon2, clxcon_bpl_enable, clxcon_bpl_match;

enum copper_states {
    COP_stop,
    COP_read1_in2,
    COP_read1_wr_in4,
    COP_read1_wr_in2,
    COP_read1,
    COP_read2_wr_in2,
    COP_read2,
    COP_bltwait,
    COP_wait_in4,
    COP_wait_in2,
    COP_skip_in4,
    COP_skip_in2,
    COP_wait1,
    COP_wait,
    COP_skip1,
    COP_strobe_delay
};

struct copper {
    /* The current instruction words.  */
    unsigned int i1, i2;
    unsigned int saved_i1, saved_i2;
    enum copper_states state, state_prev;
    /* Instruction pointer.  */
    uaecptr ip, saved_ip;
    unsigned int hpos;
    unsigned int vpos;
    unsigned int ignore_next;
    unsigned int vcmp;
    unsigned int hcmp;

    int strobe; /* COPJMP1 / COPJMP2 accessed */
    int last_write, last_write_hpos;
};

static struct copper cop_state;
static int copper_enabled_thisline;
static int cop_min_waittime;

/*
 * Statistics
 */
unsigned int frametime;
frame_time_t lastframetime;
unsigned int timeframes;
frame_time_t idletime;
unsigned long int hsync_counter;

int bogusframe;

/* Recording of custom chip register changes.  */
static int current_change_set;

#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
/* sam: Those arrays uses around 7Mb of BSS... That seems  */
/* too much for AmigaDOS (uae crashes as soon as one loads */
/* it. So I use a different strategy here (realloc the     */
/* arrays when needed. That strategy might be usefull for  */
/* computer with low memory.                               */
struct sprite_entry *sprite_entries[2];
struct color_change *color_changes[2];
#define DEFAULT_MAX_SPRITE_ENTRY	400
#define DEFAULT_MAX_COLOR_CHANGE	400
static int max_sprite_entry;
static int delta_sprite_entry;
static int max_color_change;
static int delta_color_change;
#else
struct sprite_entry sprite_entries[2][MAX_SPR_PIXELS / 16];
struct color_change color_changes[2][MAX_REG_CHANGE];
#endif

struct decision line_decisions[2 * (MAXVPOS + 1) + 1];
struct draw_info line_drawinfo[2][2 * (MAXVPOS + 1) + 1];
struct color_entry color_tables[2][(MAXVPOS + 1) * 2];

static int next_sprite_entry = 0;
static int prev_next_sprite_entry;
static int next_sprite_forced = 1;

struct sprite_entry *curr_sprite_entries, *prev_sprite_entries;
struct color_change *curr_color_changes, *prev_color_changes;
struct draw_info *curr_drawinfo, *prev_drawinfo;
struct color_entry *curr_color_tables, *prev_color_tables;

static int next_color_change;
static int next_color_entry, remembered_color_entry;
static int color_src_match, color_dest_match, color_compare_result;

static uae_u32 thisline_changed;

#ifdef SMART_UPDATE
#define MARK_LINE_CHANGED do { thisline_changed = 1; } while (0)
#else
#define MARK_LINE_CHANGED do { ; } while (0)
#endif

static struct decision thisline_decision;
static unsigned int passed_plfstop;
static unsigned int fetch_cycle;
static int fetch_modulo_cycle;

enum fetchstate {
    fetch_not_started,
    fetch_started,
    fetch_was_plane0
} fetch_state;

/*
 * helper functions
 */

STATIC_INLINE int nodraw (void)
{
    return !currprefs.cpu_cycle_exact && framecnt != 0;
}

uae_u32 get_copper_address (int copno)
{
    switch (copno) {
    case 1: return cop1lc;
    case 2: return cop2lc;
    case -1: return cop_state.ip;
    default: return 0;
    }
}


void reset_frame_rate_hack (void)
{
    if (currprefs.m68k_speed != -1)
	return;

    rpt_did_reset = 1;
    is_lastline = 0;
    vsyncmintime = uae_gethrtime () + vsynctime;
    write_log ("Resetting frame rate hack\n");
}

STATIC_INLINE void setclr (uae_u16 *p, uae_u16 val)
{
    if (val & 0x8000)
	*p |= val & 0x7FFF;
    else
	*p &= ~val;
}

static void hsyncdelay(void)
{
#if 0
    static unsigned int prevhpos;
    while (current_hpos () == prevhpos)
        do_cycles (CYCLE_UNIT);
    prevhpos = current_hpos ();
#endif
}

STATIC_INLINE unsigned int current_hpos (void)
{
    return (get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT;
}

STATIC_INLINE uae_u8 *pfield_xlateptr (uaecptr plpt, int bytecount)
{
    plpt -= chipmem_start & chipmem_mask;
    plpt &= chipmem_mask;
    if ((plpt + bytecount) <= allocated_chipmem)
	return chipmemory + plpt;
    else {
	static int count = 0;
	if (!count) {
	    count++;
	    write_log ("Warning: Bad playfield pointer\n");
	}
	return 0;
    }
}

STATIC_INLINE void docols (struct color_entry *colentry)
{
    int i;

#ifdef AGA
    if (currprefs.chipset_mask & CSMASK_AGA) {
	for (i = 0; i < 256; i++) {
	    int v = color_reg_get (colentry, i);
	    if (v < 0 || v > 16777215)
		continue;
	    colentry->acolors[i] = CONVERT_RGB (v);
	}
    } else {
#endif
	for (i = 0; i < 32; i++) {
	    int v = color_reg_get (colentry, i);
	    if (v < 0 || v > 4095)
		continue;
	    colentry->acolors[i] = xcolors[v];
	}
#ifdef AGA
    }
#endif
}

void notice_new_xcolors (void)
{
    int i;

    docols(&current_colors);
/*    docols(&colors_for_drawing);*/
    for (i = 0; i < (MAXVPOS + 1)*2; i++) {
	docols(color_tables[0]+i);
	docols(color_tables[1]+i);
    }
}

static void do_sprites (unsigned int currhp);

static void remember_ctable (void)
{
    if (remembered_color_entry == -1) {
	/* The colors changed since we last recorded a color map. Record a
	 * new one. */
	color_reg_cpy (curr_color_tables + next_color_entry, &current_colors);
	remembered_color_entry = next_color_entry++;
    }
    thisline_decision.ctable = remembered_color_entry;
    if (color_src_match == -1 || color_dest_match != remembered_color_entry
	|| line_decisions[next_lineno].ctable != color_src_match)
    {
	/* The remembered comparison didn't help us - need to compare again. */
	int oldctable = line_decisions[next_lineno].ctable;
	int changed = 0;

	if (oldctable == -1) {
	    changed = 1;
	    color_src_match = color_dest_match = -1;
	} else {
	    color_compare_result = color_reg_cmp (&prev_color_tables[oldctable], &current_colors) != 0;
	    if (color_compare_result)
		changed = 1;
	    color_src_match = oldctable;
	    color_dest_match = remembered_color_entry;
	}
	thisline_changed |= changed;
    } else {
	/* We know the result of the comparison */
	if (color_compare_result)
	    thisline_changed = 1;
    }
}

static void remember_ctable_for_border (void)
{
    remember_ctable ();
}

/* Called to determine the state of the horizontal display window state
 * machine at the current position. It might have changed since we last
 * checked.  */
static void decide_diw (unsigned int hpos)
{
    int pix_hpos = coord_diw_to_window_x (hpos == 227 ? 455 : hpos * 2); /* (227.5*2 = 455) */
    if (hdiwstate == DIW_waiting_start && thisline_decision.diwfirstword == -1
	&& pix_hpos >= diwfirstword && last_diw_pix_hpos < diwfirstword)
    {
	thisline_decision.diwfirstword = diwfirstword < 0 ? 0 : diwfirstword;
	hdiwstate = DIW_waiting_stop;
    }
    if (hdiwstate == DIW_waiting_stop && thisline_decision.diwlastword == -1
	&& pix_hpos >= diwlastword && last_diw_pix_hpos < diwlastword)
    {
	thisline_decision.diwlastword = diwlastword < 0 ? 0 : diwlastword;
	hdiwstate = DIW_waiting_start;
    }
    last_diw_pix_hpos = pix_hpos;
}

static int fetchmode;
static int real_bitplane_number[3][3][9];

/* Disable bitplane DMA if planes > available DMA slots. This is needed
   e.g. by the Sanity WOC demo (at the "Party Effect").  */
STATIC_INLINE int GET_PLANES_LIMIT (uae_u16 bc0)
{
    int res = GET_RES(bc0);
    int planes = GET_PLANES(bc0);
    return real_bitplane_number[fetchmode][res][planes];
}

/* The HRM says 0xD8, but that can't work... */
#define HARD_DDF_STOP 0xd4
#define HARD_DDF_START 0x18

static void add_modulos (void)
{
    int m1, m2;

    if (fmode & 0x4000) {
	if (((diwstrt >> 8) ^ vpos) & 1)
	    m1 = m2 = bpl2mod;
	else
	    m1 = m2 = bpl1mod;
    } else {
	m1 = bpl1mod;
	m2 = bpl2mod;
    }

    switch (GET_PLANES_LIMIT (bplcon0)) {
#ifdef AGA
	case 8: bplpt[7] += m2;
	case 7: bplpt[6] += m1;
#endif
	case 6: bplpt[5] += m2;
	case 5: bplpt[4] += m1;
	case 4: bplpt[3] += m2;
	case 3: bplpt[2] += m1;
	case 2: bplpt[1] += m2;
	case 1: bplpt[0] += m1;
    }
}

static void finish_playfield_line (void)
{
    /* The latter condition might be able to happen in interlaced frames. */
    if (vpos >= minfirstline && (thisframe_first_drawn_line == -1 || (int)vpos < thisframe_first_drawn_line))
	thisframe_first_drawn_line = vpos;
    thisframe_last_drawn_line = vpos;

#ifdef SMART_UPDATE
    if (line_decisions[next_lineno].plflinelen != thisline_decision.plflinelen
	|| line_decisions[next_lineno].plfleft != thisline_decision.plfleft
	|| line_decisions[next_lineno].bplcon0 != thisline_decision.bplcon0
	|| line_decisions[next_lineno].bplcon2 != thisline_decision.bplcon2
#ifdef AGA
	|| line_decisions[next_lineno].bplcon3 != thisline_decision.bplcon3
	|| line_decisions[next_lineno].bplcon4 != thisline_decision.bplcon4
#endif
	)
#endif /* SMART_UPDATE */
	thisline_changed = 1;
}

/* The fetch unit mainly controls ddf stop.  It's the number of cycles that
   are contained in an indivisible block during which ddf is active.  E.g.
   if DDF starts at 0x30, and fetchunit is 8, then possible DDF stops are
   0x30 + n * 8.  */
static unsigned int fetchunit, fetchunit_mask;
/* The delay before fetching the same bitplane again.  Can be larger than
   the number of bitplanes; in that case there are additional empty cycles
   with no data fetch (this happens for high fetchmodes and low
   resolutions).  */
static unsigned int fetchstart, fetchstart_shift, fetchstart_mask;
/* fm_maxplane holds the maximum number of planes possible with the current
   fetch mode.  This selects the cycle diagram:
   8 planes: 73516240
   4 planes: 3120
   2 planes: 10.  */
static int fm_maxplane, fm_maxplane_shift;

/* The corresponding values, by fetchmode and display resolution.  */
static unsigned int fetchunits[] = { 8,8,8,0, 16,8,8,0, 32,16,8,0 };
static unsigned int fetchstarts[] = { 3,2,1,0, 4,3,2,0, 5,4,3,0 };
static unsigned int fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

static uae_u8 cycle_diagram_table[3][3][9][32];
static uae_u8 cycle_diagram_free_cycles[3][3][9];
static uae_u8 cycle_diagram_total_cycles[3][3][9];
static uae_u8 *curr_diagram;
static uae_u8 cycle_sequences[3 * 8] = { 2,1,2,1,2,1,2,1, 4,2,3,1,4,2,3,1, 8,4,6,2,7,3,5,1 };

#if 0
static void debug_cycle_diagram (void)
{
    unsigned int fm, res, planes, cycle, v;
    char aa;

    for (fm = 0; fm < 3; fm++) {
	write_log ("FMODE %d\n=======\n", fm);
	for (res = 0; res <= 2; res++) {
	    for (planes = 0; planes <= 8; planes++) {
		write_log("%d: ",planes);
		for (cycle = 0; cycle < 32; cycle++) {
		    v=cycle_diagram_table[fm][res][planes][cycle];
		    if (v==0) aa='-'; else if(v>0) aa='1'; else aa='X';
		    write_log("%c",aa);
		}
		write_log(" %d:%d\n",
		    cycle_diagram_free_cycles[fm][res][planes], cycle_diagram_total_cycles[fm][res][planes]);
	    }
	    write_log("\n");
	}
    }
    fm=0;
}
#endif

static void create_cycle_diagram_table(void)
{
    unsigned int fm, res, cycle, planes, rplanes, v;
    unsigned int fetch_start, max_planes, freecycles;
    uae_u8 *cycle_sequence;

    for (fm = 0; fm <= 2; fm++) {
	for (res = 0; res <= 2; res++) {
	    max_planes = fm_maxplanes[fm * 4 + res];
	    fetch_start = 1 << fetchstarts[fm * 4 + res];
	    cycle_sequence = &cycle_sequences[(max_planes - 1) * 8];
	    max_planes = 1 << max_planes;
	    for (planes = 0; planes <= 8; planes++) {
		freecycles = 0;
		for (cycle = 0; cycle < 32; cycle++)
		    cycle_diagram_table[fm][res][planes][cycle] = -1;
		if (planes <= max_planes) {
		    for (cycle = 0; cycle < fetch_start; cycle++) {
			if (cycle < max_planes && planes >= cycle_sequence[cycle & 7]) {
			    v = cycle_sequence[cycle & 7];
			} else {
			    v = 0;
			    freecycles++;
			}
			cycle_diagram_table[fm][res][planes][cycle] = v;
		    }
		}
		cycle_diagram_free_cycles[fm][res][planes] = freecycles;
		cycle_diagram_total_cycles[fm][res][planes] = fetch_start;
		rplanes = planes;
		if (rplanes > max_planes)
		    rplanes = 0;
		if (rplanes == 7 && fm == 0 && res == 0 && !(currprefs.chipset_mask & CSMASK_AGA))
		    rplanes = 4;
		real_bitplane_number[fm][res][planes] = rplanes;
	    }
	}
    }
#if 0
    debug_cycle_diagram ();
#endif
}


/* Used by the copper.  */
static unsigned int estimated_last_fetch_cycle;
static unsigned int cycle_diagram_shift;

static void estimate_last_fetch_cycle (unsigned int hpos)
{
    unsigned int fetchunit = fetchunits[fetchmode * 4 + GET_RES (bplcon0)];

    if (! passed_plfstop) {
	unsigned int stop = plfstop < hpos || plfstop > HARD_DDF_STOP ? HARD_DDF_STOP : plfstop;
	/* We know that fetching is up-to-date up until hpos, so we can use fetch_cycle.  */
	unsigned int fetch_cycle_at_stop = fetch_cycle + (stop - hpos);
	unsigned int starting_last_block_at = (fetch_cycle_at_stop + fetchunit - 1) & ~(fetchunit - 1);

	estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + fetchunit;
    } else {
	unsigned int starting_last_block_at = (fetch_cycle + fetchunit - 1) & ~(fetchunit - 1);
	if (passed_plfstop == 2)
	    starting_last_block_at -= fetchunit;

	estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + fetchunit;
    }
}

static uae_u32 outword[MAX_PLANES];
static int out_nbits, out_offs;
static uae_u32 todisplay[MAX_PLANES][4];
static uae_u32 fetched[MAX_PLANES];
#ifdef AGA
static uae_u32 fetched_aga0[MAX_PLANES];
static uae_u32 fetched_aga1[MAX_PLANES];
#endif

/* Expansions from bplcon0/bplcon1.  */
static int toscr_res, toscr_nr_planes, fetchwidth;
static int toscr_delay1x, toscr_delay2x, toscr_delay1, toscr_delay2;

/* The number of bits left from the last fetched words.
   This is an optimization - conceptually, we have to make sure the result is
   the same as if toscr is called in each clock cycle.  However, to speed this
   up, we accumulate display data; this variable keeps track of how much.
   Thus, once we do call toscr_nbits (which happens at least every 16 bits),
   we can do more work at once.  */
static int toscr_nbits;

/* undocumented bitplane delay hardware feature */
static int delayoffset;

STATIC_INLINE void compute_delay_offset (void)
{
    delayoffset = (16 << fetchmode) - (((plfstrt - HARD_DDF_START) & fetchstart_mask) << 1);
#if 0
	/* maybe we can finally get rid of this stupid table.. */
	if (tmp == 4)
	delayoffset = 4; // Loons Docs
    else if (tmp == 8)
	delayoffset = 8;
    else if (tmp == 12) // Loons Docs
	delayoffset = 4;
    else if (tmp == 16) /* Overkill AGA */
	delayoffset = 48;
    else if (tmp == 24) /* AB 2 */
	delayoffset = 8;
    else if (tmp == 32)
	delayoffset = 32;
    else if (tmp == 48) /* Pinball Illusions AGA, ingame */
	delayoffset = 16;
    else /* what about 40 and 56? */
	delayoffset = 0;
    //write_log("%d:%d ", vpos, delayoffset);
#endif
}

static void expand_fmodes (void)
{
    int res = GET_RES(bplcon0);
    int fm = fetchmode;
    fetchunit = fetchunits[fm * 4 + res];
    fetchunit_mask = fetchunit - 1;
    fetchstart_shift = fetchstarts[fm * 4 + res];
    fetchstart = 1 << fetchstart_shift;
    fetchstart_mask = fetchstart - 1;
    fm_maxplane_shift = fm_maxplanes[fm * 4 + res];
    fm_maxplane = 1 << fm_maxplane_shift;
    curr_diagram = cycle_diagram_table[fm][res][GET_PLANES_LIMIT (bplcon0)];
    fetch_modulo_cycle = fetchunit - fetchstart;
}

/* Expand bplcon0/bplcon1 into the toscr_xxx variables.  */
static void compute_toscr_delay_1 (void)
{
    int delay1 = (bplcon1 & 0x0f) | ((bplcon1 & 0x0c00) >> 6);
    int delay2 = ((bplcon1 >> 4) & 0x0f) | (((bplcon1 >> 4) & 0x0c00) >> 6);
    int delaymask;
    int fetchwidth = 16 << fetchmode;

    delay1 += delayoffset;
    delay2 += delayoffset;
    delaymask = (fetchwidth - 1) >> toscr_res;
    toscr_delay1x = (delay1 & delaymask) << toscr_res;
    toscr_delay2x = (delay2 & delaymask) << toscr_res;
}

static void compute_toscr_delay (int hpos)
{
    toscr_res = GET_RES (bplcon0);
    toscr_nr_planes = GET_PLANES_LIMIT (bplcon0);
    compute_toscr_delay_1 ();
}

STATIC_INLINE void maybe_first_bpl1dat (unsigned int hpos)
{
    if (thisline_decision.plfleft == -1) {
	thisline_decision.plfleft = hpos;
	compute_delay_offset ();
	compute_toscr_delay_1 ();
    }
}

STATIC_INLINE void fetch (int nr, int fm)
{
    uaecptr p;
    if (nr >= toscr_nr_planes)
	return;
    p = bplpt[nr];
    switch (fm) {
    case 0:
	fetched[nr] = last_custom_value = chipmem_wget (p);
	bplpt[nr] += 2;
	break;
#ifdef AGA
    case 1:
	fetched_aga0[nr] = chipmem_lget (p);
	last_custom_value = (uae_u16)fetched_aga0[nr];
	bplpt[nr] += 4;
	break;
    case 2:
	fetched_aga1[nr] = chipmem_lget (p);
	fetched_aga0[nr] = chipmem_lget (p + 4);
	last_custom_value = (uae_u16)fetched_aga0[nr];
	bplpt[nr] += 8;
	break;
#endif
    }
    if (passed_plfstop == 2 && fetch_cycle >= (fetch_cycle & ~fetchunit_mask) + fetch_modulo_cycle) {
	int mod;
	if (fmode & 0x4000) {
	    if (((diwstrt >> 8) ^ vpos) & 1)
		mod = bpl2mod;
	    else
		mod = bpl1mod;
	} else if (nr & 1)
	    mod = bpl2mod;
	else
	    mod = bpl1mod;
	bplpt[nr] += mod;
    }
    if (nr == 0)
	fetch_state = fetch_was_plane0;
}

static void clear_fetchbuffer (uae_u32 *ptr, int nwords)
{
    int i;

    if (! thisline_changed)
	for (i = 0; i < nwords; i++)
	    if (ptr[i]) {
		thisline_changed = 1;
		break;
	    }

    memset (ptr, 0, nwords * 4);
}

static void update_toscr_planes (void)
{
    if (toscr_nr_planes > thisline_decision.nr_planes) {
	int j;
	for (j = thisline_decision.nr_planes; j < toscr_nr_planes; j++)
	    clear_fetchbuffer ((uae_u32 *)(line_data[next_lineno] + 2 * MAX_WORDS_PER_LINE * j), out_offs);
#if 0
	if (thisline_decision.nr_planes > 0)
	    printf ("Planes from %d to %d\n", thisline_decision.nr_planes, toscr_nr_planes);
#endif
	thisline_decision.nr_planes = toscr_nr_planes;
    }
}

STATIC_INLINE void toscr_3_ecs (int nbits)
{
    int delay1 = toscr_delay1;
    int delay2 = toscr_delay2;
    int i;
    uae_u32 mask = 0xFFFF >> (16 - nbits);

    for (i = 0; i < toscr_nr_planes; i += 2) {
	outword[i] <<= nbits;
	outword[i] |= (todisplay[i][0] >> (16 - nbits + delay1)) & mask;
	todisplay[i][0] <<= nbits;
    }
    for (i = 1; i < toscr_nr_planes; i += 2) {
	outword[i] <<= nbits;
	outword[i] |= (todisplay[i][0] >> (16 - nbits + delay2)) & mask;
	todisplay[i][0] <<= nbits;
    }
}

STATIC_INLINE void shift32plus (uae_u32 *p, int n)
{
    uae_u32 t = p[1];
    t <<= n;
    t |= p[0] >> (32 - n);
    p[1] = t;
}

#ifdef AGA
STATIC_INLINE void aga_shift (uae_u32 *p, int n, int fm)
{
    if (fm == 2) {
	shift32plus (p + 2, n);
	shift32plus (p + 1, n);
    }
    shift32plus (p + 0, n);
    p[0] <<= n;
}

STATIC_INLINE void toscr_3_aga (int nbits, int fm)
{
    int delay1 = toscr_delay1;
    int delay2 = toscr_delay2;
    int i;
    uae_u32 mask = 0xFFFF >> (16 - nbits);

    {
	int offs = (16 << fm) - nbits + delay1;
	int off1 = offs >> 5;
	if (off1 == 3)
	    off1 = 2;
	offs -= off1 * 32;
	for (i = 0; i < toscr_nr_planes; i += 2) {
	    uae_u32 t0 = todisplay[i][off1];
	    uae_u32 t1 = todisplay[i][off1 + 1];
	    uae_u64 t = (((uae_u64)t1) << 32) | t0;
	    outword[i] <<= nbits;
	    outword[i] |= (t >> offs) & mask;
	    aga_shift (todisplay[i], nbits, fm);
	}
    }
    {
	int offs = (16 << fm) - nbits + delay2;
	int off1 = offs >> 5;
	if (off1 == 3)
	    off1 = 2;
	offs -= off1 * 32;
	for (i = 1; i < toscr_nr_planes; i += 2) {
	    uae_u32 t0 = todisplay[i][off1];
	    uae_u32 t1 = todisplay[i][off1 + 1];
	    uae_u64 t = (((uae_u64)t1) << 32) | t0;
	    outword[i] <<= nbits;
	    outword[i] |= (t >> offs) & mask;
	    aga_shift (todisplay[i], nbits, fm);
	}
    }
}

#endif

static void toscr_2_0 (int nbits) { toscr_3_ecs (nbits); }
#ifdef AGA
static void toscr_2_1 (int nbits) { toscr_3_aga (nbits, 1); }
static void toscr_2_2 (int nbits) { toscr_3_aga (nbits, 2); }
#endif

STATIC_INLINE void toscr_1 (int nbits, int fm)
{
    switch (fm) {
    case 0:
	toscr_2_0 (nbits);
	break;
#ifdef AGA
    case 1:
	toscr_2_1 (nbits);
	break;
    case 2:
	toscr_2_2 (nbits);
	break;
#endif
    }
    out_nbits += nbits;
    if (out_nbits == 32) {
	int i;
	uae_u8 *dataptr = line_data[next_lineno] + out_offs * 4;
	for (i = 0; i < thisline_decision.nr_planes; i++) {
	    uae_u32 *dataptr32 = (uae_u32 *)dataptr;
	    if (i >= toscr_nr_planes)
		outword[i] = 0;
	    if (*dataptr32 != outword[i])
		thisline_changed = 1;
	    *dataptr32 = outword[i];
	    dataptr += MAX_WORDS_PER_LINE * 2;
	}
	out_offs++;
	out_nbits = 0;
    }
}

static void toscr_fm0 (int);
static void toscr_fm1 (int);
static void toscr_fm2 (int);

STATIC_INLINE void toscr (int nbits, int fm)
{
    switch (fm) {
    case 0: toscr_fm0 (nbits); break;
#ifdef AGA
    case 1: toscr_fm1 (nbits); break;
    case 2: toscr_fm2 (nbits); break;
#endif
    }
}

STATIC_INLINE void toscr_0 (int nbits, int fm)
{
    int t;

    if (nbits > 16) {
	toscr (16, fm);
	nbits -= 16;
    }

    t = 32 - out_nbits;
    if (t < nbits) {
	toscr_1 (t, fm);
	nbits -= t;
    }
    toscr_1 (nbits, fm);
}

static void toscr_fm0 (int nbits) { toscr_0 (nbits, 0); }
static void toscr_fm1 (int nbits) { toscr_0 (nbits, 1); }
static void toscr_fm2 (int nbits) { toscr_0 (nbits, 2); }

static int flush_plane_data (int fm)
{
    int i = 0;
    int fetchwidth = 16 << fm;

    if (out_nbits <= 16) {
	i += 16;
	toscr_1 (16, fm);
    }
    if (out_nbits != 0) {
	i += 32 - out_nbits;
	toscr_1 (32 - out_nbits, fm);
    }
    i += 32;

    toscr_1 (16, fm);
    toscr_1 (16, fm);
    return i >> (1 + toscr_res);
}

STATIC_INLINE void flush_display (int fm)
{
    if (toscr_nbits > 0 && thisline_decision.plfleft != -1)
	toscr (toscr_nbits, fm);
    toscr_nbits = 0;
}

STATIC_INLINE void fetch_start (int hpoa)
{
    fetch_state = fetch_started;
}

/* Called when all planes have been fetched, i.e. when a new block
   of data is available to be displayed.  The data in fetched[] is
   moved into todisplay[].  */
STATIC_INLINE void beginning_of_plane_block (int pos, int fm)
{
    int i;

    flush_display (fm);
    if (fm == 0)
	for (i = 0; i < MAX_PLANES; i++)
	    todisplay[i][0] |= fetched[i];
#ifdef AGA
    else
	for (i = 0; i < MAX_PLANES; i++) {
	    if (fm == 2)
		todisplay[i][1] = fetched_aga1[i];
	    todisplay[i][0] = fetched_aga0[i];
	}
#endif
    maybe_first_bpl1dat (pos);
    toscr_delay1 = toscr_delay1x;
    toscr_delay2 = toscr_delay2x;
}

#ifdef SPEEDUP

/* The usual inlining tricks - don't touch unless you know what you are doing. */
STATIC_INLINE void long_fetch_ecs (int plane, int nwords, int weird_number_of_bits, int dma)
{
    uae_u16 *real_pt = (uae_u16 *)pfield_xlateptr (bplpt[plane] + bpl_off[plane], nwords * 2);
    int delay = (plane & 1) ? toscr_delay2 : toscr_delay1;
    int tmp_nbits = out_nbits;
    uae_u32 shiftbuffer = todisplay[plane][0];
    uae_u32 outval = outword[plane];
    uae_u32 fetchval = fetched[plane];
    uae_u32 *dataptr = (uae_u32 *)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);

    if (dma)
	bplpt[plane] += nwords * 2;

    if (real_pt == 0)
	/* @@@ Don't do this, fall back on chipmem_wget instead.  */
	return;

    while (nwords > 0) {
	int bits_left = 32 - tmp_nbits;
	uae_u32 t;

	shiftbuffer |= fetchval;

	t = (shiftbuffer >> delay) & 0xFFFF;

	if (weird_number_of_bits && bits_left < 16) {
	    outval <<= bits_left;
	    outval |= t >> (16 - bits_left);
	    thisline_changed |= *dataptr ^ outval;
	    *dataptr++ = outval;

	    outval = t;
	    tmp_nbits = 16 - bits_left;
	    shiftbuffer <<= 16;
	} else {
	    outval = (outval << 16) | t;
	    shiftbuffer <<= 16;
	    tmp_nbits += 16;
	    if (tmp_nbits == 32) {
		thisline_changed |= *dataptr ^ outval;
		*dataptr++ = outval;
		tmp_nbits = 0;
	    }
	}
	nwords--;
	if (dma) {
	    fetchval = do_get_mem_word (real_pt);
	    real_pt++;
	}
    }
    fetched[plane] = fetchval;
    todisplay[plane][0] = shiftbuffer;
    outword[plane] = outval;
}

#ifdef AGA
STATIC_INLINE void long_fetch_aga (int plane, int nwords, int weird_number_of_bits, int fm, int dma)
{
    uae_u32 *real_pt = (uae_u32 *)pfield_xlateptr (bplpt[plane] + bpl_off[plane], nwords * 2);
    int delay = (plane & 1) ? toscr_delay2 : toscr_delay1;
    int tmp_nbits = out_nbits;
    uae_u32 *shiftbuffer = todisplay[plane];
    uae_u32 outval = outword[plane];
    uae_u32 fetchval0 = fetched_aga0[plane];
    uae_u32 fetchval1 = fetched_aga1[plane];
    uae_u32 *dataptr = (uae_u32 *)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);
    int offs = (16 << fm) - 16 + delay;
    int off1 = offs >> 5;
    if (off1 == 3)
	off1 = 2;
    offs -= off1 * 32;

    if (dma)
	bplpt[plane] += nwords * 2;

    if (real_pt == 0)
	/* @@@ Don't do this, fall back on chipmem_wget instead.  */
	return;

    while (nwords > 0) {
	int i;

	shiftbuffer[0] = fetchval0;
	if (fm == 2)
	    shiftbuffer[1] = fetchval1;

	for (i = 0; i < (1 << fm); i++) {
	    int bits_left = 32 - tmp_nbits;

	    uae_u32 t0 = shiftbuffer[off1];
	    uae_u32 t1 = shiftbuffer[off1 + 1];
	    uae_u64 t = (((uae_u64)t1) << 32) | t0;

	    t0 = (uae_u32)((t >> offs) & 0xFFFF);

	    if (weird_number_of_bits && bits_left < 16) {
		outval <<= bits_left;
		outval |= t0 >> (16 - bits_left);

		thisline_changed |= *dataptr ^ outval;
		*dataptr++ = outval;

		outval = t0;
		tmp_nbits = 16 - bits_left;
		aga_shift (shiftbuffer, 16, fm);
	    } else {
		outval = (outval << 16) | t0;
		aga_shift (shiftbuffer, 16, fm);
		tmp_nbits += 16;
		if (tmp_nbits == 32) {
		    thisline_changed |= *dataptr ^ outval;
		    *dataptr++ = outval;
		    tmp_nbits = 0;
		}
	    }
	}

	nwords -= 1 << fm;

	if (dma) {
	    if (fm == 1)
		fetchval0 = do_get_mem_long (real_pt);
	    else {
		fetchval1 = do_get_mem_long (real_pt);
		fetchval0 = do_get_mem_long (real_pt + 1);
	    }
	    real_pt += fm;
	}
    }
    fetched_aga0[plane] = fetchval0;
    fetched_aga1[plane] = fetchval1;
    outword[plane] = outval;
}
#endif

static void NOINLINE long_fetch_ecs_0 (int hpos, int nwords, int dma) { long_fetch_ecs (hpos, nwords, 0, dma); }
static void NOINLINE long_fetch_ecs_1 (int hpos, int nwords, int dma) { long_fetch_ecs (hpos, nwords, 1, dma); }
#ifdef AGA
static void NOINLINE long_fetch_aga_1_0 (int hpos, int nwords, int dma) { long_fetch_aga (hpos, nwords, 0, 1, dma); }
static void NOINLINE long_fetch_aga_1_1 (int hpos, int nwords, int dma) { long_fetch_aga (hpos, nwords, 1, 1, dma); }
static void NOINLINE long_fetch_aga_2_0 (int hpos, int nwords, int dma) { long_fetch_aga (hpos, nwords, 0, 2, dma); }
static void NOINLINE long_fetch_aga_2_1 (int hpos, int nwords, int dma) { long_fetch_aga (hpos, nwords, 1, 2, dma); }
#endif

static void do_long_fetch (int hpos, int nwords, int dma, int fm)
{
    int i;

    flush_display (fm);
    switch (fm) {
    case 0:
	if (out_nbits & 15) {
	    for (i = 0; i < toscr_nr_planes; i++)
		long_fetch_ecs_1 (i, nwords, dma);
	} else {
	    for (i = 0; i < toscr_nr_planes; i++)
		long_fetch_ecs_0 (i, nwords, dma);
	}
	break;
#ifdef AGA
    case 1:
	if (out_nbits & 15) {
	    for (i = 0; i < toscr_nr_planes; i++)
		long_fetch_aga_1_1 (i, nwords, dma);
	} else {
	    for (i = 0; i < toscr_nr_planes; i++)
		long_fetch_aga_1_0 (i, nwords, dma);
	}
	break;
    case 2:
	if (out_nbits & 15) {
	    for (i = 0; i < toscr_nr_planes; i++)
		long_fetch_aga_2_1 (i, nwords, dma);
	} else {
	    for (i = 0; i < toscr_nr_planes; i++)
		long_fetch_aga_2_0 (i, nwords, dma);
	}
	break;
#endif
    }

    out_nbits += nwords * 16;
    out_offs += out_nbits >> 5;
    out_nbits &= 31;

    if (dma && toscr_nr_planes > 0)
	fetch_state = fetch_was_plane0;
}

#endif

/* make sure fetch that goes beyond maxhpos is finished */
static void finish_final_fetch (int i, int fm)
{
    if (thisline_decision.plfleft == -1)
	return;
    if (passed_plfstop == 3)
	return;
    passed_plfstop = 3;
    ddfstate = DIW_waiting_start;
    i += flush_plane_data (fm);
    thisline_decision.plfright = i;
    thisline_decision.plflinelen = out_offs;
    finish_playfield_line ();
}

STATIC_INLINE int one_fetch_cycle_0 (int i, int ddfstop_to_test, int dma, int fm)
{
    if (! passed_plfstop && i == ddfstop_to_test)
	passed_plfstop = 1;

    if ((fetch_cycle & fetchunit_mask) == 0) {
	if (passed_plfstop == 2) {
	    finish_final_fetch (i, fm);
	    return 1;
	}
	if (passed_plfstop)
	    passed_plfstop++;
    }

    if (dma) {
	/* fetchstart_mask can be larger than fm_maxplane if FMODE > 0.  This means
	   that the remaining cycles are idle; we'll fall through the whole switch
	   without doing anything.  */
	int cycle_start = fetch_cycle & fetchstart_mask;
	switch (fm_maxplane) {
	case 8:
	    switch (cycle_start) {
	    case 0: fetch (7, fm); break;
	    case 1: fetch (3, fm); break;
	    case 2: fetch (5, fm); break;
	    case 3: fetch (1, fm); break;
	    case 4: fetch (6, fm); break;
	    case 5: fetch (2, fm); break;
	    case 6: fetch (4, fm); break;
	    case 7: fetch (0, fm); break;
	    }
	    break;
	case 4:
	    switch (cycle_start) {
	    case 0: fetch (3, fm); break;
	    case 1: fetch (1, fm); break;
	    case 2: fetch (2, fm); break;
	    case 3: fetch (0, fm); break;
	    }
	    break;
	case 2:
	    switch (cycle_start) {
	    case 0: fetch (1, fm); break;
	    case 1: fetch (0, fm); break;
	    }
	    break;
	}
    }
    fetch_cycle++;
    toscr_nbits += 2 << toscr_res;

    if (toscr_nbits > 16) {
	uae_abort ("toscr_nbits > 16 (%d)", toscr_nbits);
	toscr_nbits = 0;
    }
    if (toscr_nbits == 16)
	flush_display (fm);

    return 0;
}

static int one_fetch_cycle_fm0 (int i, int ddfstop_to_test, int dma) { return one_fetch_cycle_0 (i, ddfstop_to_test, dma, 0); }
static int one_fetch_cycle_fm1 (int i, int ddfstop_to_test, int dma) { return one_fetch_cycle_0 (i, ddfstop_to_test, dma, 1); }
static int one_fetch_cycle_fm2 (int i, int ddfstop_to_test, int dma) { return one_fetch_cycle_0 (i, ddfstop_to_test, dma, 2); }

STATIC_INLINE int one_fetch_cycle (int i, int ddfstop_to_test, int dma, int fm)
{
    switch (fm) {
    case 0: return one_fetch_cycle_fm0 (i, ddfstop_to_test, dma);
#ifdef AGA
    case 1: return one_fetch_cycle_fm1 (i, ddfstop_to_test, dma);
    case 2: return one_fetch_cycle_fm2 (i, ddfstop_to_test, dma);
#endif
    default: uae_abort ("fm corrupt"); return 0;
    }
}

STATIC_INLINE void update_fetch (unsigned int until, int fm)
{
    unsigned int pos;
    unsigned int dma = dmaen (DMA_BITPLANE);

    unsigned int ddfstop_to_test;

    if (nodraw() || passed_plfstop == 3)
	return;

    /* We need an explicit test against HARD_DDF_STOP here to guard against
       programs that move the DDFSTOP before our current position before we
       reach it.  */
    ddfstop_to_test = HARD_DDF_STOP;
    if ((int)ddfstop >= last_fetch_hpos && plfstop < ddfstop_to_test)
	ddfstop_to_test = plfstop;

    compute_toscr_delay (last_fetch_hpos);
    update_toscr_planes ();

    pos = last_fetch_hpos;
    cycle_diagram_shift = last_fetch_hpos - fetch_cycle;

    /* First, a loop that prepares us for the speedup code.  We want to enter
       the SPEEDUP case with fetch_state == fetch_was_plane0, and then unroll
       whole blocks, so that we end on the same fetch_state again.  */
    for (; ; pos++) {
	if (pos == until) {
	    if (until >= maxhpos) {
		finish_final_fetch (pos, fm);
		return;
	    }
	    flush_display (fm);
	    return;
	}

	if (fetch_state == fetch_was_plane0)
	    break;

	fetch_start (pos);
	if (one_fetch_cycle (pos, ddfstop_to_test, dma, fm))
	    return;
    }

#ifdef SPEEDUP
    /* Unrolled version of the for loop below.  */
    if (! passed_plfstop && ddf_change != vpos && ddf_change + 1 != vpos
	&& dma
	&& (fetch_cycle & fetchstart_mask) == (fm_maxplane & fetchstart_mask)
	&& toscr_delay1 == toscr_delay1x && toscr_delay2 == toscr_delay2x
 # if 0
	/* @@@ We handle this case, but the code would be simpler if we
	 * disallowed it - it may even be possible to guarantee that
	 * this condition never is false.  Later.  */
	&& (out_nbits & 15) == 0
# endif
	&& toscr_nr_planes == thisline_decision.nr_planes)
    {
	unsigned int offs = (pos - fetch_cycle) & fetchunit_mask;
	unsigned int ddf2 = ((ddfstop_to_test - offs + fetchunit - 1) & ~fetchunit_mask) + offs;
	unsigned int ddf3 = ddf2 + fetchunit;
	unsigned int stop = until < ddf2 ? until : until < ddf3 ? ddf2 : ddf3;
	unsigned int count;

	count = stop - pos;

	if (count >= fetchstart) {
	    count &= ~fetchstart_mask;

	    if (thisline_decision.plfleft == -1) {
		compute_delay_offset ();
		compute_toscr_delay_1 ();
	    }

	    do_long_fetch (pos, count >> (3 - toscr_res), dma, fm);

	    /* This must come _after_ do_long_fetch so as not to confuse flush_display
	       into thinking the first fetch has produced any output worth emitting to
	       the screen.  But the calculation of delay_offset must happen _before_.  */
	    maybe_first_bpl1dat (pos);

	    if (pos <= ddfstop_to_test && pos + count > ddfstop_to_test)
		passed_plfstop = 1;
	    if (pos <= ddfstop_to_test && pos + count > ddf2)
		passed_plfstop = 2;
	    if (pos <= ddf2 && pos + count >= ddf2 + fm_maxplane)
		add_modulos ();
	    pos += count;
	    fetch_cycle += count;
	}
    } else {
#endif
	//maybe_first_bpl1dat (pos);
#ifdef SPEEDUP
    }
#endif
    for (; pos < until; pos++) {
	if (fetch_state == fetch_was_plane0)
	    beginning_of_plane_block (pos, fm);
	fetch_start (pos);

	if (one_fetch_cycle (pos, ddfstop_to_test, dma, fm))
	    return;
    }
    if (until >= maxhpos) {
	finish_final_fetch (pos, fm);
	return;
    }
    flush_display (fm);
}

static void update_fetch_0 (int hpos) { update_fetch (hpos, 0); }
static void update_fetch_1 (int hpos) { update_fetch (hpos, 1); }
static void update_fetch_2 (int hpos) { update_fetch (hpos, 2); }

STATIC_INLINE void decide_fetch (int hpos)
{
    if (fetch_state != fetch_not_started && hpos > last_fetch_hpos) {
	switch (fetchmode) {
	case 0: update_fetch_0 (hpos); break;
#ifdef AGA
	case 1: update_fetch_1 (hpos); break;
	case 2: update_fetch_2 (hpos); break;
#endif
	default: uae_abort ("fetchmode corrupt");
	}
    }
    last_fetch_hpos = hpos;
}

static void start_bpl_dma (unsigned int hpos, int hstart)
{
    fetch_start (hpos);
    fetch_cycle = 0;
    last_fetch_hpos = hstart;
    out_nbits = 0;
    out_offs = 0;
    toscr_nbits = 0;
    thisline_decision.bplres = GET_RES (bplcon0);

    ddfstate = DIW_waiting_stop;
    compute_toscr_delay (last_fetch_hpos);

    /* If someone already wrote BPL1DAT, clear the area between that point and
       the real fetch start.  */
    if (!nodraw ()) {
	if (thisline_decision.plfleft != -1) {
	    out_nbits = (plfstrt - thisline_decision.plfleft) << (1 + toscr_res);
	    out_offs = out_nbits >> 5;
	    out_nbits &= 31;
	}
	update_toscr_planes ();
    }
}

/* this may turn on datafetch if program turns dma on during the ddf */
static void maybe_start_bpl_dma (unsigned int hpos)
{
    /* OCS: BPL DMA never restarts if DMA is turned on during DDF
     * ECS/AGA: BPL DMA restarts but only if DMA was turned off
       outside of DDF or during current line, otherwise display
       processing jumps immediately to "DDFSTOP passed"-condition */
    if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	return;
    if (fetch_state != fetch_not_started)
	return;
    if (diwstate != DIW_waiting_stop)
	return;
    if (hpos <= plfstrt)
	return;
    if (hpos > plfstop - fetchunit)
	return;
    if (ddfstate != DIW_waiting_start)
	passed_plfstop = 1;
    start_bpl_dma (hpos, hpos);
}

/* This function is responsible for turning on datafetch if necessary. */
STATIC_INLINE void decide_line (unsigned int hpos)
{
    /* Take care of the vertical DIW.  */
    if (vpos == plffirstline)
	diwstate = DIW_waiting_stop;
    if (vpos == plflastline)
	diwstate = DIW_waiting_start;

    if ((int)hpos <= last_decide_line_hpos)
	return;
    if (fetch_state != fetch_not_started)
	return;

    if (dmaen (DMA_BITPLANE) && diwstate == DIW_waiting_stop) {
	int ok = 0;
	/* Test if we passed the start of the DDF window.  */
	if (last_decide_line_hpos < (int)plfstrt && hpos >= plfstrt) {
	    ok = 1;
	    /* hack warning.. Writing to DDFSTRT when DMA should start must be ignored
	     * (correct fix would be emulate this delay for every custom register, but why bother..) */
	    if (hpos - 2 == ddfstrt_old_hpos && ddfstrt_old_vpos == vpos)
		ok = 0;
	    if (ok) {
		start_bpl_dma (hpos, plfstrt);
		estimate_last_fetch_cycle (plfstrt);
		last_decide_line_hpos = hpos;
#ifndef	CUSTOM_SIMPLE
		do_sprites (plfstrt);
#endif
		return;
	    }
	}
    }

#ifndef	CUSTOM_SIMPLE
    if (last_sprite_decide_line_hpos < SPR0_HPOS + 4 * MAX_SPRITES)
	do_sprites (hpos);
#endif
    last_sprite_decide_line_hpos = hpos;

    last_decide_line_hpos = hpos;
}

/* Called when a color is about to be changed (write to a color register),
 * but the new color has not been entered into the table yet. */
static void record_color_change (unsigned int hpos, unsigned int regno, unsigned long value)
{
    if (regno < 0x1000 && nodraw ())
	return;
    /* Early positions don't appear on-screen. */
    if (vpos < minfirstline)
	return;

    decide_diw (hpos);
    decide_line (hpos);

    if (thisline_decision.ctable == -1)
	remember_ctable ();

#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
    if (next_color_change + 1 >= max_color_change) {
	++delta_color_change;
	return;
    }
#endif

    curr_color_changes[next_color_change].linepos = hpos;
    curr_color_changes[next_color_change].regno = regno;
    curr_color_changes[next_color_change++].value = value;
    if (hpos < HBLANK_OFFSET) {
	curr_color_changes[next_color_change].linepos = HBLANK_OFFSET;
	curr_color_changes[next_color_change].regno = regno;
	curr_color_changes[next_color_change++].value = value;
    }
    curr_color_changes[next_color_change].regno = -1;
}

static void record_register_change (unsigned int hpos, unsigned int regno, unsigned long value)
{
    if (regno == 0x100) {
	if (value & 0x800)
	    thisline_decision.ham_seen = 1;
	if (hpos < HARD_DDF_START || hpos < plfstrt + 0x20) {
	    thisline_decision.bplcon0 = value;
	    thisline_decision.bplres = GET_RES (value);
	}
    }
    record_color_change (hpos, regno + 0x1000, value);
}

typedef int sprbuf_res_t, cclockres_t, hwres_t,	bplres_t;

/* handle very rarely needed playfield collision (CLXDAT bit 0) */
static void do_playfield_collisions (void)
{
    int bplres = GET_RES (bplcon0);
    hwres_t ddf_left = thisline_decision.plfleft * 2 << bplres;
    hwres_t hw_diwlast = coord_window_to_diw_x (thisline_decision.diwlastword);
    hwres_t hw_diwfirst = coord_window_to_diw_x (thisline_decision.diwfirstword);
    int i, collided, minpos, maxpos;
#ifdef AGA
    int planes = (currprefs.chipset_mask & CSMASK_AGA) ? 8 : 6;
#else
    int planes = 6;
#endif

    if (clxcon_bpl_enable == 0) {
	clxdat |= 1;
	return;
    }
    if (clxdat & 1)
	return;

    collided = 0;
    minpos = thisline_decision.plfleft * 2;
    if (minpos < hw_diwfirst)
	minpos = hw_diwfirst;
    maxpos = thisline_decision.plfright * 2;
    if (maxpos > hw_diwlast)
	maxpos = hw_diwlast;
    for (i = minpos; i < maxpos && !collided; i+= 32) {
	int offs = ((i << bplres) - ddf_left) >> 3;
	int j;
	uae_u32 total = 0xffffffff;
	for (j = 0; j < planes; j++) {
	    int ena = (clxcon_bpl_enable >> j) & 1;
	    int match = (clxcon_bpl_match >> j) & 1;
	    uae_u32 t = 0xffffffff;
	    if (ena) {
		if (j < thisline_decision.nr_planes) {
		    t = *(uae_u32 *)(line_data[next_lineno] + offs + 2 * j * MAX_WORDS_PER_LINE);
		    t ^= (match & 1) - 1;
		} else {
		    t = (match & 1) - 1;
		}
	    }
	    total &= t;
	}
	if (total) {
	    collided = 1;
#if 0
	    {
		int k;
		for (k = 0; k < 1; k++) {
		    uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + offs + 2 * k * MAX_WORDS_PER_LINE);
		    *ldata ^= 0x5555555555;
		}
	    }
#endif

	}
    }
    if (collided)
	clxdat |= 1;
}

/* Sprite-to-sprite collisions are taken care of in record_sprite.  This one does
   playfield/sprite collisions. */
static void do_sprite_collisions (void)
{
    int nr_sprites = curr_drawinfo[next_lineno].nr_sprites;
    int first = curr_drawinfo[next_lineno].first_sprite_entry;
    int i;
    unsigned int collision_mask = clxmask[clxcon >> 12];
    int bplres = GET_RES (bplcon0);
    hwres_t ddf_left = thisline_decision.plfleft * 2 << bplres;
    hwres_t hw_diwlast = coord_window_to_diw_x (thisline_decision.diwlastword);
    hwres_t hw_diwfirst = coord_window_to_diw_x (thisline_decision.diwfirstword);

    if (clxcon_bpl_enable == 0) {
	clxdat |= 0x1FE;
	return;
    }

    for (i = 0; i < nr_sprites; i++) {
	struct sprite_entry *e = curr_sprite_entries + first + i;
	sprbuf_res_t j;
	sprbuf_res_t minpos = e->pos;
	sprbuf_res_t maxpos = e->max;
	hwres_t minp1 = minpos >> sprite_buffer_res;
	hwres_t maxp1 = maxpos >> sprite_buffer_res;

	if (maxp1 > hw_diwlast)
	    maxpos = hw_diwlast << sprite_buffer_res;
	if (maxp1 > thisline_decision.plfright * 2)
	    maxpos = thisline_decision.plfright * 2 << sprite_buffer_res;
	if (minp1 < hw_diwfirst)
	    minpos = hw_diwfirst << sprite_buffer_res;
	if (minp1 < thisline_decision.plfleft * 2)
	    minpos = thisline_decision.plfleft * 2 << sprite_buffer_res;

	for (j = minpos; j < maxpos; j++) {
	    int sprpix = spixels[e->first_pixel + j - e->pos] & collision_mask;
	    int k, offs, match = 1;

	    if (sprpix == 0)
		continue;

	    offs = ((j << bplres) >> sprite_buffer_res) - ddf_left;
	    sprpix = sprite_ab_merge[sprpix & 255] | (sprite_ab_merge[sprpix >> 8] << 2);
	    sprpix <<= 1;

	    /* Loop over number of playfields.  */
	    for (k = 1; k >= 0; k--) {
		unsigned int l;
#ifdef AGA
		unsigned int planes = (currprefs.chipset_mask & CSMASK_AGA) ? 8 : 6;
#else
		unsigned int planes = 6;
#endif
		if (bplcon0 & 0x400)
		    match = 1;
		for (l = k; match && l < planes; l += 2) {
		    unsigned int t = 0;
		    if (l < thisline_decision.nr_planes) {
			uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + 2 * l * MAX_WORDS_PER_LINE);
			uae_u32 word = ldata[offs >> 5];
			t = (word >> (31 - (offs & 31))) & 1;
#if 0 /* debug: draw collision mask */
			if (1) {
			    int m;
			    for (m = 0; m < 5; m++) {
				ldata = (uae_u32 *)(line_data[next_lineno] + 2 * m * MAX_WORDS_PER_LINE);
				ldata[(offs >> 5) + 1] |= 15 << (31 - (offs & 31));
			    }
			}
#endif
		    }
		    if (clxcon_bpl_enable & (1 << l)) {
			if (t != ((clxcon_bpl_match >> l) & 1))
			    match = 0;
		    }
		}
		if (match) {
#if 0 /* debug: mark lines where collisions are detected */
		    if (0) {
			int l;
			for (l = 0; l < 5; l++) {
			    uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + 2 * l * MAX_WORDS_PER_LINE);
			    ldata[(offs >> 5) + 1] |= 15 << (31 - (offs & 31));
			}
		    }
#endif
		    clxdat |= sprpix << (k * 4);
		}
	    }
	}
    }
#if 0
    {
	static int olx;
	if (clxdat != olx)
	    write_log ("%d: %04.4X\n", vpos, clxdat);
	olx = clxdat;
    }
#endif
}

STATIC_INLINE void expand_sprres (void)
{
    switch ((bplcon3 >> 6) & 3) {
    case 0: /* ECS defaults (LORES,HIRES=140ns,SHRES=70ns) */
	if ((currprefs.chipset_mask & CSMASK_ECS_DENISE) && GET_RES (bplcon0) == RES_SUPERHIRES)
	    sprres = RES_HIRES;
	else
	    sprres = RES_LORES;
	break;
    case 1:
	sprres = RES_LORES;
	break;
    case 2:
	sprres = RES_HIRES;
	break;
    case 3:
	sprres = RES_SUPERHIRES;
	break;
    }
}

STATIC_INLINE void record_sprite_1 (uae_u16 *buf, uae_u32 datab, int num, int dbl,
				    unsigned int mask, int do_collisions, uae_u32 collision_mask)
{
    int j = 0;
    while (datab) {
	unsigned int tmp = *buf;
	unsigned int col = (datab & 3) << (2 * num);
	tmp |= col;
	if ((j & mask) == 0)
	    *buf++ = tmp;
	if (dbl)
	    *buf++ = tmp;
	j++;
	datab >>= 2;
	if (do_collisions) {
	    tmp &= collision_mask;
	    if (tmp) {
		unsigned int shrunk_tmp = sprite_ab_merge[tmp & 255] | (sprite_ab_merge[tmp >> 8] << 2);
		clxdat |= sprclx[shrunk_tmp];
	    }
	}
    }
}

/* DATAB contains the sprite data; 16 pixels in two-bit packets.  Bits 0/1
   determine the color of the leftmost pixel, bits 2/3 the color of the next
   etc.
   This function assumes that for all sprites in a given line, SPRXP either
   stays equal or increases between successive calls.

   The data is recorded either in lores pixels (if ECS), or in hires pixels
   (if AGA).  No support for SHRES sprites.  */

static void record_sprite (int line, int num, int sprxp, uae_u16 *data, uae_u16 *datb, unsigned int ctl, unsigned int ctlx)
{
    struct sprite_entry *e = curr_sprite_entries + next_sprite_entry;
    unsigned int i;
    int word_offs;
    uae_u16 *buf;
    uae_u32 collision_mask;
    unsigned int width = sprite_width;
    int dbl = 0, half = 0;
    unsigned int mask = 0;

#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
    if (e >= &curr_sprite_entries[max_sprite_entry-1]) {
	/* We need to re-allocate the sprite_entries table */
	delta_sprite_entry++;
	return;
    }
#endif

    if (sprres != RES_LORES)
	thisline_decision.any_hires_sprites = 1;

#ifdef AGA
    if (currprefs.chipset_mask & CSMASK_AGA) {
	width = (width << 1) >> sprres;
	dbl = sprite_buffer_res - sprres;
	if (dbl < 0) {
	    half = -dbl;
	    dbl = 0;
	}
	mask = sprres == RES_SUPERHIRES ? 1 : 0;
    }
#endif

    /* Try to coalesce entries if they aren't too far apart.  */
    if (! next_sprite_forced && e[-1].max + 16 >= sprxp) {
	e--;
    } else {
	next_sprite_entry++;
	e->pos = sprxp;
	e->has_attached = 0;
    }

    if (sprxp < e->pos)
	uae_abort ("sprxp < e->pos");

    e->max = sprxp + width;
    e[1].first_pixel = e->first_pixel + ((e->max - e->pos + 3) & ~3);
    next_sprite_forced = 0;

    collision_mask = clxmask[clxcon >> 12];
    word_offs = e->first_pixel + sprxp - e->pos;

    for (i = 0; i < sprite_width; i += 16) {
	unsigned int da = *data;
	unsigned int db = *datb;
	uae_u32 datab = ((sprtaba[da & 0xFF] << 16) | sprtaba[da >> 8]
			 | (sprtabb[db & 0xFF] << 16) | sprtabb[db >> 8]);

	buf = spixels + word_offs + ((i << dbl) >> half);
	if (currprefs.collision_level > 0 && collision_mask)
	    record_sprite_1 (buf, datab, num, dbl, mask, 1, collision_mask);
	else
	    record_sprite_1 (buf, datab, num, dbl, mask, 0, collision_mask);
	data++;
	datb++;
    }

    /* We have 8 bits per pixel in spixstate, two for every sprite pair.  The
       low order bit records whether the attach bit was set for this pair.  */
    if ((sprctl[num] & 0x80) || (!(currprefs.chipset_mask & CSMASK_AGA) && (sprctl[num ^ 1] & 0x80))) {
	uae_u32 state = 0x01010101 << (num & ~1);
	uae_u32 *stbuf = spixstate.words + (word_offs >> 2);
	uae_u8 *stb1 = spixstate.bytes + word_offs;
	for (i = 0; i < width; i += 8) {
	    stb1[0] |= state;
	    stb1[1] |= state;
	    stb1[2] |= state;
	    stb1[3] |= state;
	    stb1[4] |= state;
	    stb1[5] |= state;
	    stb1[6] |= state;
	    stb1[7] |= state;
	    stb1 += 8;
	}
	e->has_attached = 1;
    }
}

static void decide_sprites (unsigned int hpos)
{
    unsigned int nrs[MAX_SPRITES], posns[MAX_SPRITES];
    unsigned int count, i;
     /* apparently writes to custom registers happen in the 3/4th of cycle
      * and sprite xpos comparator sees it immediately */
    unsigned int point = hpos * 2 - 3;
    unsigned int width = sprite_width;
    unsigned int window_width = (width << lores_shift) >> sprres;

    if (nodraw () || hpos < 0x14 || nr_armed == 0 || point == last_sprite_point)
	return;

    decide_diw (hpos);
    decide_line (hpos);

    count = 0;
    for (i = 0; i < MAX_SPRITES; i++) {
	unsigned int sprxp = spr[i].xpos;
	unsigned int hw_xp = (sprxp >> sprite_buffer_res);
	int window_xp = coord_hw_to_window_x (hw_xp) + (DIW_DDF_OFFSET << lores_shift);
	unsigned int j;
	unsigned int bestp;

#if DEBUGGER
	if (!((debug_sprite_mask) & (1 << i)))
	    continue;
#endif

	if (! spr[i].armed || hw_xp <= last_sprite_point || hw_xp > point)
	    continue;
	if ( !(bplcon3 & 2) && /* sprites outside playfields enabled? */
	    ((thisline_decision.diwfirstword >= 0 && window_xp + (int)window_width < thisline_decision.diwfirstword)
	    || (thisline_decision.diwlastword >= 0 && window_xp > thisline_decision.diwlastword)))
	    continue;

	/* Sort the sprites in order of ascending X position before recording them.  */
	for (bestp = 0; bestp < count; bestp++) {
	    if (posns[bestp] > sprxp)
		break;
	    if (posns[bestp] == sprxp && nrs[bestp] < i)
		break;
	}
	for (j = count; j > bestp; j--) {
	    posns[j] = posns[j-1];
	    nrs[j] = nrs[j-1];
	}
	posns[j] = sprxp;
	nrs[j] = i;
	count++;
    }
    for (i = 0; i < count; i++) {
	unsigned int nr = nrs[i];
	record_sprite (next_lineno, nr, spr[nr].xpos, sprdata[nr], sprdatb[nr], sprctl[nr], sprctl[nr ^ 1]);
    }
    last_sprite_point = point;
}

STATIC_INLINE int sprites_differ (struct draw_info *dip, struct draw_info *dip_old)
{
    struct sprite_entry *this_first = curr_sprite_entries + dip->first_sprite_entry;
    struct sprite_entry *this_last = curr_sprite_entries + dip->last_sprite_entry;
    struct sprite_entry *prev_first = prev_sprite_entries + dip_old->first_sprite_entry;
    int npixels;
    int i;

    if (dip->nr_sprites != dip_old->nr_sprites)
	return 1;

    if (dip->nr_sprites == 0)
	return 0;

    for (i = 0; i < dip->nr_sprites; i++)
	if (this_first[i].pos != prev_first[i].pos
	    || this_first[i].max != prev_first[i].max
	    || this_first[i].has_attached != prev_first[i].has_attached)
	    return 1;

    npixels = this_last->first_pixel + (this_last->max - this_last->pos) - this_first->first_pixel;
    if (memcmp (spixels + this_first->first_pixel, spixels + prev_first->first_pixel,
		npixels * sizeof (uae_u16)) != 0)
	return 1;
    if (memcmp (spixstate.bytes + this_first->first_pixel, spixstate.bytes + prev_first->first_pixel, npixels) != 0)
	return 1;
    return 0;
}

STATIC_INLINE int color_changes_differ (struct draw_info *dip, struct draw_info *dip_old)
{
    if (dip->nr_color_changes != dip_old->nr_color_changes)
	return 1;

    if (dip->nr_color_changes == 0)
	return 0;
    if (memcmp (curr_color_changes + dip->first_color_change,
		prev_color_changes + dip_old->first_color_change,
		dip->nr_color_changes * sizeof *curr_color_changes) != 0)
	return 1;
    return 0;
}

/* End of a horizontal scan line. Finish off all decisions that were not
 * made yet. */
static void finish_decisions (void)
{
    struct draw_info *dip;
    struct draw_info *dip_old;
    struct decision *dp;
    int changed;
    int hpos = current_hpos ();

    if (nodraw ())
	return;

    decide_diw (hpos);
    decide_line (hpos);
    decide_fetch (hpos);

    if (thisline_decision.plfleft != -1 && thisline_decision.plflinelen == -1) {
	if (fetch_state != fetch_not_started) {
	    write_log("fetch_state=%d plfleft=%d,len=%d,vpos=%d,hpos=%d\n",
		fetch_state, thisline_decision.plfleft, thisline_decision.plflinelen,
		vpos, hpos);
	    uae_abort ("fetch_state != fetch_not_started");
	}
	thisline_decision.plfright = thisline_decision.plfleft;
	thisline_decision.plflinelen = 0;
	thisline_decision.bplres = RES_LORES;
    }

    /* Large DIWSTOP values can cause the stop position never to be
     * reached, so the state machine always stays in the same state and
     * there's a more-or-less full-screen DIW. */
    if (hdiwstate == DIW_waiting_stop || thisline_decision.diwlastword > max_diwlastword)
	thisline_decision.diwlastword = max_diwlastword;

    if (thisline_decision.diwfirstword != line_decisions[next_lineno].diwfirstword)
	MARK_LINE_CHANGED;
    if (thisline_decision.diwlastword != line_decisions[next_lineno].diwlastword)
	MARK_LINE_CHANGED;

    dip = curr_drawinfo + next_lineno;
    dip_old = prev_drawinfo + next_lineno;
    dp = line_decisions + next_lineno;
    dp->valid = 0;
    changed = thisline_changed + interlace_started;

    if (thisline_decision.plfleft != -1)
	record_diw_line (diwfirstword, diwlastword);

    if (thisline_decision.plfleft != -1 || (bplcon3 & 2))
	decide_sprites (hpos);

    dip->last_sprite_entry = next_sprite_entry;
    dip->last_color_change = next_color_change;

    if (thisline_decision.ctable == -1) {
	if (thisline_decision.plfleft == -1)
	    remember_ctable_for_border ();
	else
	    remember_ctable ();
    }

    dip->nr_color_changes = next_color_change - dip->first_color_change;
    dip->nr_sprites = next_sprite_entry - dip->first_sprite_entry;

    if (thisline_decision.plfleft != line_decisions[next_lineno].plfleft)
	changed = 1;
    if (! changed && color_changes_differ (dip, dip_old))
	changed = 1;
    if (!changed && thisline_decision.plfleft != -1 && sprites_differ (dip, dip_old))
	changed = 1;

    if (changed) {
	thisline_changed = 1;
	*dp = thisline_decision;
	dp->valid = 1;
    } else
	/* The only one that may differ: */
	dp->ctable = thisline_decision.ctable;
}

/* Set the state of all decisions to "undecided" for a new scanline. */
static void reset_decisions (void)
{
    if (nodraw ())
	return;

    thisline_decision.bplres = GET_RES (bplcon0);
    thisline_decision.any_hires_sprites = 0;
    thisline_decision.nr_planes = 0;

    thisline_decision.plfleft = -1;
    thisline_decision.plflinelen = -1;
    thisline_decision.ham_seen = !! (bplcon0 & 0x800);
    thisline_decision.ham_at_start = !! (bplcon0 & 0x800);

    /* decided_res shouldn't be touched before it's initialized by decide_line(). */
    thisline_decision.diwfirstword = -1;
    thisline_decision.diwlastword = -1;
    if (hdiwstate == DIW_waiting_stop) {
	thisline_decision.diwfirstword = 0;
	if (thisline_decision.diwfirstword != line_decisions[next_lineno].diwfirstword)
	    MARK_LINE_CHANGED;
    }
    thisline_decision.ctable = -1;

    thisline_changed = 0;
    curr_drawinfo[next_lineno].first_color_change = next_color_change;
    curr_drawinfo[next_lineno].first_sprite_entry = next_sprite_entry;
    next_sprite_forced = 1;

    last_sprite_point = 0;
    fetch_state = fetch_not_started;
    passed_plfstop = 0;

    memset (todisplay, 0, sizeof todisplay);
    memset (fetched, 0, sizeof fetched);
#ifdef AGA
    memset (fetched_aga0, 0, sizeof fetched_aga0);
    memset (fetched_aga1, 0, sizeof fetched_aga1);
#endif
    memset (outword, 0, sizeof outword);

    last_decide_line_hpos = -1;
    last_sprite_decide_line_hpos = -1;
    last_diw_pix_hpos = -1;
    last_ddf_pix_hpos = -1;
    last_sprite_hpos = 0;
    last_fetch_hpos = -1;

    /* These are for comparison. */
    thisline_decision.bplcon0 = bplcon0;
    thisline_decision.bplcon2 = bplcon2;
#ifdef AGA
    thisline_decision.bplcon3 = bplcon3;
    thisline_decision.bplcon4 = bplcon4;
#endif
}

int vsynctime_orig;
int turbo_emulation;

void compute_vsynctime (void)
{
    fake_vblank_hz = 0;
    if (currprefs.chipset_refreshrate) {
	vblank_hz = currprefs.chipset_refreshrate;
	if (is_vsync ()) {
	    vblank_skip = 1;
	    if (!fake_vblank_hz && vblank_hz > 85) {
		vblank_hz /= 2;
		vblank_skip = -1;
	    }
	}
    }
    if (!fake_vblank_hz)
	fake_vblank_hz = vblank_hz;
    if (turbo_emulation)
	vsynctime = vsynctime_orig = 1;
    else
	vsynctime = vsynctime_orig = syncbase / fake_vblank_hz;
#ifdef OPENGL
    OGL_refresh ();
#endif
#ifdef D3D
    D3D_refresh ();
#endif
    if (currprefs.produce_sound > 1)
	update_sound (fake_vblank_hz);
}


static void dumpsync (void)
{
    static int cnt = 10;
    if (cnt < 0)
	return;
    cnt--;
    write_log ("BEAMCON0 = %04.4X VTOTAL=%04.4X HTOTAL=%04.4X\n", new_beamcon0, vtotal, htotal);
    write_log ("HSSTOP=%04.4X HBSTRT=%04.4X HBSTOP=%04.4X\n", hsstop, hbstrt, hbstop);
    write_log ("VSSTOP=%04.4X VBSTRT=%04.4X VBSTOP=%04.4X\n", vsstop, vbstrt, vbstop);
    write_log ("HSSTRT=%04.4X VSSTRT=%04.4X HCENTER=%04.4X\n", hsstrt, vsstrt, hcenter);
}

/* set PAL or NTSC timing variables */
void init_hz (void)
{
    int isntsc;

    if ((currprefs.chipset_refreshrate == 50 && !currprefs.ntscmode) ||
	(currprefs.chipset_refreshrate == 60 && currprefs.ntscmode)) {
	currprefs.chipset_refreshrate = 0;
	changed_prefs.chipset_refreshrate = 0;
    }
    if (is_vsync ()) {
	currprefs.chipset_refreshrate = abs (currprefs.gfx_refreshrate);
	changed_prefs.chipset_refreshrate = abs (currprefs.gfx_refreshrate);
    }

    beamcon0 = new_beamcon0;
    isntsc = beamcon0 & 0x20 ? 0 : 1;
    if (hack_vpos > 0) {
	if ((int)maxvpos == hack_vpos)
	    return;
	maxvpos = hack_vpos;
	vblank_hz = 15600 / hack_vpos;
	hack_vpos = -1;
    } else if (hack_vpos < 0) {
	hack_vpos = 0;
    }
    if (hack_vpos == 0) {
	if (!isntsc) {
	    maxvpos = MAXVPOS_PAL;
	    maxhpos = MAXHPOS_PAL;
	    minfirstline = VBLANK_ENDLINE_PAL;
	    vblank_hz = VBLANK_HZ_PAL;
	    sprite_vblank_endline = VBLANK_SPRITE_PAL;
	} else {
	    maxvpos = MAXVPOS_NTSC;
	    maxhpos = MAXHPOS_NTSC;
	    minfirstline = VBLANK_ENDLINE_NTSC;
	    vblank_hz = VBLANK_HZ_NTSC;
	    sprite_vblank_endline = VBLANK_SPRITE_NTSC;
	}
    }
    if (beamcon0 & 0x80) {
	if (vtotal >= MAXVPOS)
	    vtotal = MAXVPOS - 1;
	maxvpos = vtotal + 1;
	if (htotal >= MAXHPOS)
	    htotal = MAXHPOS - 1;
	maxhpos = htotal + 1;
	vblank_hz = 227 * 312 * 50 / (maxvpos * maxhpos);
	minfirstline = vsstop;
	if (minfirstline < 2)
	    minfirstline = 2;
	sprite_vblank_endline = minfirstline - 2;
	dumpsync ();
    }
    /* limit to sane values */
    if (vblank_hz < 10)
	vblank_hz = 10;
    if (vblank_hz > 300)
	vblank_hz = 300;
    eventtab[ev_hsync].oldcycles = get_cycles ();
    eventtab[ev_hsync].evtime = get_cycles() + HSYNCTIME;
    events_schedule ();
    compute_vsynctime ();
#ifdef OPENGL
    OGL_refresh ();
#endif
#ifdef PICASSO96
    init_hz_p96 ();
#endif
    write_log ("%s mode, %dHz (h=%d v=%d)\n",
	isntsc ? "NTSC" : "PAL", vblank_hz, maxhpos, maxvpos);
}

static void calcdiw (void)
{
    unsigned int hstrt = diwstrt & 0xFF;
    unsigned int hstop = diwstop & 0xFF;
    unsigned int vstrt = diwstrt >> 8;
    unsigned int vstop = diwstop >> 8;

    if (diwhigh_written) {
	hstrt |= ((diwhigh >> 5) & 1) << 8;
	hstop |= ((diwhigh >> 13) & 1) << 8;
	vstrt |= (diwhigh & 7) << 8;
	vstop |= ((diwhigh >> 8) & 7) << 8;
    } else {
	hstop += 0x100;
	if ((vstop & 0x80) == 0)
	    vstop |= 0x100;
    }

    diwfirstword = coord_diw_to_window_x (hstrt);
    diwlastword = coord_diw_to_window_x (hstop);
    if (diwfirstword >= diwlastword) {
	diwfirstword = 0;
	diwlastword = max_diwlastword;
    }
    if (diwfirstword < 0)
	diwfirstword = 0;

    plffirstline = vstrt;
    plflastline = vstop;

#if 0
    /* This happens far too often. */
    if (plffirstline < minfirstline_bpl) {
	write_log ("Warning: Playfield begins before line %d (%d)!\n", minfirstline_bpl, plffirstline);
    }
#endif

#if 0 /* this comparison is not needed but previous is.. */
    if (plflastline > 313) {
	/* Turrican does this */
	write_log ("Warning: Playfield out of range!\n");
	plflastline = 313;
    }
#endif

    plfstrt = ddfstrt;
    plfstop = ddfstop;
    /* probably not the correct place.. */
    if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
	if (ddfstop > maxhpos)
	    plfstrt = 0;
	if (plfstrt < HARD_DDF_START)
	    plfstrt = HARD_DDF_START;
    }
}

/* display mode changed (lores, doubling etc..), recalculate everything */
void init_custom (void)
{
    create_cycle_diagram_table ();
    reset_drawing ();
    init_hz ();
    calcdiw ();
}

static int timehack_alive = 0;

static uae_u32 REGPARAM2 timehack_helper (TrapContext *context)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    if (m68k_dreg (&context->regs, 0) == 0)
	return timehack_alive;

    timehack_alive = 10;

    gettimeofday (&tv, NULL);
    put_long (m68k_areg (&context->regs, 0), tv.tv_sec - (((365 * 8 + 2) * 24) * 60 * 60));
    put_long (m68k_areg (&context->regs, 0) + 4, tv.tv_usec);
    return 0;
#else
    return 2;
#endif
}

 /*
  * register functions
  */
STATIC_INLINE uae_u16 DENISEID (void)
{
#ifdef AGA
    if (currprefs.chipset_mask & CSMASK_AGA)
	return 0xF8;
#endif
    if (currprefs.chipset_mask & CSMASK_ECS_DENISE)
	return 0xFC;
    return 0xffff;
}
STATIC_INLINE uae_u16 DMACONR (void)
{
    uae_u16 v;
    decide_blitter (current_hpos ());
    v = dmacon | (bltstate == BLT_done ? 0 : 0x4000)
	    | (blt_info.blitzero ? 0x2000 : 0);
    return v;
}
STATIC_INLINE uae_u16 INTENAR (void)
{
    return intena;
}
uae_u16 INTREQR (void)
{
    return intreq;
}
STATIC_INLINE uae_u16 ADKCONR (void)
{
    return adkcon;
}

STATIC_INLINE int GETVPOS(void)
{
    return vpos_lpen > 0 ? vpos_lpen : (((bplcon0 & 2) && !currprefs.genlock) ? vpos_previous : (int)vpos);
}
STATIC_INLINE int GETHPOS(void)
{
    return vpos_lpen > 0 ? hpos_lpen : (((bplcon0 & 2) && !currprefs.genlock) ? hpos_previous : (int)current_hpos ());
}

STATIC_INLINE uae_u16 VPOSR (void)
{
    unsigned int csbit = currprefs.ntscmode ? 0x1000 : 0;
    unsigned int vp = (GETVPOS() >> 8) & 7;
#ifdef AGA
    csbit |= (currprefs.chipset_mask & CSMASK_AGA) ? 0x2300 : 0;
#endif
    csbit |= (currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 0x2000 : 0;
    if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	vp &= 1;
    vp = vp | lof | csbit;
#if 0
    write_log ("vposr %x at %x\n", vp, m68k_getpc (&regs));
#endif
    if (currprefs.cpu_level >= 2)
	hsyncdelay ();
    return vp;
}
static void VPOSW (uae_u16 v)
{
#if 0
    write_log ("vposw %x at %x\n", v, m68k_getpc (&regs));
#endif
    if (lof != (v & 0x8000))
	lof_changed = 1;
    lof = v & 0x8000;
    if ( (v & 1) && vpos > 0)
	hack_vpos = vpos;
}

STATIC_INLINE uae_u16 VHPOSR (void)
{
    uae_u16 vp = GETVPOS();
    uae_u16 hp = GETHPOS();
    vp <<= 8;
    vp |= hp;
    if (currprefs.cpu_level >= 2)
	hsyncdelay ();
    return vp;
}

STATIC_INLINE void COP1LCH (uae_u16 v) { cop1lc = (cop1lc & 0xffff) | ((uae_u32)v << 16); }
STATIC_INLINE void COP1LCL (uae_u16 v) { cop1lc = (cop1lc & ~0xffff) | (v & 0xfffe); }
STATIC_INLINE void COP2LCH (uae_u16 v) { cop2lc = (cop2lc & 0xffff) | ((uae_u32)v << 16); }
STATIC_INLINE void COP2LCL (uae_u16 v) { cop2lc = (cop2lc & ~0xffff) | (v & 0xfffe); }

static void COPJMP (int num)
{
    int was_active = eventtab[ev_copper].active;
    int oldstrobe = cop_state.strobe;

    eventtab[ev_copper].active = 0;
    if (was_active)
	events_schedule ();

    unset_special (&regs, SPCFLAG_COPPER);
    cop_state.ignore_next = 0;
    if (!oldstrobe)
	cop_state.state_prev = cop_state.state;
    cop_state.state = COP_read1;
    cop_state.vpos = vpos;
    cop_state.hpos = current_hpos () & ~1;
    copper_enabled_thisline = 0;
    cop_state.strobe = num;

    if (dmaen (DMA_COPPER)) {
	copper_enabled_thisline = 1;
	set_special (&regs, SPCFLAG_COPPER);
    } else if (oldstrobe > 0 && oldstrobe != num && cop_state.state_prev == COP_wait) {
	/* dma disabled, copper idle and accessing both COPxJMPs -> copper stops! */
	cop_state.state = COP_stop;
    }
}

STATIC_INLINE void COPCON (uae_u16 a)
{
    copcon = a;
}

static void compute_spcflag_copper (void);
static void DMACON (unsigned int hpos, uae_u16 v)
{
    int oldcop, newcop;
    uae_u16 changed;

    uae_u16 oldcon = dmacon;

    decide_line (hpos);
    decide_fetch (hpos);
    decide_blitter (hpos);

    setclr (&dmacon, v);
    dmacon &= 0x1FFF;

    changed = dmacon ^ oldcon;

    oldcop = (oldcon & DMA_COPPER) && (oldcon & DMA_MASTER);
    newcop = (dmacon & DMA_COPPER) && (dmacon & DMA_MASTER);

    if (oldcop != newcop) {
	eventtab[ev_copper].active = 0;
	if (newcop && !oldcop) {
	    compute_spcflag_copper ();
	} else if (!newcop) {
	    copper_enabled_thisline = 0;
	    unset_special (&regs, SPCFLAG_COPPER);
	}
    }
    if ((dmacon & DMA_BLITPRI) > (oldcon & DMA_BLITPRI) && bltstate != BLT_done) {
	static int count = 0;
	if (!count) {
	    count = 1;
	    write_log ("warning: program is doing blitpri hacks.\n");
	}
	set_special (&regs, SPCFLAG_BLTNASTY);
	decide_blitter (hpos);
    }
    if (dmaen (DMA_BLITTER) && bltstate == BLT_init)
	bltstate = BLT_work;
    if ((dmacon & (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER)) != (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER)) {
	unset_special (&regs, SPCFLAG_BLTNASTY);
	decide_blitter (hpos);
    }
    if (changed & (DMA_MASTER | 0x0f))
	audio_hsync (0);
    if (changed & (DMA_MASTER | DMA_BITPLANE)) {
	ddf_change = vpos;
	if (dmaen (DMA_BITPLANE))
	    maybe_start_bpl_dma (hpos);
    }

    events_schedule();
}

#ifdef CPUEMU_6

static int irq_pending[15];       /* If true, an IRQ is pending arrival at the CPU. */
static unsigned long irq_due[15]; /* Cycle time that IRQ will arrive at CPU */

/*
 * Handle interrupt delay in cycle-exact mode.
 */
static int intlev_2 (void)
{
    uae_u16 imask = intreq & intena;
    int il = -1;

    if (imask && (intena & 0x4000)) {
	unsigned long cycles = get_cycles ();
	int i;

	for (i = 14; i >= 0; i--) {
	    if (imask & (1 << i)) {
		if (irq_pending[i] && (cycles >= irq_due[i])) {
		    irq_pending[i] = 0;

		    if (i == 13 || i == 14)
			il = -1;
		    else if (i == 11 || i == 12)
			return 5;
		    else if (i >= 7 && i <= 10)
			return 4;
		    else if (i >= 4 && i <= 6)
			return 3;
		    else if (i == 3)
			return 2;
		    else
			return 1;
	        }
	    }
	}
    } else
	unset_special (&regs, SPCFLAG_INT);

    return il;
}
#endif

/*
 * Get interrupt level of IRQ.
 */
int intlev (void)
{
    int il = -1;

#ifdef CPUEMU_6
    if (currprefs.cpu_cycle_exact) {
	il = intlev_2 ();
	if (il >= 0 && il <= (int) regs.intmask)
	    unset_special (&regs, SPCFLAG_INT);
    } else
#endif
    {
	uae_u16 imask = intreq & intena;
	if (imask && (intena & 0x4000)) {
	    if (imask & 0x6000)
		il = 6;
	    if (imask & 0x1800)
		il = 5;
	    if (imask & 0x0780)
		il = 4;
	    if (imask & 0x0070)
		il = 3;
	    if (imask & 0x0008)
		il = 2;
	    if (imask & 0x0007)
		il = 1;
	}
    }

    return il;
}

static void doint (void)
{
    set_special (&regs, SPCFLAG_INT);

#ifdef CPUEMU_6
    if (currprefs.cpu_cycle_exact) {
	int i;
	uae_u16 imask;

	imask = intreq & intena;

	if (imask && (intena & 0x4000)) {
	    /* Set up delay for IRQ to arrive at the CPU. */
	    unsigned long cycle_irq_due = get_cycles () + 4 * CYCLE_UNIT;
	    for (i = 0; i < 15; i++) {
		if ((imask & (1 << i)) && irq_pending[i] == 0) {
		    irq_pending[i] = 1;
		    irq_due[i]     = cycle_irq_due;
		}
	    }
	}
    }
#endif
}

STATIC_INLINE void INTENA (uae_u16 v)
{
    setclr (&intena,v);
#if 0
    if (v & 0x40)
	write_log("INTENA %04.4X (%04.4X) %p\n", intena, v, m68k_getpc (&regs));
#endif
    if (v & 0x8000)
	doint ();
}

void INTREQ_0 (uae_u16 v)
{
    if (v & (0x80|0x100|0x200|0x400))
	audio_update_irq (v);

    setclr (&intreq, v);

#ifdef CPUEMU_6
    if (currprefs.cpu_cycle_exact) {
	if (!(v & 0x8000)) {
	    /* Interrupt request is being cleared - reset
	     * pending status. */
	    int i;
	    for (i = 0; i < 15; i++) {
		if (v & (1 << i))
		    irq_pending[i] = 0;
	    }
	}
    }
#endif

    doint ();
}

void INTREQ (uae_u16 v)
{
    INTREQ_0 (v);
    serial_check_irq ();
    rethink_cias ();
}

static void ADKCON (int hpos, uae_u16 v)
{
    if (currprefs.produce_sound > 0)
	update_audio ();

    setclr (&adkcon,v);
    audio_update_adkmasks ();
    DISK_update (hpos);

    if ((v >> 11) & 1)
	serial_uartbreak ((adkcon >> 11) & 1);
}

static void BEAMCON0 (uae_u16 v)
{
    if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
	if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE))
	    v &= 0x20;
	if (v != new_beamcon0) {
	    new_beamcon0 = v;
	    if (v & ~0x20)
		write_log ("warning: %04.4X written to BEAMCON0\n", v);
	}
    }
}

#ifndef CUSTOM_SIMPLE

static void varsync (void)
{
#ifdef PICASSO96
    if (p96refresh_active)
    {
	extern int p96hack_vpos2;
	static int p96hack_vpos_old;
	if (p96hack_vpos_old == p96hack_vpos2) return;
	vtotal = p96hack_vpos2;
	p96hack_vpos_old = p96hack_vpos2;
	hack_vpos = -1;
	return;
    }
#endif
    if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE))
	return;
    if (!(beamcon0 & 0x80))
	return;
    hack_vpos = -1;
    dumpsync ();
}
#endif

int is_bitplane_dma (unsigned int hpos)
{
    if (fetch_state == fetch_not_started || (int)hpos < thisline_decision.plfleft)
	return 0;
    if ((passed_plfstop == 3 && (int)hpos >= thisline_decision.plfright)
	|| hpos >= estimated_last_fetch_cycle)
	return 0;
    return curr_diagram[(hpos - cycle_diagram_shift) & fetchstart_mask];
}

STATIC_INLINE int is_bitplane_dma_inline (unsigned int hpos)
{
    if (fetch_state == fetch_not_started || (int)hpos < thisline_decision.plfleft)
	return 0;
    if ((passed_plfstop == 3 && (int)hpos >= thisline_decision.plfright)
	|| hpos >= estimated_last_fetch_cycle)
	return 0;
    return curr_diagram[(hpos - cycle_diagram_shift) & fetchstart_mask];
}

static void BPLxPTH (unsigned int hpos, uae_u16 v, int num)
{
    decide_line (hpos);
    decide_fetch (hpos);
    bplpt[num] = (bplpt[num] & 0xffff) | ((uae_u32)v << 16);
    //write_log("%d:%d:BPL%dPTH %08.8X\n", hpos, vpos, num, v);
}
static void BPLxPTL (unsigned int hpos, uae_u16 v, int num)
{
    int delta = 0;
    decide_line (hpos);
    decide_fetch (hpos);
    /* fix for "bitplane dma fetch at the same time while updating BPLxPTL" */
    /* fixes "3v Demo" by Cave and "New Year Demo" by Phoenix */
    if (is_bitplane_dma(hpos - 1) == num + 1 && num > 0)
	delta = 2 << fetchmode;
    bplpt[num] = (bplpt[num] & ~0xffff) | ((v + delta) & 0xfffe);
    //write_log("%d:%d:BPL%dPTL %08.8X\n", hpos, vpos, num, v);
}

static void BPLCON0 (unsigned int hpos, uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
	v &= ~0x00F1;
    else if (! (currprefs.chipset_mask & CSMASK_AGA))
	v &= ~0x00B1;

    if (bplcon0 == v)
	return;

    if ((bplcon0 & 2) && !(v & 2)) {
	vpos_previous = vpos;
	hpos_previous = hpos;
    }

    if ((v & 4) && !interlace_seen)
	interlace_started = 2;

    ddf_change = vpos;
    decide_line (hpos);
    decide_fetch (hpos);
    decide_blitter (hpos);

    bplcon0 = v;
    record_register_change (hpos, 0x100, v);

#ifdef AGA
    if (currprefs.chipset_mask & CSMASK_AGA) {
	decide_sprites (hpos);
	expand_sprres ();
    }
#endif

    expand_fmodes ();
    calcdiw ();
    estimate_last_fetch_cycle (hpos);
}

STATIC_INLINE void BPLCON1 (unsigned int hpos, uae_u16 v)
{
    if (!(currprefs.chipset_mask & CSMASK_AGA))
	v &= 0xff;
    if (bplcon1 == v)
	return;
    ddf_change = vpos;
    decide_line (hpos);
    decide_fetch (hpos);
    bplcon1 = v;
}

STATIC_INLINE void BPLCON2 (unsigned int hpos, uae_u16 v)
{
    if (!(currprefs.chipset_mask & CSMASK_AGA))
	v &= 0x7f;
    if (bplcon2 == v)
	return;
    decide_line (hpos);
    bplcon2 = v;
    record_register_change (hpos, 0x104, v);
}

#ifdef AGA
STATIC_INLINE void BPLCON3 (unsigned int hpos, uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_AGA))
	return;
    if (bplcon3 == v)
	return;
    decide_line (hpos);
    decide_sprites (hpos);
    bplcon3 = v;
    expand_sprres ();
    record_register_change (hpos, 0x106, v);
}

STATIC_INLINE void BPLCON4 (unsigned int hpos, uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_AGA))
	return;
    if (bplcon4 == v)
	return;
    decide_line (hpos);
    bplcon4 = v;
    record_register_change (hpos, 0x10c, v);
}
#endif

static void BPL1MOD (unsigned int hpos, uae_u16 v)
{
    v &= ~1;
    if ((uae_s16)bpl1mod == (uae_s16)v)
	return;
    decide_line (hpos);
    decide_fetch (hpos);
    bpl1mod = v;
}

static void BPL2MOD (unsigned int hpos, uae_u16 v)
{
    v &= ~1;
    if ((uae_s16)bpl2mod == (uae_s16)v)
	return;
    decide_line (hpos);
    decide_fetch (hpos);
    bpl2mod = v;
}

STATIC_INLINE void BPL1DAT (unsigned int hpos, uae_u16 v)
{
    decide_line (hpos);
    bpl1dat = v;

    maybe_first_bpl1dat (hpos);
}

static void DIWSTRT (unsigned int hpos, uae_u16 v)
{
    if (diwstrt == v && ! diwhigh_written)
	return;
    decide_line (hpos);
    diwhigh_written = 0;
    diwstrt = v;
    calcdiw ();
}

static void DIWSTOP (unsigned int hpos, uae_u16 v)
{
    if (diwstop == v && ! diwhigh_written)
	return;
    decide_line (hpos);
    diwhigh_written = 0;
    diwstop = v;
    calcdiw ();
}

static void DIWHIGH (int hpos, uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	return;
    if (diwhigh_written && diwhigh == v)
	return;
    decide_line (hpos);
    diwhigh_written = 1;
    diwhigh = v;
    calcdiw ();
}

static void DDFSTRT (unsigned int hpos, uae_u16 v)
{
    v &= 0xfe;
    if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	v &= 0xfc;
    if (ddfstrt == v && hpos != plfstrt - 2)
	return;
    ddf_change = vpos;
    decide_line (hpos);
    ddfstrt_old_hpos = hpos;
    ddfstrt_old_vpos = vpos;
    ddfstrt = v;
    calcdiw ();
    if (ddfstop > 0xD4 && (ddfstrt & 4) == 4) {
	static int last_warned;
	last_warned = (last_warned + 1) & 4095;
	if (last_warned == 0)
	    write_log ("WARNING! Very strange DDF values (%x %x).\n", ddfstrt, ddfstop);
    }
}

static void DDFSTOP (unsigned int hpos, uae_u16 v)
{
    v &= 0xfe;
    if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	v &= 0xfc;
    if (ddfstop == v)
	return;
    decide_line (hpos);
    decide_fetch (hpos);
    decide_blitter (hpos);
    ddfstop = v;
    calcdiw ();
    if (fetch_state != fetch_not_started)
	estimate_last_fetch_cycle (hpos);
    if (ddfstop > 0xD4 && (ddfstrt & 4) == 4) {
	static int last_warned;
	if (last_warned == 0)
	    write_log ("WARNING! Very strange DDF values (%x).\n", ddfstop);
	last_warned = (last_warned + 1) & 4095;
    }
}

static void FMODE (uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_AGA))
	v = 0;
    ddf_change = vpos;
    fmode = v;
    sprite_width = GET_SPRITEWIDTH (fmode);
    switch (fmode & 3) {
    case 0:
	fetchmode = 0;
	break;
    case 1:
    case 2:
	fetchmode = 1;
	break;
    case 3:
	fetchmode = 2;
	break;
    }
    expand_fmodes ();
    calcdiw ();
}

static void BLTADAT (uae_u16 v)
{
    maybe_blit (current_hpos(), 0);

    blt_info.bltadat = v;
}
/*
 * "Loading data shifts it immediately" says the HRM. Well, that may
 * be true for BLTBDAT, but not for BLTADAT - it appears the A data must be
 * loaded for every word so that AFWM and ALWM can be applied.
 */
static void BLTBDAT (uae_u16 v)
{
    maybe_blit (current_hpos(), 0);

    if (bltcon1 & 2)
	blt_info.bltbhold = v << (bltcon1 >> 12);
    else
	blt_info.bltbhold = v >> (bltcon1 >> 12);
    blt_info.bltbdat = v;
}
static void BLTCDAT (uae_u16 v) { maybe_blit (current_hpos(), 0); blt_info.bltcdat = v; reset_blit (0); }

static void BLTAMOD (uae_u16 v) { maybe_blit (current_hpos(), 1); blt_info.bltamod = (uae_s16)(v & 0xFFFE); reset_blit (0); }
static void BLTBMOD (uae_u16 v) { maybe_blit (current_hpos(), 1); blt_info.bltbmod = (uae_s16)(v & 0xFFFE); reset_blit (0); }
static void BLTCMOD (uae_u16 v) { maybe_blit (current_hpos(), 1); blt_info.bltcmod = (uae_s16)(v & 0xFFFE); reset_blit (0); }
static void BLTDMOD (uae_u16 v) { maybe_blit (current_hpos(), 1); blt_info.bltdmod = (uae_s16)(v & 0xFFFE); reset_blit (0); }

static void BLTCON0 (uae_u16 v) { maybe_blit (current_hpos(), 2); bltcon0 = v; blinea_shift = v >> 12; reset_blit (1); }
/* The next category is "Most useless hardware register".
 * And the winner is... */
static void BLTCON0L (uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	return;
    maybe_blit (current_hpos(), 2); bltcon0 = (bltcon0 & 0xFF00) | (v & 0xFF);
    reset_blit (1);
}
static void BLTCON1 (uae_u16 v) { maybe_blit (current_hpos(), 2); bltcon1 = v; reset_blit (2); }

static void BLTAFWM (uae_u16 v) { maybe_blit (current_hpos(), 2); blt_info.bltafwm = v; reset_blit (0); }
static void BLTALWM (uae_u16 v) { maybe_blit (current_hpos(), 2); blt_info.bltalwm = v; reset_blit (0); }

static void BLTAPTH (uae_u16 v) { maybe_blit (current_hpos(), 0); bltapt = (bltapt & 0xffff) | ((uae_u32)v << 16); }
static void BLTAPTL (uae_u16 v) { maybe_blit (current_hpos(), 0); bltapt = (bltapt & ~0xffff) | (v & 0xFFFE); }
static void BLTBPTH (uae_u16 v) { maybe_blit (current_hpos(), 0); bltbpt = (bltbpt & 0xffff) | ((uae_u32)v << 16); }
static void BLTBPTL (uae_u16 v) { maybe_blit (current_hpos(), 0); bltbpt = (bltbpt & ~0xffff) | (v & 0xFFFE); }
static void BLTCPTH (uae_u16 v) { maybe_blit (current_hpos(), 0); bltcpt = (bltcpt & 0xffff) | ((uae_u32)v << 16); }
static void BLTCPTL (uae_u16 v) { maybe_blit (current_hpos(), 0); bltcpt = (bltcpt & ~0xffff) | (v & 0xFFFE); }
static void BLTDPTH (uae_u16 v) { maybe_blit (current_hpos(), 0); bltdpt = (bltdpt & 0xffff) | ((uae_u32)v << 16); }
static void BLTDPTL (uae_u16 v) { maybe_blit (current_hpos(), 0); bltdpt = (bltdpt & ~0xffff) | (v & 0xFFFE); }

static void BLTSIZE (uae_u16 v)
{
    maybe_blit (current_hpos(), 0);

    blt_info.vblitsize = v >> 6;
    blt_info.hblitsize = v & 0x3F;
    if (!blt_info.vblitsize) blt_info.vblitsize = 1024;
    if (!blt_info.hblitsize) blt_info.hblitsize = 64;
    do_blitter (current_hpos());
}

static void BLTSIZV (uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	return;
    maybe_blit (current_hpos(), 0);
    blt_info.vblitsize = v & 0x7FFF;
}

static void BLTSIZH (uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	return;
    maybe_blit (current_hpos(), 0);
    blt_info.hblitsize = v & 0x7FF;
    if (!blt_info.vblitsize)
	blt_info.vblitsize = 32768;
    if (!blt_info.hblitsize)
	blt_info.hblitsize = 0x800;
    do_blitter (current_hpos());
}

STATIC_INLINE void spr_arm (unsigned int num, int state)
{
    switch (state) {
	case 0:
	    nr_armed -= spr[num].armed;
	    spr[num].armed = 0;
	    break;
	default:
            nr_armed += 1 - spr[num].armed;
	    spr[num].armed = 1;
	    break;
    }
}

STATIC_INLINE void sprstartstop (struct sprite *s)
{
    if (vpos == s->vstart)
	s->dmastate = 1;
    if (vpos == s->vstop)
	s->dmastate = 0;
}

STATIC_INLINE void SPRxCTLPOS (unsigned int num)
{
    unsigned int sprxp;
    struct sprite *s = &spr[num];

    sprstartstop (s);
    sprxp = (sprpos[num] & 0xFF) * 2 + (sprctl[num] & 1);
    /* Quite a bit salad in this register... */
#ifdef AGA
    if (currprefs.chipset_mask & CSMASK_AGA) {
	/* We ignore the SHRES 35ns increment for now; SHRES support doesn't
	   work anyway, so we may as well restrict AGA sprites to a 70ns
	   resolution.  */
	sprxp <<= 1;
	sprxp |= (sprctl[num] >> 4) & 1;
    }
#endif
    s->xpos = sprxp;
    s->vstart = (sprpos[num] >> 8) | ((sprctl[num] << 6) & 0x100);
    s->vstop = (sprctl[num] >> 8) | ((sprctl[num] << 7) & 0x100);
    if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
	s->vstart |= (sprctl[num] << 3) & 0x200;
	s->vstop |= (sprctl[num] << 4) & 0x200;
    }
    sprstartstop (s);
}

STATIC_INLINE void SPRxCTL_1 (uae_u16 v, unsigned int num, unsigned int hpos)
{
#if SPRITE_DEBUG > 0
    struct sprite *s = &spr[num];
#endif
    sprctl[num] = v;
    spr_arm (num, 0);
    SPRxCTLPOS (num);
#if SPRITE_DEBUG > 0
    if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	write_log ("%d:%d:SPR%dCTL %04.4X P=%06.6X VSTRT=%d VSTOP=%d HSTRT=%d D=%d A=%d CP=%x PC=%x\n",
	    vpos, hpos, num, v, s->pt, s->vstart, s->vstop, s->xpos, spr[num].dmastate, spr[num].armed, cop_state.ip, m68k_getpc (&regs));
    }
#endif

}
STATIC_INLINE void SPRxPOS_1 (uae_u16 v, unsigned int num, unsigned int hpos)
{
#if SPRITE_DEBUG > 0
    struct sprite *s = &spr[num];
#endif
    sprpos[num] = v;
    SPRxCTLPOS (num);
#if SPRITE_DEBUG > 0
    if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	write_log ("%d:%d:SPR%dPOS %04.4X P=%06.6X VSTRT=%d VSTOP=%d HSTRT=%d D=%d A=%d CP=%x PC=%x\n",
	    vpos, hpos, num, v, s->pt, s->vstart, s->vstop, s->xpos, spr[num].dmastate, spr[num].armed, cop_state.ip, m68k_getpc (&regs));
    }
#endif
}

STATIC_INLINE void SPRxDATA_1 (uae_u16 v, unsigned int num, unsigned int hpos)
{
    sprdata[num][0] = v;
#ifdef AGA
    sprdata[num][1] = v;
    sprdata[num][2] = v;
    sprdata[num][3] = v;
#endif
    spr_arm (num, 1);
#if SPRITE_DEBUG > 1
    if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	write_log ("%d:%d:SPR%dDATA %04.4X P=%06.6X D=%d A=%d PC=%x\n",
	    vpos, hpos, num, v, spr[num].pt, spr[num].dmastate, spr[num].armed, m68k_getpc (&regs));
    }
#endif
}

STATIC_INLINE void SPRxDATB_1 (uae_u16 v, unsigned int num, unsigned int hpos)
{
    sprdatb[num][0] = v;
#ifdef AGA
    sprdatb[num][1] = v;
    sprdatb[num][2] = v;
    sprdatb[num][3] = v;
#endif
#if SPRITE_DEBUG > 1
    if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	write_log ("%d:%d:SPR%dDATB %04.4X P=%06.6X D=%d A=%d PC=%x\n",
	    vpos, hpos, num, v, spr[num].pt, spr[num].dmastate, spr[num].armed, m68k_getpc (&regs));
    }
#endif
}

static void SPRxDATA (unsigned int hpos, uae_u16 v, unsigned int num) { decide_sprites (hpos); SPRxDATA_1 (v, num, hpos); }
static void SPRxDATB (unsigned int hpos, uae_u16 v, unsigned int num) { decide_sprites (hpos); SPRxDATB_1 (v, num, hpos); }
static void SPRxCTL  (unsigned int hpos, uae_u16 v, unsigned int num) { decide_sprites (hpos); SPRxCTL_1 (v, num, hpos); }
static void SPRxPOS  (unsigned int hpos, uae_u16 v, unsigned int num) { decide_sprites (hpos); SPRxPOS_1 (v, num, hpos); }

static void SPRxPTH  (unsigned int hpos, uae_u16 v, unsigned int num)
{
    decide_sprites (hpos);
    spr[num].pt &= 0xffff;
    spr[num].pt |= (uae_u32)v << 16;
#if SPRITE_DEBUG > 0
    if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	write_log ("%d:%d:SPR%dPTH %06.6X\n", vpos, hpos, num, spr[num].pt);
    }
#endif
}

static void SPRxPTL (unsigned int hpos, uae_u16 v, unsigned int num)
{
    decide_sprites (hpos);
    spr[num].pt &= ~0xffff;
    spr[num].pt |= v;
#if SPRITE_DEBUG > 0
     if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	write_log ("%d:%d:SPR%dPTL %06.6X\n", vpos, hpos, num, spr[num].pt);
     }
#endif
}

static void CLXCON (uae_u16 v)
{
    clxcon = v;
    clxcon_bpl_enable = (v >> 6) & 63;
    clxcon_bpl_match = v & 63;
}

static void CLXCON2 (uae_u16 v)
{
    if (!(currprefs.chipset_mask & CSMASK_AGA))
	return;
    clxcon2 = v;
    clxcon_bpl_enable |= v & (0x40|0x80);
    clxcon_bpl_match |= (v & (0x01|0x02)) << 6;
}

static uae_u16 CLXDAT (void)
{
    uae_u16 v = clxdat | 0x8000;
    clxdat = 0;
    return v;
}

#ifdef AGA

static uae_u16 COLOR_READ (int num)
{
    int cr, cg, cb, colreg;
    uae_u16 cval;

    if (!(currprefs.chipset_mask & CSMASK_AGA) || !(bplcon2 & 0x0100))
	return 0xffff;

    colreg = ((bplcon3 >> 13) & 7) * 32 + num;
    cr = current_colors.color_regs_aga[colreg] >> 16;
    cg = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
    cb = current_colors.color_regs_aga[colreg] & 0xFF;
    if (bplcon3 & 0x200)
	cval = ((cr & 15) << 8) | ((cg & 15) << 4) | ((cb & 15) << 0);
    else
	cval = ((cr >> 4) << 8) | ((cg >> 4) << 4) | ((cb >> 4) << 0);
    return cval;
}
#endif

static void COLOR_WRITE (unsigned int hpos, uae_u16 v, int num)
{
    v &= 0xFFF;
#ifdef AGA
    if (currprefs.chipset_mask & CSMASK_AGA) {
	unsigned int r,g,b;
	unsigned int cr,cg,cb;
	unsigned int colreg;
	uae_u32 cval;

	/* writing is disabled when RDRAM=1 */
	if (bplcon2 & 0x0100)
	    return;

	colreg = ((bplcon3 >> 13) & 7) * 32 + num;
	r = (v & 0xF00) >> 8;
	g = (v & 0xF0) >> 4;
	b = (v & 0xF) >> 0;
	cr =  current_colors.color_regs_aga[colreg] >> 16;
	cg = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
	cb =  current_colors.color_regs_aga[colreg] & 0xFF;

	if (bplcon3 & 0x200) {
	    cr &= 0xF0; cr |= r;
	    cg &= 0xF0; cg |= g;
	    cb &= 0xF0; cb |= b;
	} else {
	    cr = r + (r << 4);
	    cg = g + (g << 4);
	    cb = b + (b << 4);
	}
	cval = (cr << 16) | (cg << 8) | cb;
	if (cval == current_colors.color_regs_aga[colreg])
	    return;

	/* Call this with the old table still intact. */
	record_color_change (hpos, colreg, cval);
	remembered_color_entry = -1;
	current_colors.color_regs_aga[colreg] = cval;
	current_colors.acolors[colreg] = CONVERT_RGB (cval);
   } else {
#endif
	if (current_colors.color_regs_ecs[num] == v)
	    return;
	/* Call this with the old table still intact. */
	record_color_change (hpos, num, v);
	remembered_color_entry = -1;
	current_colors.color_regs_ecs[num] = v;
	current_colors.acolors[num] = xcolors[v];
#ifdef AGA
    }
#endif
}

/* The copper code.  The biggest nightmare in the whole emulator.

   Alright.  The current theory:
   1. Copper moves happen 2 cycles after state READ2 is reached.
      It can't happen immediately when we reach READ2, because the
      data needs time to get back from the bus.  An additional 2
      cycles are needed for non-Agnus registers, to take into account
      the delay for moving data from chip to chip.
   2. As stated in the HRM, a WAIT really does need an extra cycle
      to wake up.  This is implemented by _not_ falling through from
      a successful wait to READ1, but by starting the next cycle.
      (Note: the extra cycle for the WAIT apparently really needs a
      free cycle; i.e. contention with the bitplane fetch can slow
      it down).
   3. Apparently, to compensate for the extra wake up cycle, a WAIT
      will use the _incremented_ horizontal position, so the WAIT
      cycle normally finishes two clocks earlier than the position
      it was waiting for.  The extra cycle then takes us to the
      position that was waited for.
      If the earlier cycle is busy with a bitplane, things change a bit.
      E.g., waiting for position 0x50 in a 6 plane display: In cycle
      0x4e, we fetch BPL5, so the wait wakes up in 0x50, the extra cycle
      takes us to 0x54 (since 0x52 is busy), then we have READ1/READ2,
      and the next register write is at 0x5c.
   4. The last cycle in a line is not usable for the copper.
   5. A 4 cycle delay also applies to the WAIT instruction.  This means
      that the second of two back-to-back WAITs (or a WAIT whose
      condition is immediately true) takes 8 cycles.
   6. This also applies to a SKIP instruction.  The copper does not
      fetch the next instruction while waiting for the second word of
      a WAIT or a SKIP to arrive.
   7. A SKIP also seems to need an unexplained additional two cycles
      after its second word arrives; this is _not_ a memory cycle (I
      think, the documentation is pretty clear on this).
   8. Two additional cycles are inserted when writing to COPJMP1/2.  */

/* Determine which cycles are available for the copper in a display
 * with a agiven number of planes.  */

STATIC_INLINE int copper_cant_read (unsigned int hpos)
{
    if (hpos + 1 >= maxhpos)
	return 1;
    return is_bitplane_dma_inline (hpos);
}

STATIC_INLINE int dangerous_reg (int reg)
{
    /* Safe:
     * Bitplane pointers, control registers, modulos and data.
     * Sprite pointers, control registers, and data.
     * Color registers.  */
    if (reg >= 0xE0 && reg < 0x1C0)
	return 0;
    return 1;
}

static int test_copper_dangerous (unsigned int address)
{
    if ((address & 0x1fe) < (copcon & 2 ? ((currprefs.chipset_mask & CSMASK_AGA) ? 0 : 0x40u) : 0x80u)) {
	cop_state.state = COP_stop;
	copper_enabled_thisline = 0;
	unset_special (&regs, SPCFLAG_COPPER);
	return 1;
    }
    return 0;
}

static void perform_copper_write (unsigned int old_hpos)
{
    unsigned int address = cop_state.saved_i1 & 0x1FE;

#ifdef DEBUGGER
    if (debug_copper)
	record_copper (cop_state.saved_ip - 4, old_hpos, vpos);
#endif

    if (test_copper_dangerous (address))
	return;

    if (address == 0x88) {
	cop_state.ip = cop1lc;
	cop_state.state = COP_strobe_delay;
    } else if (address == 0x8A) {
	cop_state.ip = cop2lc;
	cop_state.state = COP_strobe_delay;
    } else {
	custom_wput_1 (old_hpos, cop_state.saved_i1, cop_state.saved_i2, 0);
	cop_state.last_write = cop_state.saved_i1;
	cop_state.last_write_hpos = old_hpos;
	old_hpos++;
	if (cop_state.saved_i1 >= 0x140 && cop_state.saved_i1 < 0x180 && old_hpos >= SPR0_HPOS && old_hpos < SPR0_HPOS + 4 * MAX_SPRITES) {
	    //write_log ("%d:%d %04.4X:%04.4X\n", vpos, old_hpos, cop_state.saved_i1, cop_state.saved_i2);
	    do_sprites (old_hpos);
	}
    }
}

static int isagnus[]= {
    1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0, /* 32 0x00 - 0x3e */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 27 0x40 - 0x74 */

    0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0, /* 21 */
    1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0, /* 32 0xa0 - 0xde */
    /* BPLxPTH/BPLxPTL */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 16 */
    /* BPLCON0-3,BPLMOD1-2 */
    0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0, /* 16 */
    /* SPRxPTH/SPRxPTL */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 16 */
    /* SPRxPOS/SPRxCTL/SPRxDATA/SPRxDATB */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    /* COLORxx */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* RESERVED */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static void dump_copper (const char *error, unsigned int until_hpos)
{
    static int warned = 10;

    if (warned < 0)
	return;
    warned--;
    write_log("%s: vpos=%d until_hpos=%d\n",
	error, vpos, until_hpos);
    write_log("cvcmp=%d chcmp=%d chpos=%d cvpos=%d ci1=%04.4X ci2=%04.4X\n",
	cop_state.vcmp,cop_state.hcmp,cop_state.hpos,cop_state.vpos,cop_state.saved_i1,cop_state.saved_i2);
    write_log("cstate=%d ip=%08.8X ev_copper=%d\n",
	cop_state.state,cop_state.ip,eventtab[ev_copper].active);
}

static void update_copper (unsigned int until_hpos)
{
    unsigned int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
    unsigned int c_hpos = cop_state.hpos;

    if (eventtab[ev_copper].active) {
	dump_copper ("error1",until_hpos);
	eventtab[ev_copper].active = 0;
	return;
    }

    if (cop_state.state == COP_wait && vp < cop_state.vcmp) {
	dump_copper ("error2",until_hpos);
	eventtab[ev_copper].active = 0;
	copper_enabled_thisline = 0;
	return;
    }

    until_hpos &= ~1;

    if (until_hpos > (maxhpos & ~1))
	until_hpos = maxhpos & ~1;

    until_hpos += 2;
    for (;;) {
	unsigned int old_hpos = c_hpos;
	unsigned int hp;

	if (c_hpos >= until_hpos)
	    break;

	/* So we know about the fetch state.  */
	decide_line (c_hpos);

	switch (cop_state.state) {
	case COP_read1_in2:
	    cop_state.state = COP_read1;
	    break;
	case COP_read1_wr_in2:
	    cop_state.state = COP_read1;
	    perform_copper_write (old_hpos);
	    /* That could have turned off the copper.  */
	    if (! copper_enabled_thisline)
		goto out;

	    break;
	case COP_read1_wr_in4:
	    cop_state.state = COP_read1_wr_in2;
	    break;
	case COP_read2_wr_in2:
	    cop_state.state = COP_read2;
	    perform_copper_write (old_hpos);
	    /* That could have turned off the copper.  */
	    if (! copper_enabled_thisline)
		goto out;

	    break;
	case COP_wait_in2:
	    cop_state.state = COP_wait1;
	    break;
	case COP_wait_in4:
	    cop_state.state = COP_wait_in2;
	    break;
	case COP_skip_in2:
	    cop_state.state = COP_skip1;
	    break;
	case COP_skip_in4:
	    cop_state.state = COP_skip_in2;
	    break;
	case COP_strobe_delay:
	    cop_state.state = COP_read1_in2;
	    break;

	default:
	    break;
	}

	c_hpos += 2;
#if 0
	if (copper_cant_read (old_hpos))
	    continue;
#endif
	if (cop_state.strobe) {
	    if (cop_state.strobe > 0)
		cop_state.ip = cop_state.strobe == 1 ? cop1lc : cop2lc;
	    cop_state.strobe = 0;
	}

	switch (cop_state.state) {

	case COP_read1_wr_in4:
	    uae_abort ("COP_read1_wr_in4");

	case COP_read1_wr_in2:
	case COP_read1:
	    if (copper_cant_read (old_hpos))
		continue;
	    cop_state.i1 = chipmem_wget (cop_state.ip);
#ifdef CPUEMU_6
	    cycle_line[old_hpos] |= CYCLE_COPPER;
#endif
	    cop_state.ip += 2;
	    cop_state.state = cop_state.state == COP_read1 ? COP_read2 : COP_read2_wr_in2;
	    break;

	case COP_read2_wr_in2:
	    uae_abort ("read2_wr_in2");

	case COP_read2:
	    if (copper_cant_read (old_hpos))
		continue;
	    cop_state.i2 = chipmem_wget (cop_state.ip);
#ifdef CPUEMU_6
	    cycle_line[old_hpos] |= CYCLE_COPPER;
#endif
	    cop_state.ip += 2;
	    if (cop_state.ignore_next) {
		cop_state.ignore_next = 0;
		cop_state.state = COP_read1;
		break;
	    }

	    cop_state.saved_i1 = cop_state.i1;
	    cop_state.saved_i2 = cop_state.i2;
	    cop_state.saved_ip = cop_state.ip;

	    if (cop_state.i1 & 1) {
		if (cop_state.i2 & 1)
		    cop_state.state = COP_skip_in4;
		else
		    cop_state.state = COP_wait_in4;
	    } else {
		unsigned int reg = cop_state.i1 & 0x1FE;
		cop_state.state = isagnus[reg >> 1] ? COP_read1_wr_in2 : COP_read1_wr_in4;
	    }
	    break;

	case COP_wait1:
	    /* There's a nasty case here.  As stated in the "Theory" comment above, we
	       test against the incremented copper position.  I believe this means that
	       we have to increment the _vertical_ position at the last cycle in the line,
	       and set the horizontal position to 0.
	       Normally, this isn't going to make a difference, since we consider these
	       last cycles unavailable for the copper, so waking up in the last cycle has
	       the same effect as waking up at the start of the line.  However, there is
	       one possible problem:  If we're at 0xFFE0, any wait for an earlier position
	       must _not_ complete (since, in effect, the current position will be back
	       at 0/0).  This can be seen in the Superfrog copper list.
	       Things get monstrously complicated if we try to handle this "properly" by
	       incrementing vpos and setting c_hpos to 0.  Especially the various speedup
	       hacks really assume that vpos remains constant during one line.  Hence,
	       this hack: defer the entire decision until the next line if necessary.  */
	    if (c_hpos >= (maxhpos & ~1))
		break;

	    cop_state.state = COP_wait;

	    cop_state.vcmp = (cop_state.saved_i1 & (cop_state.saved_i2 | 0x8000)) >> 8;
	    cop_state.hcmp = (cop_state.saved_i1 & cop_state.saved_i2 & 0xFE);

	    vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);

	    if (cop_state.saved_i1 == 0xFFFF && cop_state.saved_i2 == 0xFFFE) {
		cop_state.state = COP_stop;
		copper_enabled_thisline = 0;
		unset_special (&regs, SPCFLAG_COPPER);
		goto out;
	    }
	    if (vp < cop_state.vcmp) {
		copper_enabled_thisline = 0;
		unset_special (&regs, SPCFLAG_COPPER);
		goto out;
	    }

	    /* fall through */
	case COP_wait:
	    if (vp < cop_state.vcmp)
		uae_abort ("vp < cop_state.vcmp");
	    if (copper_cant_read (old_hpos))
		continue;

	    hp = c_hpos & (cop_state.saved_i2 & 0xFE);
	    if (vp == cop_state.vcmp && hp < cop_state.hcmp)
		break;

	    /* Now we know that the comparisons were successful.  We might still
	       have to wait for the blitter though.  */
	    if ((cop_state.saved_i2 & 0x8000) == 0 && (DMACONR() & 0x4000)) {
		/* We need to wait for the blitter.  */
		cop_state.state = COP_bltwait;
		copper_enabled_thisline = 0;
		unset_special (&regs, SPCFLAG_COPPER);
		goto out;
	    }

#ifdef DEBUGGER
	    if (debug_copper)
		record_copper (cop_state.ip - 4, old_hpos, vpos);
#endif

	    cop_state.state = COP_read1;
	    break;

	case COP_skip1:
	{
	    static int skipped_before;
	    unsigned int vcmp, hcmp, vp1, hp1;

	    if (! skipped_before) {
		skipped_before = 1;
		write_log ("Program uses Copper SKIP instruction.\n");
	    }

	    if (c_hpos >= (maxhpos & ~1))
		break;

	    vcmp = (cop_state.saved_i1 & (cop_state.saved_i2 | 0x8000)) >> 8;
	    hcmp = (cop_state.saved_i1 & cop_state.saved_i2 & 0xFE);
	    vp1 = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
	    hp1 = c_hpos & (cop_state.saved_i2 & 0xFE);

	    if ((vp1 > vcmp || (vp1 == vcmp && hp1 >= hcmp))
		&& ((cop_state.saved_i2 & 0x8000) != 0 || ! (DMACONR() & 0x4000)))
		cop_state.ignore_next = 1;
	    if (chipmem_wget (cop_state.ip) & 1) { /* FIXME: HACK!!! */
		/* copper never skips if following instruction is WAIT or another SKIP... */
		cop_state.ignore_next = 0;
	    }

	    cop_state.state = COP_read1;

	    if (cop_state.ignore_next && (chipmem_wget (cop_state.ip) & 1) == 0) {
		/* another undocumented copper feature:
		   copper stops if skipped instruction is MOVE to dangerous register...
		*/
		test_copper_dangerous (chipmem_wget(cop_state.ip));
	    }

#ifdef DEBUGGER
	    if (debug_copper)
		record_copper (cop_state.ip - 4, old_hpos, vpos);
#endif

	    break;
	}
	default:
	    break;
	}
    }

  out:
    cop_state.hpos = c_hpos;
}

static void compute_spcflag_copper (void)
{
    copper_enabled_thisline = 0;
    unset_special (&regs, SPCFLAG_COPPER);
    if (! dmaen (DMA_COPPER) || cop_state.state == COP_stop || cop_state.state == COP_bltwait)
	return;

    if (cop_state.state == COP_wait) {
	unsigned int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);

	if (vp < cop_state.vcmp)
	    return;
    }
    copper_enabled_thisline = 1;

    if (! eventtab[ev_copper].active)
	set_special (&regs, SPCFLAG_COPPER);
}

void copper_handler (void)
{
    /* This will take effect immediately, within the same cycle.  */
    set_special (&regs, SPCFLAG_COPPER);

    if (! copper_enabled_thisline)
	uae_abort ("copper_handler");

    eventtab[ev_copper].active = 0;
}

void blitter_done_notify (void)
{
    if (cop_state.state != COP_bltwait)
	return;

    cop_state.hpos = current_hpos () & ~1;
    cop_state.vpos = vpos;
    /* apparently there is small delay until copper wakes up.. */
    cop_state.state = COP_wait_in2;
    compute_spcflag_copper ();
}

void do_copper (void)
{
    unsigned int hpos = current_hpos ();
    update_copper (hpos);
}

/* ADDR is the address that is going to be read/written; this access is
   the reason why we want to update the copper.  This function is also
   used from hsync_handler to finish up the line; for this case, we check
   hpos against maxhpos.  */
STATIC_INLINE void sync_copper_with_cpu (unsigned int hpos, int do_schedule)
{
    /* Need to let the copper advance to the current position.  */
    if (eventtab[ev_copper].active) {
	eventtab[ev_copper].active = 0;
	if (do_schedule)
	    events_schedule ();
	set_special (&regs, SPCFLAG_COPPER);
    }
    if (copper_enabled_thisline)
	update_copper (hpos);
}

STATIC_INLINE uae_u16 sprite_fetch (struct sprite *s, int dma, unsigned int hpos, int cycle, int mode)
{
    uae_u16 data = last_custom_value;
    if (dma) {
	data = last_custom_value = chipmem_wget (s->pt);
#ifdef CPUEMU_6
	cycle_line[hpos] |= CYCLE_SPRITE;
#endif
    }
    s->pt += 2;
    return data;
}

STATIC_INLINE void do_sprites_1 (unsigned int num, int cycle, unsigned int hpos)
{
    struct sprite *s = &spr[num];
    int dma, posctl = 0;
    uae_u16 data;

    if (vpos == sprite_vblank_endline)
	spr_arm (num, 0);

#if SPRITE_DEBUG > 3
    if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY)
	write_log("%d:%d:slot%d:%d\n", vpos, hpos, num, cycle);
#endif
    if (vpos == s->vstart) {
#if SPRITE_DEBUG > 0
	if (!s->dmastate && vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY)
	    write_log ("%d:%d:SPR%d START\n", vpos, hpos, num);
#endif
	s->dmastate = 1;
    }
    if (vpos == s->vstop || vpos == sprite_vblank_endline) {
#if SPRITE_DEBUG > 0
	if (s->dmastate && vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY)
	    write_log ("%d:%d:SPR%d STOP\n", vpos, hpos, num);
#endif
	s->dmastate = 0;
    }
    if (!dmaen (DMA_SPRITE))
	return;
    if (cycle && !s->dmacycle)
	return; /* Superfrog intro flashing bee fix */

    dma = hpos < plfstrt || diwstate != DIW_waiting_stop;
    if (vpos == s->vstop || vpos == sprite_vblank_endline) {
	s->dmastate = 0;
	posctl = 1;
	if (dma) {
	    data = sprite_fetch (s, dma, hpos, cycle, 0);
	    switch (sprite_width)
	    {
		case 64:
		sprite_fetch (s, dma, hpos, cycle, 0);
		sprite_fetch (s, dma, hpos, cycle, 0);
		case 32:
		sprite_fetch (s, dma, hpos, cycle, 0);
		break;
	    }
	} else {
	    data = cycle == 0 ? sprpos[num] : sprctl[num];
	}
#if SPRITE_DEBUG > 1
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	    write_log ("%d:%d:dma:P=%06.6X ", vpos, hpos, s->pt);
	}
#endif
	//write_log ("%d:%d: %04.4X=%04.4X\n", vpos, hpos, 0x140 + cycle * 2 + num * 8, data);
	if (cycle == 0) {
	    SPRxPOS_1 (data, num, hpos);
	    s->dmacycle = 1;
	} else {
	    SPRxCTL_1 (data, num, hpos);
	    s->dmastate = 0;
	    sprstartstop (s);
	}
    }
    if (s->dmastate && !posctl) {
	uae_u16 data;

	data = sprite_fetch (s, dma, hpos, cycle, 1);

#if SPRITE_DEBUG > 1
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY) {
	    write_log ("%d:%d:dma:P=%06.6X ", vpos, hpos, s->pt);
	}
#endif
	if (cycle == 0) {
	    SPRxDATA_1 (dma ? data : sprdata[num][0], num, hpos);
	    s->dmacycle = 1;
	} else {
	    SPRxDATB_1 (dma ? data : sprdatb[num][0], num, hpos);
	    spr_arm (num, 1);
	}
#ifdef AGA
	switch (sprite_width)
	{
	    case 64:
	    {
		uae_u16 data32 = sprite_fetch (s, dma, hpos, cycle, 1);
		uae_u16 data641 = sprite_fetch (s, dma, hpos, cycle, 1);
		uae_u16 data642 = sprite_fetch (s, dma, hpos, cycle, 1);
		if (dma) {
		    if (cycle == 0) {
			sprdata[num][3] = data642;
			sprdata[num][2] = data641;
			sprdata[num][1] = data32;
		    } else {
			sprdatb[num][3] = data642;
			sprdatb[num][2] = data641;
			sprdatb[num][1] = data32;
		    }
		}
	    }
	    break;
	    case 32:
	    {
		uae_u16 data32 = sprite_fetch (s, dma, hpos, cycle, 1);
		if (dma) {
		    if (cycle == 0)
			sprdata[num][1] = data32;
		    else
			sprdatb[num][1] = data32;
		}
	    }
	    break;
	}
#endif
    }
}

static void do_sprites (unsigned int hpos)
{
    unsigned int maxspr, minspr;
    unsigned int i;

    /* I don't know whether this is right. Some programs write the sprite pointers
     * directly at the start of the copper list. With the test against currvp, the
     * first two words of data are read on the second line in the frame. The problem
     * occurs when the program jumps to another copperlist a few lines further down
     * which _also_ writes the sprite pointer registers. This means that a) writing
     * to the sprite pointers sets the state to SPR_restart; or b) that sprite DMA
     * is disabled until the end of the vertical blanking interval. The HRM
     * isn't clear - it says that the vertical sprite position can be set to any
     * value, but this wouldn't be the first mistake... */
    /* Update: I modified one of the programs to write the sprite pointers the
     * second time only _after_ the VBlank interval, and it showed the same behaviour
     * as it did unmodified under UAE with the above check. This indicates that the
     * solution below is correct. */
    /* Another update: seems like we have to use the NTSC value here (see Sanity Turmoil
     * demo).  */
    /* Maximum for Sanity Turmoil: 27.
       Minimum for Sanity Arte: 22.  */
    if (vpos < sprite_vblank_endline)
	return;

#ifndef CUSTOM_SIMPLE
    maxspr = hpos;
    minspr = last_sprite_hpos;

    if (minspr >= SPR0_HPOS + MAX_SPRITES * 4 || maxspr < SPR0_HPOS)
	return;

    if (maxspr > SPR0_HPOS + MAX_SPRITES * 4)
	maxspr = SPR0_HPOS + MAX_SPRITES * 4;
    if (minspr < SPR0_HPOS)
	minspr = SPR0_HPOS;

    for (i = minspr; i < maxspr; i++) {
	int cycle = -1;
	unsigned int num = (i - SPR0_HPOS) / 4;
	switch ((i - SPR0_HPOS) & 3)
	    {
	    case 0:
	    cycle = 0;
	    spr[num].dmacycle = 0;
	    break;
	    case 2:
	    cycle = 1;
	    break;
	}
	if (cycle >= 0)
	    do_sprites_1 (num, cycle, i);
    }
    last_sprite_hpos = hpos;
#else
    for (i = 0; i < MAX_SPRITES * 2; i++) {
	spr[i / 2].dmacycle = 1;
	do_sprites_1 (i / 2, i & 1, 0);
    }
#endif
}

static void init_sprites (void)
{
    memset (sprpos, 0, sizeof sprpos);
    memset (sprctl, 0, sizeof sprctl);
}

/*
 * On systems without virtual memory or with low memory, we allocate the
 * sprite_entries and color_changes tables dynamically rather than having
 * them declared static. We don't initially allocate at their maximum sizes;
 * we start the tables off small and grow them as required.
 *
 * This function expands the tables if necessary.
 */
static void adjust_array_sizes (void)
{
#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
    if (delta_sprite_entry) {
	void *p1;
	void *p2;
	int   mcc = max_sprite_entry + 50 + delta_sprite_entry;

	delta_sprite_entry = 0;

	p1 = realloc (sprite_entries[0], mcc * sizeof (struct sprite_entry));
	p2 = realloc (sprite_entries[1], mcc * sizeof (struct sprite_entry));

	if (p1 && p2) {
	    sprite_entries[0] = p1;
	    sprite_entries[1] = p2;

	    memset (&sprite_entries[0][max_sprite_entry], (mcc - max_sprite_entry) * sizeof(struct sprite_entry), 0);
	    memset (&sprite_entries[1][max_sprite_entry], (mcc - max_sprite_entry) * sizeof(struct sprite_entry), 0);

	    write_log ("New max_sprite_entry=%d\n", mcc);

	    max_sprite_entry = mcc;
	} else
	    write_log ("WARNING: Failed to enlarge sprite_entries table\n");
    }
    if (delta_color_change) {
	void *p1;
	void *p2;
	int   mcc = max_color_change + 200 + delta_color_change;

	delta_color_change = 0;

	p1 = realloc (color_changes[0], mcc * sizeof (struct color_change));
	p2 = realloc (color_changes[1], mcc * sizeof (struct color_change));

	if (p1 && p2) {
	    color_changes[0] = p1;
	    color_changes[1] = p2;

	    write_log ("New max_color_change=%d\n", mcc);

	    max_color_change = mcc;
	} else
	    write_log ("WARNING: Failed to enlarge color_changes table\n");
    }
#endif
}

static void init_hardware_frame (void)
{
    next_lineno = 0;
    nextline_how = nln_normal;
    diwstate = DIW_waiting_start;
    hdiwstate = DIW_waiting_start;
    ddfstate = DIW_waiting_start;
    if (interlace_started > 0)
	interlace_started--;
}

void init_hardware_for_drawing_frame (void)
{
    /* Avoid this code in the first frame after a customreset.  */
    if (prev_sprite_entries) {
	int first_pixel = prev_sprite_entries[0].first_pixel;
	int npixels = prev_sprite_entries[prev_next_sprite_entry].first_pixel - first_pixel;
	memset (spixels + first_pixel, 0, npixels * sizeof *spixels);
	memset (spixstate.bytes + first_pixel, 0, npixels * sizeof *spixstate.bytes);
    }
    prev_next_sprite_entry = next_sprite_entry;

    next_color_change = 0;
    next_sprite_entry = 0;
    next_color_entry = 0;
    remembered_color_entry = -1;

    adjust_array_sizes ();

    prev_sprite_entries = sprite_entries[current_change_set];
    curr_sprite_entries = sprite_entries[current_change_set ^ 1];
    prev_color_changes = color_changes[current_change_set];
    curr_color_changes = color_changes[current_change_set ^ 1];
    prev_color_tables = color_tables[current_change_set];
    curr_color_tables = color_tables[current_change_set ^ 1];

    prev_drawinfo = line_drawinfo[current_change_set];
    curr_drawinfo = line_drawinfo[current_change_set ^ 1];
    current_change_set ^= 1;

    color_src_match = color_dest_match = -1;

    /* Use both halves of the array in alternating fashion.  */
    curr_sprite_entries[0].first_pixel = current_change_set * MAX_SPR_PIXELS;
    next_sprite_forced = 1;
}

/*
 * Convert time from syncbase ticks to milliseconds.
 */
STATIC_INLINE double frame_time_to_ms (frame_time_t time)
{
     return (double) time / (syncbase / 1000.0);
}

/*
 * Wait until end of frame in a system-friendly, non-CPU hogging way
 * if possible.
 */
static frame_time_t framewait_friendly (frame_time_t end_time)
{
    frame_time_t start_time = uae_gethrtime ();
    frame_time_t curr_time;
    frame_time_t time_left;

    /*
     * Use system sleep routine to get close to end_time
     */
    for (;;) {
	curr_time = uae_gethrtime ();
	time_left = end_time - curr_time;

	if (time_left < (-syncbase) || time_left > syncbase) {
	    /* Either timer has screwed up or we've badly overshot end_time */
	    write_log ("framewait overflow\n");
	    return curr_time;
	} else {
	    static int min_sleep = 3; /* Estimate of how long a 2ms sleep will actually take */

	    if (frame_time_to_ms (time_left) >= min_sleep) {
		static int count = 0;
		static int total = 0;

	        uae_msleep (2);

		/* Callibrate time we slept for and try to adjust to
		 * changing lantencies.
		 * TODO: This stuff needs tidying up, fine-tuning and some way
		 * of attempting to back-off busy-waiting.
		 */
		total += frame_time_to_ms (uae_gethrtime () - curr_time);
		count++;

		if ((count & 31) == 0) {
		    min_sleep = 1 + total / 32;
		    total = 0;
		    count = 0;
		}
	    } else
	        break;
	}
    }

    /*
     * Busy-wait remaining time.
     */
    do {
	curr_time = uae_gethrtime ();
	time_left = end_time - curr_time;

	if (time_left < (-syncbase) || time_left > syncbase) {
	    time_left = 0;
	}
    } while (time_left >= 0);

    idletime += curr_time - start_time;

    return curr_time;
}

/*
 * Wait for end of frame using CPU-hogging, polling method.
 */
static frame_time_t framewait_busy (frame_time_t end_time)
{
    frame_time_t start_time = uae_gethrtime ();
    frame_time_t curr_time;
    frame_time_t time_left;

    do {
	curr_time = uae_gethrtime ();
	time_left = end_time - curr_time;

	if (time_left < (-syncbase) || time_left > syncbase) {
	    /* Either timer has screwed up or we've badly overshot end_time */
	    write_log ("framewait overflow\n");
	    return curr_time;
	}
    } while (time_left >= 0);

    idletime += curr_time - start_time;

    return curr_time;
}

static void framewait (void)
{
    frame_time_t curr_time;

    /* TODO: Make this a compile-time option. */
    if (1)
	curr_time = framewait_friendly (vsyncmintime);
    else
	curr_time = framewait_busy (vsyncmintime);

    vsyncmintime = curr_time + vsynctime;
}

static frame_time_t frametime2;

void fpscounter_reset (void)
{
    timeframes = 0;
    frametime2 = 0;
    bogusframe = 2;
    lastframetime = uae_gethrtime ();
    idletime = 0;
}

static void fpscounter (void)
{
    frame_time_t now, last;

    now = uae_gethrtime ();
    last = now - lastframetime;
    lastframetime = now;

    if (bogusframe)
	return;

    frametime += (1000*last/syncbase);
    frametime2 += last;
    timeframes++;
    if ((timeframes & 31) == 0) {
	double idle = 1000 - (idletime == 0 ? 0.0 : (double)idletime * 1000.0 / (vsynctime * 32.0));
	int fps = frametime2 == 0 ? 0 : syncbase * 32 / (frametime2 / 10);
	if (fps > 9999)
	    fps = 9999;
	if (idle < 0)
	    idle = 0;
	if (idle > 100 * 10)
	    idle = 100 * 10;
	if (fake_vblank_hz * 10 > fps) {
	    double mult = (double)fake_vblank_hz * 10.0 / fps;
	    idle *= mult;
	}
	if (turbo_emulation && idle < 100 * 10)
	    idle = 100 * 10;
	gui_fps (fps, (int)idle);
	frametime2 = 0;
	idletime = 0;
    }
}

static void vsync_handler (void)
{
    fpscounter ();

    if (!is_vsync ()
#ifdef AVIOUTPUT
	&& ((avioutput_framelimiter && avioutput_enabled) || !avioutput_enabled)
#endif
	) {
#ifdef JIT
	if (!compiled_code) {
#endif
	    if (currprefs.m68k_speed == -1) {
		frame_time_t curr_time = uae_gethrtime ();
		vsyncmintime += vsynctime;
		/* @@@ Mathias? How do you think we should do this? */
		/* If we are too far behind, or we just did a reset, adjust the
		 * needed time. */
		if ((curr_time - vsyncmintime) > 0 || rpt_did_reset)
		    vsyncmintime = curr_time + vsynctime;
		rpt_did_reset = 0;
	    } else {
		framewait ();
	    }
#ifdef JIT
	} else {
	    if (currprefs.m68k_speed == 0) {
		framewait ();
	    }
	}
#endif
    }

    if (bogusframe > 0)
	bogusframe--;

    gui_handle_events ();
    handle_events ();

    INTREQ (0x8020);
    if (bplcon0 & 4)
	lof ^= 0x8000;

#ifdef PICASSO96
    /* And now let's update the Picasso palette, if required */
    DX_SetPalette_vsync();
    if (picasso_on)
	picasso_handle_vsync ();
#endif

   vsync_handle_redraw (lof, lof_changed);

    {
	static int cnt = 0;
	if (cnt == 0) {
	    /* resolution_check_change (); */
	    DISK_check_change ();
	    cnt = 5;
	}
	cnt--;
    }

#ifdef DEBUGGER
    if (debug_copper)
	record_copper_reset();
#endif

    /* For now, let's only allow this to change at vsync time.  It gets too
     * hairy otherwise.  */
    if ((beamcon0 & (0x20|0x80)) != (new_beamcon0 & (0x20|0x80)) || hack_vpos)
	init_hz ();

    lof_changed = 0;

    eventtab[ev_copper].active = 0;
    COPJMP (1);

    init_hardware_frame ();

    if (timehack_alive > 0)
	timehack_alive--;
    inputdevice_vsync ();
}

#ifdef JIT

#define N_LINES 8

static __inline__ int trigger_frh(int v)
{
    return (v & (N_LINES - 1)) == 0;
}

static long int diff32 (frame_time_t x, frame_time_t y)
{
    return (long int)(x - y);
}

static void frh_handler(void)
{
    if (currprefs.m68k_speed == -1) {
	frame_time_t curr_time = uae_gethrtime ();
	vsyncmintime += vsynctime * N_LINES / maxvpos;
	/* @@@ Mathias? How do you think we should do this? */
	/* If we are too far behind, or we just did a reset, adjust the
	 * needed time. */
	if (rpt_did_reset) {
	    vsyncmintime = curr_time + vsynctime;
	    rpt_did_reset = 0;
	}
	/* Allow this to be one frame's worth of cycles out */
	while (diff32 (curr_time, vsyncmintime + vsynctime) > 0) {
	    vsyncmintime += vsynctime * N_LINES / maxvpos;
	    if (turbo_emulation)
		break;
	}
    }
}
#endif

#if 0
static void copper_check (int n)
{
    if (cop_state.state == COP_wait) {
	unsigned int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
	if (vp < cop_state.vcmp) {
	    if (eventtab[ev_copper].active || copper_enabled_thisline)
		write_log ("COPPER BUG %d: vp=%d vpos=%d vcmp=%d act=%d thisline=%d\n", n, vp, vpos, cop_state.vcmp, eventtab[ev_copper].active, copper_enabled_thisline);
	}
    }
}
#endif

void hsync_handler (void)
{
    static unsigned int ciahsync;
    unsigned int hpos = current_hpos ();

    sync_copper_with_cpu (maxhpos, 0);
    finish_decisions ();
    if (thisline_decision.plfleft != -1) {
	if (currprefs.collision_level > 1)
	    do_sprite_collisions ();
	if (currprefs.collision_level > 2)
	    do_playfield_collisions ();
    }
    hsync_record_line_state (next_lineno, nextline_how, thisline_changed);

    eventtab[ev_hsync].evtime += get_cycles () - eventtab[ev_hsync].oldcycles;
    eventtab[ev_hsync].oldcycles = get_cycles ();
    CIA_hsync_handler ();
#ifdef CD32
    AKIKO_hsync_handler ();
#endif

#ifdef PICASSO96
    picasso_handle_hsync ();
#endif
    ciahsync++;
    if (ciahsync >= (currprefs.ntscmode ? MAXVPOS_NTSC : MAXVPOS_PAL) * MAXHPOS_PAL / maxhpos) { /* not so perfect.. */
	CIA_vsync_handler ();
	ciahsync = 0;
    }

    /* reset light pen latch */
    if (vpos == sprite_vblank_endline)
	vpos_lpen = -1;

#ifdef CPUEMU_6
    if (currprefs.cpu_cycle_exact || currprefs.blitter_cycle_exact) {
	decide_blitter (hpos);
	memset (cycle_line, 0, sizeof cycle_line);
	cycle_line[1] = CYCLE_REFRESH;
	cycle_line[3] = CYCLE_REFRESH;
	cycle_line[5] = CYCLE_REFRESH;
	cycle_line[7] = CYCLE_REFRESH;
    }
#endif
    if ((currprefs.chipset_mask & CSMASK_AGA) || (!currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	last_custom_value = rand ();
    else
	last_custom_value = 0xffff;

    if (!currprefs.blitter_cycle_exact && bltstate != BLT_done && dmaen (DMA_BITPLANE) && diwstate == DIW_waiting_stop)
	blitter_slowdown (thisline_decision.plfleft, thisline_decision.plfright - (16 << fetchmode),
	    cycle_diagram_total_cycles[fmode][GET_RES (bplcon0)][GET_PLANES_LIMIT (bplcon0)],
	    cycle_diagram_free_cycles[fmode][GET_RES (bplcon0)][GET_PLANES_LIMIT (bplcon0)]);

    if (currprefs.produce_sound)
	audio_hsync (1);

    hardware_line_completed (next_lineno);

    /* In theory only an equality test is needed here - but if a program
       goes haywire with the VPOSW register, it can cause us to miss this,
       with vpos going into the thousands (and all the nasty consequences
       this has).  */

    if (++vpos >= (maxvpos + (lof == 0 ? 0 : 1))) {
	if (bplcon0 & 8) {
	    vpos_lpen = vpos - 1;
	    hpos_lpen = maxhpos;
	}
	vpos = 0;
	vsync_handler ();
    }

    DISK_hsync (maxhpos);

#ifdef JIT
    if (compiled_code) {
	if (currprefs.m68k_speed == -1) {
	    static int count = 0;
	    count++;
	    if (trigger_frh(count)) {
		frh_handler();
	    }
	    is_lastline = trigger_frh(count+1) && ! rpt_did_reset;
	} else {
	    is_lastline=0;
	}
    } else {
#endif
	is_lastline = (vpos + 1 == (maxvpos + (lof == 0 ? 0 : 1))) && (currprefs.m68k_speed == -1) && ! rpt_did_reset;
#ifdef JIT
    }
#endif

    if ((bplcon0 & 4) && currprefs.gfx_linedbl)
	notice_interlace_seen ();

    if (!nodraw ()) {
	unsigned int lineno = vpos;
	nextline_how = nln_normal;
	if (currprefs.gfx_linedbl) {
	    lineno *= 2;
	    nextline_how = currprefs.gfx_linedbl == 1 ? nln_doubled : nln_nblack;
	    if (bplcon0 & 4) {
		if (!lof) {
		    lineno++;
		    nextline_how = nln_lower;
		} else {
		    nextline_how = nln_upper;
		}
	    }
	}
	next_lineno = lineno;
	reset_decisions ();
    }
#ifdef FILESYS
    if (uae_int_requested) {
	set_uae_int_flag ();
	INTREQ (0x8000 | 0x0008);
    }
#endif
    /* See if there's a chance of a copper wait ending this line.  */
    cop_state.hpos = 0;
    cop_state.last_write = 0;
    compute_spcflag_copper ();
    inputdevice_hsync ();
    serial_hsynchandler ();
    misc_hsync_stuff ();
#ifdef CUSTOM_SIMPLE
    do_sprites (0);
#endif

    hsync_counter++;
    //copper_check (2);
}

void customreset (void)
{
    int zero = 0;

    write_log ("reset at %x\n", m68k_getpc (&regs));
    hsync_counter = 0;
    if (! savestate_state) {
	currprefs.chipset_mask = changed_prefs.chipset_mask;
	if ((currprefs.chipset_mask & CSMASK_AGA) == 0) {
	    unsigned int i;
	    for (i = 0; i < 32; i++) {
		current_colors.color_regs_ecs[i] = 0;
		current_colors.acolors[i] = xcolors[0];
	    }
#ifdef AGA
	} else {
	    unsigned int i;
	    for (i = 0; i < 256; i++) {
		current_colors.color_regs_aga[i] = 0;
		current_colors.acolors[i] = CONVERT_RGB (zero);
	    }
#endif
	}

	clxdat = 0;

	/* Clear the armed flags of all sprites.  */
	memset (spr, 0, sizeof spr);
	nr_armed = 0;

	dmacon = intena = 0;

	copcon = 0;
	DSKLEN (0, 0);

	bplcon0 = 0;
	bplcon4 = 0x11; /* Get AGA chipset into ECS compatibility mode */
	bplcon3 = 0xC00;

	FMODE (0);
	CLXCON (0);
    }

#ifdef AUTOCONFIG
    expamem_reset ();
#endif
    a1000_reset ();
    DISK_reset ();
    CIA_reset ();
#ifdef JIT
    compemu_reset ();
#endif
    unset_special (&regs, ~(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE));

    vpos = 0;

    inputdevice_reset ();
    timehack_alive = 0;

    curr_sprite_entries = 0;
    prev_sprite_entries = 0;
    sprite_entries[0][0].first_pixel = 0;
    sprite_entries[1][0].first_pixel = MAX_SPR_PIXELS;
    sprite_entries[0][1].first_pixel = 0;
    sprite_entries[1][1].first_pixel = MAX_SPR_PIXELS;
    memset (spixels, 0, 2 * MAX_SPR_PIXELS * sizeof *spixels);
    memset (&spixstate, 0, sizeof spixstate);

    bltstate = BLT_done;
    cop_state.state = COP_stop;
    diwstate = DIW_waiting_start;
    hdiwstate = DIW_waiting_start;
    set_cycles (0);

    new_beamcon0 = currprefs.ntscmode ? 0x00 : 0x20;
    hack_vpos = 0;
    init_hz ();
    vpos_lpen = -1;

    audio_reset ();
    if (savestate_state != STATE_RESTORE) {
	/* must be called after audio_reset */
	adkcon = 0;
	serial_uartbreak (0);
	audio_update_adkmasks ();
    }

    init_sprites ();

    init_hardware_frame ();
    drawing_init ();

    reset_decisions ();

    bogusframe = 1;

    sprite_buffer_res = currprefs.chipset_mask & CSMASK_AGA ? RES_HIRES : RES_LORES;
    if (savestate_state == STATE_RESTORE) {
        unsigned int i;
	uae_u16 v;
	uae_u32 vv;

	audio_update_adkmasks ();
	INTENA (0);
	INTREQ (0);
#if 0
	DMACON (0, 0);
#endif
	COPJMP (1);
	v = bplcon0;
	BPLCON0 (0, 0);
	BPLCON0 (0, v);
	FMODE (fmode);
	if (!(currprefs.chipset_mask & CSMASK_AGA)) {
	    for (i = 0 ; i < 32 ; i++)  {
		vv = current_colors.color_regs_ecs[i];
		current_colors.color_regs_ecs[i] = -1;
		record_color_change (0, i, vv);
		remembered_color_entry = -1;
		current_colors.color_regs_ecs[i] = vv;
		current_colors.acolors[i] = xcolors[vv];
	    }
#ifdef AGA
	} else {
	    for(i = 0 ; i < 256 ; i++)  {
		vv = current_colors.color_regs_aga[i];
		current_colors.color_regs_aga[i] = -1;
		record_color_change (0, i, vv);
		remembered_color_entry = -1;
		current_colors.color_regs_aga[i] = vv;
		current_colors.acolors[i] = CONVERT_RGB(vv);
	    }
#endif
	}
	CLXCON (clxcon);
	CLXCON2 (clxcon2);
	calcdiw ();
	write_log ("State restored\n");
	dumpcustom ();
	for (i = 0; i < 8; i++)
	    nr_armed += spr[i].armed != 0;
	if (! currprefs.produce_sound) {
	    eventtab[ev_audio].active = 0;
	    events_schedule ();
	}
    }
    expand_sprres ();

    #ifdef ACTION_REPLAY
    /* Doing this here ensures we can use the 'reset' command from within AR */
    action_replay_reset ();
    #endif
    #if defined(ENFORCER)
    enforcer_disable();
    #endif
}

void dumpcustom (void)
{
    write_log ("DMACON: %x INTENA: %x INTREQ: %x VPOS: %x HPOS: %x\n", DMACONR(),
	       (unsigned int)intena, (unsigned int)intreq, (unsigned int)vpos, (unsigned int)current_hpos());
    write_log ("COP1LC: %08lx, COP2LC: %08lx COPPTR: %08lx\n", (unsigned long)cop1lc, (unsigned long)cop2lc, cop_state.ip);
    write_log ("DIWSTRT: %04x DIWSTOP: %04x DDFSTRT: %04x DDFSTOP: %04x\n",
	       (unsigned int)diwstrt, (unsigned int)diwstop, (unsigned int)ddfstrt, (unsigned int)ddfstop);
    write_log ("BPLCON 0: %04x 1: %04x 2: %04x 3: %04x 4: %04x\n", bplcon0, bplcon1, bplcon2, bplcon3, bplcon4);
    if (timeframes) {
	write_log ("Average frame time: %f ms [frames: %d time: %d]\n",
		   (double)frametime / timeframes, timeframes, frametime);
	if (total_skipped)
	    write_log ("Skipped frames: %d\n", total_skipped);
    }
    /*for (i=0; i<256; i++) if (blitcount[i]) write_log ("minterm %x = %d\n",i,blitcount[i]);  blitter debug */
}

static void gen_custom_tables (void)
{
    int i;
    for (i = 0; i < 256; i++) {
	sprtaba[i] = ((((i >> 7) & 1) << 0)
		      | (((i >> 6) & 1) << 2)
		      | (((i >> 5) & 1) << 4)
		      | (((i >> 4) & 1) << 6)
		      | (((i >> 3) & 1) << 8)
		      | (((i >> 2) & 1) << 10)
		      | (((i >> 1) & 1) << 12)
		      | (((i >> 0) & 1) << 14));
	sprtabb[i] = sprtaba[i] * 2;
	sprite_ab_merge[i] = (((i & 15) ? 1 : 0)
			      | ((i & 240) ? 2 : 0));
    }
    for (i = 0; i < 16; i++) {
	clxmask[i] = (((i & 1) ? 0xF : 0x3)
		      | ((i & 2) ? 0xF0 : 0x30)
		      | ((i & 4) ? 0xF00 : 0x300)
		      | ((i & 8) ? 0xF000 : 0x3000));
	sprclx[i] = (((i & 0x3) == 0x3 ? 1 : 0)
		     | ((i & 0x5) == 0x5 ? 2 : 0)
		     | ((i & 0x9) == 0x9 ? 4 : 0)
		     | ((i & 0x6) == 0x6 ? 8 : 0)
		     | ((i & 0xA) == 0xA ? 16 : 0)
		     | ((i & 0xC) == 0xC ? 32 : 0)) << 9;
    }
}

/* mousehack is now in "filesys boot rom" */
static uae_u32 REGPARAM2 mousehack_helper_old (struct TrapContext *ctx)
{
    return 0;
}

static int allocate_sprite_tables (void)
{
#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
    int num;

    delta_sprite_entry = 0;
    delta_color_change = 0;

    if (!sprite_entries[0]) {
        max_sprite_entry = DEFAULT_MAX_SPRITE_ENTRY;
        max_color_change = DEFAULT_MAX_COLOR_CHANGE;

	for (num = 0; num < 2; num++) {
	    sprite_entries[num] = xmalloc (max_sprite_entry * sizeof (struct sprite_entry));
	    color_changes[num] = xmalloc (max_color_change * sizeof (struct color_change));

	    if (sprite_entries[num] && color_changes[num]) {
		memset (sprite_entries[num], 0, max_sprite_entry * sizeof (struct sprite_entry));
		memset (color_changes[num], 0, max_color_change * sizeof (struct color_change));
	    } else
	    	return 0;
	}
    }

    if (!spixels) {
	spixels = xmalloc (2 * MAX_SPR_PIXELS * sizeof *spixels);
	if (!spixels)
	    return 0;
    }
#endif
    return 1;
}

int custom_init (void)
{
    if (!allocate_sprite_tables()) {
	gui_message ("Not enough memory for sprite tables\n");
	return 0;
    }

#ifdef AUTOCONFIG
    {
	uaecptr pos;
	pos = here ();

	org (RTAREA_BASE+0xFF70);
	calltrap (deftrap (mousehack_helper_old));
	dw (RTS);

	org (RTAREA_BASE+0xFFA0);
	calltrap (deftrap (timehack_helper));
	dw (RTS);

	org (pos);
    }
#endif

    gen_custom_tables ();
    build_blitfilltable ();

    drawing_init ();

    create_cycle_diagram_table ();

    /* We have to do this somewhere... */
    syncbase = uae_gethrtimebase ();

    return 1;
}

/* Custom chip memory bank */

static uae_u32 custom_lget (uaecptr) REGPARAM;
static uae_u32 custom_wget (uaecptr) REGPARAM;
static uae_u32 custom_bget (uaecptr) REGPARAM;
static void custom_lput (uaecptr, uae_u32) REGPARAM;
static void custom_wput (uaecptr, uae_u32) REGPARAM;
static void custom_bput (uaecptr, uae_u32) REGPARAM;

addrbank custom_bank = {
    custom_lget, custom_wget, custom_bget,
    custom_lput, custom_wput, custom_bput,
    default_xlate, default_check, NULL
};

STATIC_INLINE uae_u32 REGPARAM2 custom_wget_1 (uaecptr addr, int noput)
{
    uae_u16 v;
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    addr &= 0xfff;
#ifdef CUSTOM_DEBUG
    write_log ("%d:%d:wget: %04.4X=%04.4X pc=%p\n", current_hpos(), vpos, addr, addr & 0x1fe, m68k_getpc (&regs));
#endif
    switch (addr & 0x1fe) {
     case 0x002: v = DMACONR (); break;
     case 0x004: v = VPOSR (); break;
     case 0x006: v = VHPOSR (); break;

     case 0x00A: v = JOY0DAT (); break;
     case 0x00C: v = JOY1DAT (); break;
     case 0x00E: v = CLXDAT (); break;
     case 0x010: v = ADKCONR (); break;

     case 0x012: v = POT0DAT (); break;
     case 0x014: v = POT1DAT (); break;
     case 0x016: v = POTGOR (); break;
     case 0x018: v = SERDATR (); break;
     case 0x01A: v = DSKBYTR (current_hpos ()); break;
     case 0x01C: v = INTENAR (); break;
     case 0x01E: v = INTREQR (); break;
     case 0x07C: v = DENISEID (); break;

     case 0x02E: v = 0xffff; break; /* temporary hack */

#ifdef AGA
     case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
     case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
     case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
     case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
     case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
     case 0x1BC: case 0x1BE:
	v = COLOR_READ ((addr & 0x3E) / 2);
	break;
#endif

     default:
	/* reading write-only register causes write with last value in bus */
	v = last_custom_value;
	if (!noput) {
	    int r;
	    unsigned int hpos = current_hpos ();
	    decide_line (hpos);
	    decide_fetch (hpos);
	    decide_blitter (hpos);
	    v = last_custom_value;
	    r = custom_wput_1 (hpos, addr, v, 1);
	}
	return v;
    }
    return v;
}

STATIC_INLINE uae_u32 custom_wget2 (uaecptr addr)
{
    uae_u32 v;
    sync_copper_with_cpu (current_hpos (), 1);
    if (currprefs.cpu_level >= 2) {
	if (addr >= 0xde0000 && addr <= 0xdeffff)
	    return 0x7f7f;
	if (addr >= 0xdd0000 && addr <= 0xddffff)
	     return 0xffff;
    }
    v = custom_wget_1 (addr, 0);
#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_COMMON
    addr &= 0x1ff;
    ar_custom[addr + 0] = (uae_u8)(v >> 8);
    ar_custom[addr + 1] = (uae_u8)(v);
#endif
#endif
    return v;
}

uae_u32 REGPARAM2 custom_wget (uaecptr addr)
{
    uae_u32 v;

    if (addr & 1) {
	/* think about move.w $dff005,d0.. (68020+ only) */
	addr &= ~1;
	v = custom_wget2 (addr) << 8;
	v |= custom_wget2 (addr + 2) >> 8;
	return v;
    }
    return custom_wget2 (addr);
 }

uae_u32 REGPARAM2 custom_bget (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    return custom_wget2 (addr & ~1) >> (addr & 1 ? 0 : 8);
}

uae_u32 REGPARAM2 custom_lget (uaecptr addr)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_READ;
#endif
    return ((uae_u32)custom_wget (addr) << 16) | custom_wget (addr + 2);
}

int REGPARAM2 custom_wput_1 (unsigned int hpos, uaecptr addr, uae_u32 value, int noget)
{
    addr &= 0x1FE;
    value &= 0xffff;
#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_COMMON
    ar_custom[addr+0]=(uae_u8)(value>>8);
    ar_custom[addr+1]=(uae_u8)(value);
#endif
#endif
    last_custom_value = value;
    switch (addr) {
     case 0x00E: CLXDAT (); break;

     case 0x020: DSKPTH (value); break;
     case 0x022: DSKPTL (value); break;
     case 0x024: DSKLEN (value, hpos); break;
     case 0x026: DSKDAT (value); break;

     case 0x02A: VPOSW (value); break;
     case 0x02E: COPCON (value); break;
     case 0x030: SERDAT (value); break;
     case 0x032: SERPER (value); break;
     case 0x034: POTGO (value); break;
     case 0x040: BLTCON0 (value); break;
     case 0x042: BLTCON1 (value); break;

     case 0x044: BLTAFWM (value); break;
     case 0x046: BLTALWM (value); break;

     case 0x050: BLTAPTH (value); break;
     case 0x052: BLTAPTL (value); break;
     case 0x04C: BLTBPTH (value); break;
     case 0x04E: BLTBPTL (value); break;
     case 0x048: BLTCPTH (value); break;
     case 0x04A: BLTCPTL (value); break;
     case 0x054: BLTDPTH (value); break;
     case 0x056: BLTDPTL (value); break;

     case 0x058: BLTSIZE (value); break;

     case 0x064: BLTAMOD (value); break;
     case 0x062: BLTBMOD (value); break;
     case 0x060: BLTCMOD (value); break;
     case 0x066: BLTDMOD (value); break;

     case 0x070: BLTCDAT (value); break;
     case 0x072: BLTBDAT (value); break;
     case 0x074: BLTADAT (value); break;

     case 0x07E: DSKSYNC (hpos, value); break;

     case 0x080: COP1LCH (value); break;
     case 0x082: COP1LCL (value); break;
     case 0x084: COP2LCH (value); break;
     case 0x086: COP2LCL (value); break;

     case 0x088: COPJMP (1); break;
     case 0x08A: COPJMP (2); break;

     case 0x08E: DIWSTRT (hpos, value); break;
     case 0x090: DIWSTOP (hpos, value); break;
     case 0x092: DDFSTRT (hpos, value); break;
     case 0x094: DDFSTOP (hpos, value); break;

     case 0x096: DMACON (hpos, value); break;
     case 0x098: CLXCON (value); break;
     case 0x09A: INTENA (value); break;
     case 0x09C: INTREQ (value); break;
     case 0x09E: ADKCON (hpos, value); break;

     case 0x0A0: AUDxLCH (0, value); break;
     case 0x0A2: AUDxLCL (0, value); break;
     case 0x0A4: AUDxLEN (0, value); break;
     case 0x0A6: AUDxPER (0, value); break;
     case 0x0A8: AUDxVOL (0, value); break;
     case 0x0AA: AUDxDAT (0, value); break;

     case 0x0B0: AUDxLCH (1, value); break;
     case 0x0B2: AUDxLCL (1, value); break;
     case 0x0B4: AUDxLEN (1, value); break;
     case 0x0B6: AUDxPER (1, value); break;
     case 0x0B8: AUDxVOL (1, value); break;
     case 0x0BA: AUDxDAT (1, value); break;

     case 0x0C0: AUDxLCH (2, value); break;
     case 0x0C2: AUDxLCL (2, value); break;
     case 0x0C4: AUDxLEN (2, value); break;
     case 0x0C6: AUDxPER (2, value); break;
     case 0x0C8: AUDxVOL (2, value); break;
     case 0x0CA: AUDxDAT (2, value); break;

     case 0x0D0: AUDxLCH (3, value); break;
     case 0x0D2: AUDxLCL (3, value); break;
     case 0x0D4: AUDxLEN (3, value); break;
     case 0x0D6: AUDxPER (3, value); break;
     case 0x0D8: AUDxVOL (3, value); break;
     case 0x0DA: AUDxDAT (3, value); break;

     case 0x0E0: BPLxPTH (hpos, value, 0); break;
     case 0x0E2: BPLxPTL (hpos, value, 0); break;
     case 0x0E4: BPLxPTH (hpos, value, 1); break;
     case 0x0E6: BPLxPTL (hpos, value, 1); break;
     case 0x0E8: BPLxPTH (hpos, value, 2); break;
     case 0x0EA: BPLxPTL (hpos, value, 2); break;
     case 0x0EC: BPLxPTH (hpos, value, 3); break;
     case 0x0EE: BPLxPTL (hpos, value, 3); break;
     case 0x0F0: BPLxPTH (hpos, value, 4); break;
     case 0x0F2: BPLxPTL (hpos, value, 4); break;
     case 0x0F4: BPLxPTH (hpos, value, 5); break;
     case 0x0F6: BPLxPTL (hpos, value, 5); break;
     case 0x0F8: BPLxPTH (hpos, value, 6); break;
     case 0x0FA: BPLxPTL (hpos, value, 6); break;
     case 0x0FC: BPLxPTH (hpos, value, 7); break;
     case 0x0FE: BPLxPTL (hpos, value, 7); break;

     case 0x100: BPLCON0 (hpos, value); break;
     case 0x102: BPLCON1 (hpos, value); break;
     case 0x104: BPLCON2 (hpos, value); break;
#ifdef AGA
     case 0x106: BPLCON3 (hpos, value); break;
#endif

     case 0x108: BPL1MOD (hpos, value); break;
     case 0x10A: BPL2MOD (hpos, value); break;
#ifdef AGA
     case 0x10E: CLXCON2 (value); break;
#endif

     case 0x110: BPL1DAT (hpos, value); break;

     case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
     case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
     case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
     case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
     case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
     case 0x1BC: case 0x1BE:
	COLOR_WRITE (hpos, value & 0xFFF, (addr & 0x3E) / 2);
	break;
     case 0x120: case 0x124: case 0x128: case 0x12C:
     case 0x130: case 0x134: case 0x138: case 0x13C:
	SPRxPTH (hpos, value, (addr - 0x120) / 4);
	break;
     case 0x122: case 0x126: case 0x12A: case 0x12E:
     case 0x132: case 0x136: case 0x13A: case 0x13E:
	SPRxPTL (hpos, value, (addr - 0x122) / 4);
	break;
     case 0x140: case 0x148: case 0x150: case 0x158:
     case 0x160: case 0x168: case 0x170: case 0x178:
	SPRxPOS (hpos, value, (addr - 0x140) / 8);
	break;
     case 0x142: case 0x14A: case 0x152: case 0x15A:
     case 0x162: case 0x16A: case 0x172: case 0x17A:
	SPRxCTL (hpos, value, (addr - 0x142) / 8);
	break;
     case 0x144: case 0x14C: case 0x154: case 0x15C:
     case 0x164: case 0x16C: case 0x174: case 0x17C:
	SPRxDATA (hpos, value, (addr - 0x144) / 8);
	break;
     case 0x146: case 0x14E: case 0x156: case 0x15E:
     case 0x166: case 0x16E: case 0x176: case 0x17E:
	SPRxDATB (hpos, value, (addr - 0x146) / 8);
	break;

     case 0x36: JOYTEST (value); break;
     case 0x5A: BLTCON0L (value); break;
     case 0x5C: BLTSIZV (value); break;
     case 0x5E: BLTSIZH (value); break;
     case 0x1E4: DIWHIGH (hpos, value); break;
#ifdef AGA
     case 0x10C: BPLCON4 (hpos, value); break;
#endif

#ifndef CUSTOM_SIMPLE
     case 0x1DC: BEAMCON0 (value); break;
     case 0x1C0: if (htotal != value) { htotal = value; varsync (); } break;
     case 0x1C2: if (hsstop != value) { hsstop = value; varsync (); } break;
     case 0x1C4: if (hbstrt != value) { hbstrt = value; varsync (); } break;
     case 0x1C6: if (hbstop != value) { hbstop = value; varsync (); } break;
     case 0x1C8: if (vtotal != value) { vtotal = value; varsync (); } break;
     case 0x1CA: if (vsstop != value) { vsstop = value; varsync (); } break;
     case 0x1CC: if (vbstrt < value || vbstrt > value + 1) { vbstrt = value; varsync (); } break;
     case 0x1CE: if (vbstop < value || vbstop > value + 1) { vbstop = value; varsync (); } break;
     case 0x1DE: if (hsstrt != value) { hsstrt = value; varsync (); } break;
     case 0x1E0: if (vsstrt != value) { vsstrt = value; varsync (); } break;
     case 0x1E2: if (hcenter != value) { hcenter = value; varsync (); } break;
#endif

#ifdef AGA
     case 0x1FC: FMODE (value); break;
#endif

     /* writing to read-only register causes read access */
     default:
	if (!noget)
	    custom_wget_1 (addr, 1);
	if (!(currprefs.chipset_mask & CSMASK_AGA) && (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	    last_custom_value = 0xffff;
	return 1;
    }
    return 0;
}

void REGPARAM2 custom_wput (uaecptr addr, uae_u32 value)
{
    unsigned int hpos = current_hpos ();
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
#ifdef CUSTOM_DEBUG
    write_log ("%d:%d:wput: %04.4X %04.4X pc=%p\n", hpos, vpos, addr & 0x01fe, value & 0xffff, m68k_getpc (&regs));
#endif
    sync_copper_with_cpu (hpos, 1);
    custom_wput_1 (hpos, addr, value, 0);
}

void REGPARAM2 custom_bput (uaecptr addr, uae_u32 value)
{
    static int warned;

    uae_u16 rval = (value << 8) | (value & 0xFF);
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    custom_wput (addr & ~1, rval);
    if (warned < 10) {
	if (m68k_getpc (&regs) < 0xe00000 || m68k_getpc (&regs) >= 0x10000000) {
	    write_log ("Byte put to custom register %04.4X PC=%08.8X\n", addr, m68k_getpc (&regs));
	    warned++;
	}
    }
}

void REGPARAM2 custom_lput(uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= SPECIAL_MEM_WRITE;
#endif
    custom_wput (addr & 0xfffe, value >> 16);
    custom_wput ((addr + 2) & 0xfffe, (uae_u16)value);
}



#ifdef SAVESTATE

void custom_prepare_savestate (void)
{
}

#define RB restore_u8 ()
#define RW restore_u16 ()
#define RL restore_u32 ()

const uae_u8 *restore_custom (const uae_u8 *src)
{
    uae_u16 dsklen, dskbytr;
    int dskpt;
    unsigned int i;

    audio_reset ();

    changed_prefs.chipset_mask = currprefs.chipset_mask = RL;
    RW;				/* 000 ? */
    RW;				/* 002 DMACONR */
    RW;				/* 004 VPOSR */
    RW;				/* 006 VHPOSR */
    RW;				/* 008 DSKDATR (dummy register) */
    RW;				/* 00A JOY0DAT */
    RW;				/* 00C JOY1DAT */
    clxdat = RW;		/* 00E CLXDAT */
    RW;				/* 010 ADKCONR */
    RW;				/* 012 POT0DAT* */
    RW;				/* 014 POT1DAT* */
    RW;				/* 016 POTINP* */
    RW;				/* 018 SERDATR* */
    dskbytr = RW;		/* 01A DSKBYTR */
    RW;				/* 01C INTENAR */
    RW;				/* 01E INTREQR */
    dskpt = RL;			/* 020-022 DSKPT */
    dsklen = RW;		/* 024 DSKLEN */
    RW;				/* 026 DSKDAT */
    RW;				/* 028 REFPTR */
    lof = RW;			/* 02A VPOSW */
    RW;				/* 02C VHPOSW */
    COPCON(RW);			/* 02E COPCON */
    RW;				/* 030 SERDAT* */
    RW;				/* 032 SERPER* */
    POTGO(RW);			/* 034 POTGO */
    RW;				/* 036 JOYTEST* */
    RW;				/* 038 STREQU */
    RW;				/* 03A STRVHBL */
    RW;				/* 03C STRHOR */
    RW;				/* 03E STRLONG */
    BLTCON0(RW);		/* 040 BLTCON0 */
    BLTCON1(RW);		/* 042 BLTCON1 */
    BLTAFWM(RW);		/* 044 BLTAFWM */
    BLTALWM(RW);		/* 046 BLTALWM */
    BLTCPTH(RL);		/* 048-04B BLTCPT */
    BLTBPTH(RL);		/* 04C-04F BLTBPT */
    BLTAPTH(RL);		/* 050-053 BLTAPT */
    BLTDPTH(RL);		/* 054-057 BLTDPT */
    RW;				/* 058 BLTSIZE */
    RW;				/* 05A BLTCON0L */
    blt_info.vblitsize = RW;	/* 05C BLTSIZV */
    blt_info.hblitsize = RW;	/* 05E BLTSIZH */
    BLTCMOD(RW);		/* 060 BLTCMOD */
    BLTBMOD(RW);		/* 062 BLTBMOD */
    BLTAMOD(RW);		/* 064 BLTAMOD */
    BLTDMOD(RW);		/* 066 BLTDMOD */
    RW;				/* 068 ? */
    RW;				/* 06A ? */
    RW;				/* 06C ? */
    RW;				/* 06E ? */
    BLTCDAT(RW);		/* 070 BLTCDAT */
    BLTBDAT(RW);		/* 072 BLTBDAT */
    BLTADAT(RW);		/* 074 BLTADAT */
    RW;				/* 076 ? */
    RW;				/* 078 ? */
    RW;				/* 07A ? */
    RW;				/* 07C LISAID */
    DSKSYNC(-1, RW);		/* 07E DSKSYNC */
    cop1lc = RL;		/* 080/082 COP1LC */
    cop2lc = RL;		/* 084/086 COP2LC */
    RW;				/* 088 ? */
    RW;				/* 08A ? */
    RW;				/* 08C ? */
    diwstrt = RW;		/* 08E DIWSTRT */
    diwstop = RW;		/* 090 DIWSTOP */
    ddfstrt = RW;		/* 092 DDFSTRT */
    ddfstop = RW;		/* 094 DDFSTOP */
    dmacon = RW & ~(0x2000|0x4000); /* 096 DMACON */
    CLXCON(RW);			/* 098 CLXCON */
    intena = RW;		/* 09A INTENA */
    intreq = RW;		/* 09C INTREQ */
    adkcon = RW;		/* 09E ADKCON */
    for (i = 0; i < 8; i++)
	bplpt[i] = RL;
    bplcon0 = RW;		/* 100 BPLCON0 */
    bplcon1 = RW;		/* 102 BPLCON1 */
    bplcon2 = RW;		/* 104 BPLCON2 */
    bplcon3 = RW;		/* 106 BPLCON3 */
    bpl1mod = RW;		/* 108 BPL1MOD */
    bpl2mod = RW;		/* 10A BPL2MOD */
    bplcon4 = RW;		/* 10C BPLCON4 */
    clxcon2 = RW;		/* 10E CLXCON2* */
    for(i = 0; i < 8; i++)
	RW;			/*     BPLXDAT */
    for(i = 0; i < 32; i++)
	current_colors.color_regs_ecs[i] = RW; /* 180 COLORxx */
    htotal = RW;		/* 1C0 HTOTAL */
    hsstop = RW;		/* 1C2 HSTOP ? */
    hbstrt = RW;		/* 1C4 HBSTRT ? */
    hbstop = RW;		/* 1C6 HBSTOP ? */
    vtotal = RW;		/* 1C8 VTOTAL */
    vsstop = RW;		/* 1CA VSSTOP */
    vbstrt = RW;		/* 1CC VBSTRT */
    vbstop = RW;		/* 1CE VBSTOP */
    RW;				/* 1D0 ? */
    RW;				/* 1D2 ? */
    RW;				/* 1D4 ? */
    RW;				/* 1D6 ? */
    RW;				/* 1D8 ? */
    RW;				/* 1DA ? */
    new_beamcon0 = RW;		/* 1DC BEAMCON0 */
    hsstrt = RW;		/* 1DE HSSTRT */
    vsstrt = RW;		/* 1E0 VSSTT  */
    hcenter = RW;		/* 1E2 HCENTER */
    diwhigh = RW;		/* 1E4 DIWHIGH */
    if (diwhigh & 0x8000)
	diwhigh_written = 1;
    diwhigh &= 0x7fff;
    RW;				/* 1E6 ? */
    RW;				/* 1E8 ? */
    RW;				/* 1EA ? */
    RW;				/* 1EC ? */
    RW;				/* 1EE ? */
    RW;				/* 1F0 ? */
    RW;				/* 1F2 ? */
    RW;				/* 1F4 ? */
    RW;				/* 1F6 ? */
    RW;				/* 1F8 ? */
    RW;				/* 1FA ? */
    fmode = RW;			/* 1FC FMODE */
    last_custom_value = RW;	/* 1FE ? */

    DISK_restore_custom (dskpt, dsklen, dskbytr);

    return src;
}

#endif /* SAVESTATE */

#if defined SAVESTATE || defined DEBUGGER

#define SB save_u8
#define SW save_u16
#define SL save_u32

extern uae_u16 serper;

uae_u8 *save_custom (uae_u32 *len, uae_u8 *dstptr, int full)
{
    uae_u8 *dstbak, *dst;
    unsigned int i;
    uae_u32 dskpt;
    uae_u16 dsklen, dsksync, dskbytr;

    DISK_save_custom (&dskpt, &dsklen, &dsksync, &dskbytr);

    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = malloc (8 + 256 * 2);

    SL (currprefs.chipset_mask);
    SW (0);			/* 000 ? */
    SW (dmacon);		/* 002 DMACONR */
    SW (VPOSR());		/* 004 VPOSR */
    SW (VHPOSR());		/* 006 VHPOSR */
    SW (0);			/* 008 DSKDATR */
    SW (JOY0DAT());		/* 00A JOY0DAT */
    SW (JOY1DAT());		/* 00C JOY1DAT */
    SW (clxdat);		/* 00E CLXDAT */
    SW (ADKCONR());		/* 010 ADKCONR */
    SW (POT0DAT());		/* 012 POT0DAT */
    SW (POT0DAT());		/* 014 POT1DAT */
    SW (0)	;		/* 016 POTINP * */
    SW (0);			/* 018 SERDATR * */
    SW (dskbytr);		/* 01A DSKBYTR */
    SW (INTENAR());		/* 01C INTENAR */
    SW (INTREQR());		/* 01E INTREQR */
    SL (dskpt);			/* 020-023 DSKPT */
    SW (dsklen);		/* 024 DSKLEN */
    SW (0);			/* 026 DSKDAT */
    SW (0);			/* 028 REFPTR */
    SW (lof);			/* 02A VPOSW */
    SW (0);			/* 02C VHPOSW */
    SW (copcon);		/* 02E COPCON */
    SW (serper);		/* 030 SERDAT * */
    SW (serdat);		/* 032 SERPER * */
    SW (potgo_value);		/* 034 POTGO */
    SW (0);			/* 036 JOYTEST * */
    SW (0);			/* 038 STREQU */
    SW (0);			/* 03A STRVBL */
    SW (0);			/* 03C STRHOR */
    SW (0);			/* 03E STRLONG */
    SW (bltcon0);		/* 040 BLTCON0 */
    SW (bltcon1);		/* 042 BLTCON1 */
    SW (blt_info.bltafwm);	/* 044 BLTAFWM */
    SW (blt_info.bltalwm);	/* 046 BLTALWM */
    SL (bltcpt);		/* 048-04B BLTCPT */
    SL (bltbpt);		/* 04C-04F BLTCPT */
    SL (bltapt);		/* 050-053 BLTCPT */
    SL (bltdpt);		/* 054-057 BLTCPT */
    SW (0);			/* 058 BLTSIZE */
    SW (0);			/* 05A BLTCON0L (use BLTCON0 instead) */
    SW (blt_info.vblitsize);	/* 05C BLTSIZV */
    SW (blt_info.hblitsize);	/* 05E BLTSIZH */
    SW (blt_info.bltcmod);	/* 060 BLTCMOD */
    SW (blt_info.bltbmod);	/* 062 BLTBMOD */
    SW (blt_info.bltamod);	/* 064 BLTAMOD */
    SW (blt_info.bltdmod);	/* 066 BLTDMOD */
    SW (0);			/* 068 ? */
    SW (0);			/* 06A ? */
    SW (0);			/* 06C ? */
    SW (0);			/* 06E ? */
    SW (blt_info.bltcdat);	/* 070 BLTCDAT */
    SW (blt_info.bltbdat);	/* 072 BLTBDAT */
    SW (blt_info.bltadat);	/* 074 BLTADAT */
    SW (0);			/* 076 ? */
    SW (0);			/* 078 ? */
    SW (0);			/* 07A ? */
    SW (DENISEID());		/* 07C DENISEID/LISAID */
    SW (dsksync);		/* 07E DSKSYNC */
    SL (cop1lc);		/* 080-083 COP1LC */
    SL (cop2lc);		/* 084-087 COP2LC */
    SW (0);			/* 088 ? */
    SW (0);			/* 08A ? */
    SW (0);			/* 08C ? */
    SW (diwstrt);		/* 08E DIWSTRT */
    SW (diwstop);		/* 090 DIWSTOP */
    SW (ddfstrt);		/* 092 DDFSTRT */
    SW (ddfstop);		/* 094 DDFSTOP */
    SW (dmacon);		/* 096 DMACON */
    SW (clxcon);		/* 098 CLXCON */
    SW (intena);		/* 09A INTENA */
    SW (intreq);		/* 09C INTREQ */
    SW (adkcon);		/* 09E ADKCON */
    for (i = 0; full && i < 32; i++)
	SW (0);
    for (i = 0; i < 8; i++)
	SL (bplpt[i]);		/* 0E0-0FE BPLxPT */
    SW (bplcon0);		/* 100 BPLCON0 */
    SW (bplcon1);		/* 102 BPLCON1 */
    SW (bplcon2);		/* 104 BPLCON2 */
    SW (bplcon3);		/* 106 BPLCON3 */
    SW (bpl1mod);		/* 108 BPL1MOD */
    SW (bpl2mod);		/* 10A BPL2MOD */
    SW (bplcon4);		/* 10C BPLCON4 */
    SW (clxcon2);		/* 10E CLXCON2 */
    for (i = 0;i < 8; i++)
	SW (0);			/* 110 BPLxDAT */
    if (full) {
	for (i = 0; i < 8; i++) {
	    SL (spr[i].pt);	    /* 120-13E SPRxPT */
	    SW (sprpos[i]);	    /* 1x0 SPRxPOS */
	    SW (sprctl[i]);	    /* 1x2 SPRxPOS */
	    SW (sprdata[i][0]);	    /* 1x4 SPRxDATA */
	    SW (sprdatb[i][0]);	    /* 1x6 SPRxDATB */
	}
    }
    for ( i = 0; i < 32; i++)
	SW (current_colors.color_regs_ecs[i]); /* 180-1BE COLORxx */
    SW (htotal);		/* 1C0 HTOTAL */
    SW (hsstop);		/* 1C2 HSTOP*/
    SW (hbstrt);		/* 1C4 HBSTRT */
    SW (hbstop);		/* 1C6 HBSTOP */
    SW (vtotal);		/* 1C8 VTOTAL */
    SW (vsstop);		/* 1CA VSSTOP */
    SW (vbstrt);		/* 1CC VBSTRT */
    SW (vbstop);		/* 1CE VBSTOP */
    SW (0);			/* 1D0 */
    SW (0);			/* 1D2 */
    SW (0);			/* 1D4 */
    SW (0);			/* 1D6 */
    SW (0);			/* 1D8 */
    SW (0);			/* 1DA */
    SW (beamcon0);		/* 1DC BEAMCON0 */
    SW (hsstrt);		/* 1DE HSSTRT */
    SW (vsstrt);		/* 1E0 VSSTRT */
    SW (hcenter);		/* 1E2 HCENTER */
    SW (diwhigh | (diwhigh_written ? 0x8000 : 0)); /* 1E4 DIWHIGH */
    SW (0);			/* 1E6 */
    SW (0);			/* 1E8 */
    SW (0);			/* 1EA */
    SW (0);			/* 1EC */
    SW (0);			/* 1EE */
    SW (0);			/* 1F0 */
    SW (0);			/* 1F2 */
    SW (0);			/* 1F4 */
    SW (0);			/* 1F6 */
    SW (0);			/* 1F8 */
    SW (0);			/* 1FA */
    SW (fmode);			/* 1FC FMODE */
    SW (last_custom_value);	/* 1FE */

    *len = dst - dstbak;
    return dstbak;
}

#endif /* SAVESTATE || DEBUGGER */

#ifdef SAVESTATE

const uae_u8 *restore_custom_agacolors (const uae_u8 *src)
{
    unsigned int i;

    for (i = 0; i < 256; i++)
#ifdef AGA
	current_colors.color_regs_aga[i] = RL;
#else
	RL;
#endif
    return src;
}

uae_u8 *save_custom_agacolors (uae_u32 *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak, *dst;
    unsigned int i;

    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = malloc (256 * 4);
    for (i = 0; i < 256; i++)
#ifdef AGA
    SL (current_colors.color_regs_aga[i]);
#else
    SL (0);
#endif
    *len = dst - dstbak;
    return dstbak;
}

const uae_u8 *restore_custom_sprite (unsigned int num, const uae_u8 *src)
{
    spr[num].pt = RL;		/* 120-13E SPRxPT */
    sprpos[num] = RW;		/* 1x0 SPRxPOS */
    sprctl[num] = RW;		/* 1x2 SPRxPOS */
    sprdata[num][0] = RW;	/* 1x4 SPRxDATA */
    sprdatb[num][0] = RW;	/* 1x6 SPRxDATB */
    sprdata[num][1] = RW;
    sprdatb[num][1] = RW;
    sprdata[num][2] = RW;
    sprdatb[num][2] = RW;
    sprdata[num][3] = RW;
    sprdatb[num][3] = RW;
    spr[num].armed = RB;
    return src;
}

uae_u8 *save_custom_sprite (unsigned int num, uae_u32 *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak, *dst;

    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = malloc (25);
    SL (spr[num].pt);		/* 120-13E SPRxPT */
    SW (sprpos[num]);		/* 1x0 SPRxPOS */
    SW (sprctl[num]);		/* 1x2 SPRxPOS */
    SW (sprdata[num][0]);	/* 1x4 SPRxDATA */
    SW (sprdatb[num][0]);	/* 1x6 SPRxDATB */
    SW (sprdata[num][1]);
    SW (sprdatb[num][1]);
    SW (sprdata[num][2]);
    SW (sprdatb[num][2]);
    SW (sprdata[num][3]);
    SW (sprdatb[num][3]);
    SB (spr[num].armed ? 1 : 0);
    *len = dst - dstbak;
    return dstbak;
}

#endif /* SAVESTATE */


void check_prefs_changed_custom (void)
{
    currprefs.gfx_framerate = changed_prefs.gfx_framerate;
    if (inputdevice_config_change_test ()) {
	inputdevice_copyconfig (&changed_prefs, &currprefs);
	inputdevice_updateconfig (&currprefs);
    }
    currprefs.immediate_blits = changed_prefs.immediate_blits;
    currprefs.collision_level = changed_prefs.collision_level;

    if (currprefs.chipset_mask != changed_prefs.chipset_mask ||
	currprefs.gfx_vsync != changed_prefs.gfx_vsync ||
	currprefs.ntscmode != changed_prefs.ntscmode) {
	currprefs.gfx_vsync = changed_prefs.gfx_vsync;
	currprefs.chipset_mask = changed_prefs.chipset_mask;
	if (currprefs.ntscmode != changed_prefs.ntscmode) {
	    currprefs.ntscmode = changed_prefs.ntscmode;
	    new_beamcon0 = currprefs.ntscmode ? 0x00 : 0x20;
	}
	init_custom ();
    }
#ifdef GFXFILTER
    currprefs.gfx_filter_horiz_zoom = changed_prefs.gfx_filter_horiz_zoom;
    currprefs.gfx_filter_vert_zoom = changed_prefs.gfx_filter_vert_zoom;
    currprefs.gfx_filter_horiz_offset = changed_prefs.gfx_filter_horiz_offset;
    currprefs.gfx_filter_vert_offset = changed_prefs.gfx_filter_vert_offset;
    currprefs.gfx_filter_scanlines = changed_prefs.gfx_filter_scanlines;
    currprefs.gfx_filter_filtermode = changed_prefs.gfx_filter_filtermode;
#endif
}

#ifdef CPUEMU_6

STATIC_INLINE void sync_copper (unsigned int hpos)
{
    if (eventtab[ev_copper].active) {
	eventtab[ev_copper].active = 0;
	update_copper (hpos);
	return;
    }
    if (copper_enabled_thisline)
	update_copper (hpos);
}

STATIC_INLINE void decide_fetch_ce (unsigned int hpos)
{
    if ((ddf_change == vpos || ddf_change + 1 == vpos) && vpos < maxvpos)
	decide_fetch (hpos);
}

STATIC_INLINE void dma_cycle (void)
{
    unsigned int hpos;
    static int bnasty;

    for (;;) {
	int bpldma;
	int blitpri = dmaen (DMA_BLITPRI);
	do_cycles (1 * CYCLE_UNIT);
	hpos = current_hpos ();
	sync_copper (hpos);
	decide_line (hpos);
	decide_fetch_ce (hpos);
	bpldma = is_bitplane_dma (hpos);
	if (bltstate != BLT_done) {
	    if (!blitpri && bnasty >= 3 && !cycle_line[hpos] && !bpldma) {
		bnasty = 0;
		break;
	    }
	    decide_blitter (hpos);
	    if (dmaen(DMA_BLITTER))
		bnasty++;
	}
	if (cycle_line[hpos] == 0 && !bpldma)
	    break;
	/* bus was allocated to dma channel, wait for next cycle.. */
    }
    bnasty = 0;
    cycle_line[hpos] |= CYCLE_CPU;
}

uae_u32 wait_cpu_cycle_read (uaecptr addr, int mode)
{
    uae_u32 v = 0;
    dma_cycle ();
    if (mode > 0)
	v = get_word (addr);
    else if (mode == 0)
	v = get_byte (addr);
    do_cycles (1 * CYCLE_UNIT);
    return v;
}

void wait_cpu_cycle_write (uaecptr addr, int mode, uae_u32 v)
{
    dma_cycle ();
    if (mode > 0)
	put_word (addr, v);
    else if (mode == 0)
	put_byte (addr, v);
    do_cycles (1 * CYCLE_UNIT);
}

void do_cycles_ce (long cycles)
{
    unsigned int hpos;
    while (cycles > 0) {
	do_cycles (1 * CYCLE_UNIT);
	cycles -= CYCLE_UNIT;
	hpos = current_hpos ();
	sync_copper (hpos);
	decide_line (hpos);
	decide_fetch_ce (hpos);
	decide_blitter (hpos);
    }
}

#endif

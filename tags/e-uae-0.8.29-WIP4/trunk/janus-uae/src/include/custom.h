 /*
  * UAE - The Un*x Amiga Emulator
  *
  * custom chip support
  *
  * (c) 1995 Bernd Schmidt
  */

#include "machdep/rpt.h"

/* These are the masks that are ORed together in the chipset_mask option.
 * If CSMASK_AGA is set, the ECS bits are guaranteed to be set as well.  */
#define CSMASK_ECS_AGNUS 1
#define CSMASK_ECS_DENISE 2
#define CSMASK_AGA 4

uae_u32 get_copper_address(int copno);

extern int custom_init (void);
extern void customreset (void);
extern int intlev (void);
extern void dumpcustom (void);

extern void do_disk (void);
extern void do_copper (void);

extern void notice_new_xcolors (void);
extern void notice_screen_contents_lost (void);
extern void init_row_map (void);
extern void init_hz (void);
extern void init_custom (void);

extern int picasso_requested_on;
extern int picasso_on;
extern int turbo_emulation;

/* Set to 1 to leave out the current frame in average frame time calculation.
 * Useful if the debugger was active.  */
extern int bogusframe;
extern unsigned long hsync_counter;

extern uae_u16 dmacon;
extern uae_u16 intena,intreq;

//extern int current_hpos (void);
extern unsigned int vpos;

extern int n_frames;

int is_bitplane_dma (unsigned int hpos);

STATIC_INLINE int dmaen (unsigned int dmamask)
{
    return (dmamask & dmacon) && (dmacon & 0x200);
}

#define SPCFLAG_STOP 2
#define SPCFLAG_COPPER 4
#define SPCFLAG_INT 8
#define SPCFLAG_BRK 16
#define SPCFLAG_EXTRA_CYCLES 32
#define SPCFLAG_TRACE 64
#define SPCFLAG_DOTRACE 128
#define SPCFLAG_DOINT 256 /* arg, JIT fails without this.. */
#define SPCFLAG_BLTNASTY 512
#define SPCFLAG_EXEC 1024
#define SPCFLAG_ACTION_REPLAY 2048
#define SPCFLAG_TRAP 4096 /* enforcer-hack */
#define SPCFLAG_MODE_CHANGE 8192
#define SPCFLAG_END_COMPILE 16384

extern uae_u16 adkcon;

extern unsigned int joy0dir, joy1dir;
extern int joy0button, joy1button;

extern void INTREQ (uae_u16);
extern void INTREQ_0 (uae_u16);
extern uae_u16 INTREQR (void);

/* maximums for statically allocated tables */

#define MAXHPOS 256
#define MAXVPOS 576

/* PAL/NTSC values */

#define MAXHPOS_PAL 227
#define MAXHPOS_NTSC 227
#define MAXVPOS_PAL 312
#define MAXVPOS_NTSC 262
#define VBLANK_ENDLINE_PAL 27
#define VBLANK_ENDLINE_NTSC 28
#define VBLANK_SPRITE_PAL 25
#define VBLANK_SPRITE_NTSC 20
#define VBLANK_HZ_PAL 50
#define VBLANK_HZ_NTSC 60

extern unsigned int maxhpos;
extern unsigned int maxvpos;
extern unsigned int minfirstline;
extern int vblank_hz, fake_vblank_hz, vblank_skip;
extern frame_time_t syncbase;
#define NUMSCRLINES (maxvpos+1-minfirstline+1)

#define DMA_AUD0      0x0001
#define DMA_AUD1      0x0002
#define DMA_AUD2      0x0004
#define DMA_AUD3      0x0008
#define DMA_DISK      0x0010
#define DMA_SPRITE    0x0020
#define DMA_BLITTER   0x0040
#define DMA_COPPER    0x0080
#define DMA_BITPLANE  0x0100
#define DMA_MASTER    0x0200
#define DMA_BLITPRI   0x0400

#define CYCLE_REFRESH	0x01
#define CYCLE_MISC	0x02
#define CYCLE_SPRITE	0x04
#define CYCLE_BITPLANE	0x08
#define CYCLE_COPPER	0x10
#define CYCLE_BLITTER	0x20
#define CYCLE_CPU	0x40
#define CYCLE_NOCPU	0x80

extern unsigned int frametime;
extern unsigned int timeframes;
extern unsigned int plfstrt;
extern unsigned int plfstop;
extern unsigned int plffirstline, plflastline;
extern uae_u16 htotal, vtotal;

/* 100 words give you 1600 horizontal pixels. Should be more than enough for
 * superhires. Don't forget to update the definition in genp2c.c as well.
 * needs to be larger for superhires support */
#ifdef CUSTOM_SIMPLE
#define MAX_WORDS_PER_LINE 50
#else
#define MAX_WORDS_PER_LINE 100
#endif

#ifdef AGA
/* AGA mode color lookup tables */
extern unsigned int xredcolors[256], xgreencolors[256], xbluecolors[256];
#endif

extern int bpl_off[8];

#define RES_LORES 0
#define RES_HIRES 1
#define RES_SUPERHIRES 2

/* calculate shift depending on resolution (replaced "decided_hires ? 4 : 8") */
#define RES_SHIFT(res) ((res) == RES_LORES ? 8 : (res) == RES_HIRES ? 4 : 2)

/* get resolution from bplcon0 */
STATIC_INLINE int GET_RES (uae_u16 con0)
{
    int res = ((con0) & 0x8000) ? RES_HIRES : ((con0) & 0x40) ? RES_SUPERHIRES : RES_LORES;
    return res;
}
/* get sprite width from FMODE */
#define GET_SPRITEWIDTH(FMODE) ((((FMODE) >> 2) & 3) == 3 ? 64 : (((FMODE) >> 2) & 3) == 0 ? 16 : 32)
/* Compute the number of bitplanes from a value written to BPLCON0  */
#define GET_PLANES(x) ((((x) >> 12) & 7) | (((x) & 0x10) >> 1))

extern void fpscounter_reset (void);
extern frame_time_t idletime;
extern int lightpen_x, lightpen_y, lightpen_cx, lightpen_cy;

struct customhack {
    uae_u16 v;
    int vpos, hpos;
};
void customhack_put (struct customhack *ch, uae_u16 v, int hpos);
uae_u16 customhack_get (struct customhack *ch, int hpos);

extern void misc_hsync_stuff (void);

extern void hsync_handler (void);
extern void copper_handler (void);
#define HSYNCTIME (maxhpos * CYCLE_UNIT)

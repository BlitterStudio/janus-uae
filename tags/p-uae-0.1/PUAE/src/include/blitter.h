 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Blitter emulation
  *
  * (c) 1995 Bernd Schmidt
  */

struct bltinfo {
    int blitzero;
    int blitashift, blitbshift, blitdownashift, blitdownbshift;
    uae_u16 bltadat, bltbdat, bltcdat, bltddat;
    uae_u16 bltahold, bltbhold, bltafwm, bltalwm;
    int vblitsize, hblitsize;
    int bltamod, bltbmod, bltcmod, bltdmod;
    int got_cycle;
};

extern enum blitter_states {
    BLT_done, BLT_init, BLT_read, BLT_work, BLT_write, BLT_next
} bltstate;

extern struct bltinfo blt_info;

extern int blitter_nasty, blit_interrupt;

extern uae_u16 bltsize;
extern uae_u16 bltcon0, bltcon1;
extern uae_u32 bltapt, bltbpt, bltcpt, bltdpt;
extern int blit_singlechannel;

extern void maybe_blit (int, int);
extern void reset_blit (int);
extern int blitnasty (void);
extern int blitnnasty (int);
extern void blitter_handler (uae_u32);
extern void build_blitfilltable (void);
extern void do_blitter (int, int);
extern void decide_blitter (int hpos);
extern int blitter_need (int hpos);
extern void blitter_done_notify (int hpos);
extern void blitter_slowdown (int, int, int, int);
extern int blitter_channel_state (void);
extern void blitter_check_start (void);

typedef void blitter_func(uaecptr, uaecptr, uaecptr, uaecptr, struct bltinfo *);

#define BLITTER_MAX_WORDS 2048

extern blitter_func * const blitfunc_dofast[256];
extern blitter_func * const blitfunc_dofast_desc[256];
extern uae_u32 blit_masktable[BLITTER_MAX_WORDS];

#define BLIT_MODE_IMMEDIATE -1
#define BLIT_MODE_APPROXIMATE 0
#define BLIT_MODE_COMPATIBLE 1
#define BLIT_MODE_EXACT 2

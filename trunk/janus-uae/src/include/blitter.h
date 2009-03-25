 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Blitter emulation
  *
  * (c) 1995 Bernd Schmidt
  */

struct bltinfo {
    int blitzero;
    int blitashift,blitbshift,blitdownashift,blitdownbshift;
    uae_u16 bltadat, bltbdat, bltcdat,bltddat,bltahold,bltbhold,bltafwm,bltalwm;
    int vblitsize,hblitsize;
    int bltamod,bltbmod,bltcmod,bltdmod;
};

extern enum blitter_states {
    BLT_done, BLT_init, BLT_read, BLT_work, BLT_write, BLT_next
} bltstate;

extern struct bltinfo blt_info;

extern uae_u16 bltsize;
extern uae_u16 bltcon0,bltcon1;
extern unsigned int blinea_shift;
extern uae_u32 bltapt,bltbpt,bltcpt,bltdpt;
extern int blit_singlechannel;

extern void maybe_blit (unsigned int, int);
extern void reset_blit (unsigned int);
extern unsigned int blitnasty (void);
extern void blitter_handler (void);
extern void build_blitfilltable (void);
extern void do_blitter (unsigned int hpos);
extern void decide_blitter (unsigned int hpos);
extern void blitter_done_notify (void);
extern void blitter_slowdown (int, int, unsigned int, unsigned int);

typedef void blitter_func(uaecptr, uaecptr, uaecptr, uaecptr, struct bltinfo *);

#define BLITTER_MAX_WORDS 2048

extern blitter_func * const blitfunc_dofast[256];
extern blitter_func * const blitfunc_dofast_desc[256];
extern uae_u32 blit_masktable[BLITTER_MAX_WORDS];

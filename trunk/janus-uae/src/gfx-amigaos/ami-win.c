/************************************************************************ 
 *
 * Amiga interface
 *
 * Copyright 1996,1997,1998 Samuel Devulder.
 * Copyright 2003-2007 Richard Drummond
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/

#include <gtk/gtk.h>
#include "sysconfig.h"
#include "sysdeps.h"

#include "td-amigaos/thread.h"


/* sam: Argg!! Why did phase5 change the path to cybergraphics ? */
//#define CGX_CGX_H <cybergraphics/cybergraphics.h>

#ifdef HAVE_LIBRARIES_CYBERGRAPHICS_H
# define CGX_CGX_H <libraries/cybergraphics.h>
# define USE_CYBERGFX           /* define this to have cybergraphics support */
#else
# ifdef HAVE_CYBERGRAPHX_CYBERGRAPHICS_H
#  define USE_CYBERGFX
#  define CGX_CGX_H <cybergraphx/cybergraphics.h>
# endif
#endif
#ifdef USE_CYBERGFX
# if defined __MORPHOS__ || defined __AROS__ || defined __amigaos4__
#  define USE_CYBERGFX_V41
# endif
#endif

#define AWTRACING_ENABLED 0
#if AWTRACING_ENABLED
#define AWTRACE(...)	do { kprintf("%s:%d %s(): ",__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define AWTRACE(...)    do { ; } while(0)
#endif

#ifdef __AROS__
#define GTKMUI
#endif
//#define DEBUG


/****************************************************************************/

#include <exec/execbase.h>
#include <exec/memory.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

/* o1i added: */
#include <graphics/gfx.h>

#include <graphics/gfxbase.h>
#include <graphics/displayinfo.h>

#include <libraries/asl.h>
#include <intuition/pointerclass.h>

/****************************************************************************/

# ifdef __amigaos4__
#  define __USE_BASETYPE__
# endif
# include <proto/intuition.h>
# include <proto/graphics.h>
# include <proto/layers.h>
# include <proto/exec.h>
# include <proto/dos.h>
# include <proto/asl.h>

#ifdef USE_CYBERGFX
# ifdef __SASC
#  include CGX_CGX_H
#  include <proto/cybergraphics.h>
# else /* not SAS/C => gcc */
#  include CGX_CGX_H
#  include <proto/cybergraphics.h>
# endif
# ifndef BMF_SPECIALFMT
#  define BMF_SPECIALFMT 0x80	/* should be cybergraphics.h but isn't for  */
				/* some strange reason */
# endif
#  ifndef RECTFMT_RAW
#   define RECTFMT_RAW     5
#  endif
#endif /* USE_CYBERGFX */

/****************************************************************************/

#include <ctype.h>
#include <signal.h>

/****************************************************************************/

#include "uae.h"
#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include "debug.h"
#include "hotkeys.h"
#include "version.h"

#define BitMap Picasso96BitMap  /* Argh! */
#include "picasso96.h"
#undef BitMap

#include "od-amiga/j.h"

/* uae includes are a mess..*/
void uae_Signal(uaecptr task, uae_u32 mask);
/****************************************************************************/
extern struct  smp_comm_pipe native2amiga_pending;
extern volatile int uae_int_requested;

extern struct uae_sem_t n2asem;
/****************************************************************************/



#define UAEIFF "UAEIFF"        /* env: var to trigger iff dump */
#define UAESM  "UAESM"         /* env: var for screen mode */

static int need_dither;        /* well.. guess :-) */
static int use_delta_buffer;   /* this will redraw only needed places */
static int use_cyb;            /* this is for cybergfx truecolor mode */
static int use_approx_color;

extern xcolnr xcolors[4096];

static uae_u8 *oldpixbuf;

/* Values for amiga_screen_type */
enum {
    UAESCREENTYPE_CUSTOM,
    UAESCREENTYPE_PUBLIC,
    UAESCREENTYPE_ASK,
    UAESCREENTYPE_LAST
};

static int  screen_is_picasso;
static char picasso_invalid_lines[1200];
static int  picasso_invalid_start;
static int  picasso_invalid_end;

/* original values in case uae_main window is "hidden" */
static WORD uae_main_window_Width, uae_main_window_Height;
static WORD uae_main_window_Left,  uae_main_window_Top;

/****************************************************************************/
/*
 * prototypes & global vars
 */

struct IntuitionBase    *IntuitionBase = NULL;
struct DosLibrary       *DOSBase = NULL;
struct GfxBase          *GfxBase = NULL;
struct Library          *LayersBase = NULL;
struct Library          *AslBase = NULL;
struct Library          *CyberGfxBase = NULL;
struct Library          *GadToolsBase = NULL;

struct AslIFace *IAsl;
struct GraphicsIFace *IGraphics;
struct LayersIFace *ILayers;
struct IntuitionIFace *IIntuition;
struct CyberGfxIFace *ICyberGfx;

unsigned long            frame_num; /* for arexx */

static UBYTE            *Line;
struct RastPort  *RP;
struct Screen    *S;
struct Window    *W;
static struct RastPort  *TempRPort;
static struct BitMap    *BitMap;
#ifdef USE_CYBERGFX
# ifdef USE_CYBERGFX_V41
static uae_u8 *CybBuffer;
# else
static struct BitMap    *CybBitMap;
# endif
#endif
struct ColorMap  *CM;
int              XOffset,YOffset;

static int os39;        /* kick 39 present */
static int usepub;      /* use public screen */
static int is_halfbrite;
static int is_ham;

static int   get_color_failed;
static int   maxpen;
static UBYTE pen[256];

#ifdef __amigaos4__ 
static int mouseGrabbed;
static int grabTicks;
#define GRAB_TIMEOUT 50
#endif

#ifdef PICASSO96
/*static*/ APTR picasso_memory;
#endif

static struct BitMap *myAllocBitMap(ULONG,ULONG,ULONG,ULONG,struct BitMap *);
static void set_title(void);
static void myFreeBitMap(struct BitMap *);
static LONG ObtainColor(ULONG, ULONG, ULONG);
static void ReleaseColors(void);
static int  DoSizeWindow(struct Window *,int,int);
static int  init_ham(void);
static void ham_conv(UWORD *src, UBYTE *buf, UWORD len);
static int  RPDepth(struct RastPort *RP);
static int get_nearest_color(int r, int g, int b);

static int get_BytesPerRow(struct Window *win);

/****************************************************************************/

void main_window_led(int led, int on);
int do_inhibit_frame(int onoff);

extern void initpseudodevices(void);
extern void closepseudodevices(void);
extern void appw_init(struct Window *W);
extern void appw_exit(void);
extern void appw_events(void);

extern int ievent_alive;

/***************************************************************************
 *
 * Default hotkeys
 *
 * We need a better way of doing this. ;-)
 */
static struct uae_hotkeyseq ami_hotkeys[] =
{
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_Q, -1,      INPUTEVENT_SPC_QUIT) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_R, -1,      INPUTEVENT_SPC_SOFTRESET) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_R,  INPUTEVENT_SPC_HARDRESET) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_D, -1,      INPUTEVENT_SPC_ENTERDEBUGGER) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_S, -1,      INPUTEVENT_SPC_TOGGLEFULLSCREEN) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_G, -1,      INPUTEVENT_SPC_TOGGLEMOUSEGRAB) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_I, -1,      INPUTEVENT_SPC_INHIBITSCREEN) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_P, -1,      INPUTEVENT_SPC_SCREENSHOT) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_A, -1,      INPUTEVENT_SPC_SWITCHINTERPOL) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NPADD, -1,  INPUTEVENT_SPC_INCRFRAMERATE) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_NPSUB, -1,  INPUTEVENT_SPC_DECRFRAMERATE) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F1, -1,     INPUTEVENT_SPC_FLOPPY0) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F2, -1,     INPUTEVENT_SPC_FLOPPY1) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F3, -1,     INPUTEVENT_SPC_FLOPPY2) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F4, -1,     INPUTEVENT_SPC_FLOPPY3) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F1, INPUTEVENT_SPC_EFLOPPY0) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F2, INPUTEVENT_SPC_EFLOPPY1) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F3, INPUTEVENT_SPC_EFLOPPY2) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_LSH, AK_F4, INPUTEVENT_SPC_EFLOPPY3) },
    { MAKE_HOTKEYSEQ (AK_CTRL, AK_LALT, AK_F, -1,      INPUTEVENT_SPC_FREEZEBUTTON) },
    { HOTKEYS_END }
};

/****************************************************************************/

extern UBYTE cidx[4][8*4096];


/*
 * Dummy buffer locking methods
 */
static int dummy_lock (struct vidbuf_description *gfxinfo)
{
    return 1;
}

static void dummy_unlock (struct vidbuf_description *gfxinfo)
{
}

static void dummy_flush_screen (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
}


/*
 * Buffer methods for planar screens with no dithering.
 *
 * This uses a delta buffer to reduce the overhead of doing the chunky-to-planar
 * conversion required.
 */
STATIC_INLINE void flush_line_planar_nodither (struct vidbuf_description *gfxinfo, int line_no)
{
    int     xs      = 0;
    int     len     = gfxinfo->width;
    int     yoffset = line_no * gfxinfo->rowbytes;
    uae_u8 *src;
    uae_u8 *dst;
    uae_u8 *newp = gfxinfo->bufmem + yoffset;
    uae_u8 *oldp = oldpixbuf + yoffset;

    /* Find first pixel changed on this line */
    while (*newp++ == *oldp++) {
	if (!--len)
	    return; /* line not changed - so don't draw it */
    }
    src   = --newp;
    dst   = --oldp;
    newp += len;
    oldp += len;

    /* Find last pixel changed on this line */
    while (*--newp == *--oldp)
	;

    len = 1 + (oldp - dst);
    xs  = src - (uae_u8 *)(gfxinfo->bufmem + yoffset);

    /* Copy changed pixels to delta buffer */
    CopyMem (src, dst, len);

    /* Blit changed pixels to the display */
    WritePixelLine8 (RP, xs + XOffset, line_no + YOffset, len, dst, TempRPort);
}

static void flush_block_planar_nodither (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    int line_no;

    for (line_no = first_line; line_no <= last_line; line_no++)
	flush_line_planar_nodither (gfxinfo, line_no);
}

/*
 * Buffer methods for planar screens with dithering.
 *
 * This uses a delta buffer to reduce the overhead of doing the chunky-to-planar
 * conversion required.
 */
STATIC_INLINE void flush_line_planar_dither (struct vidbuf_description *gfxinfo, int line_no)
{
    int      xs      = 0;
    int      len     = gfxinfo->width;
    int      yoffset = line_no * gfxinfo->rowbytes;
    uae_u16 *src;
    uae_u16 *dst;
    uae_u16 *newp = (uae_u16 *)(gfxinfo->bufmem + yoffset);
    uae_u16 *oldp = (uae_u16 *)(oldpixbuf + yoffset);

    /* Find first pixel changed on this line */
    while (*newp++ == *oldp++) {
	if (!--len)
	    return; /* line not changed - so don't draw it */
    }
    src   = --newp;
    dst   = --oldp;
    newp += len;
    oldp += len;

    /* Find last pixel changed on this line */
    while (*--newp == *--oldp)
	;

    len = (1 + (oldp - dst));
    xs  = src - (uae_u16 *)(gfxinfo->bufmem + yoffset);

    /* Copy changed pixels to delta buffer */
    CopyMem (src, dst, len * 2);

    /* Dither changed pixels to Line buffer */
    DitherLine (Line, src, xs, line_no, (len + 3) & ~3, 8);

    /* Blit dithered pixels from Line buffer to the display */
    WritePixelLine8 (RP, xs + XOffset, line_no + YOffset, len, Line, TempRPort);
}

static void flush_block_planar_dither (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    int line_no;

    for (line_no = first_line; line_no <= last_line; line_no++)
	flush_line_planar_dither (gfxinfo, line_no);
}

/*
 * Buffer methods for HAM screens.
 */
STATIC_INLINE void flush_line_ham (struct vidbuf_description *gfxinfo, int line_no)
{
    int     len = gfxinfo->width;
    uae_u8 *src = gfxinfo->bufmem + (line_no * gfxinfo->rowbytes);

    ham_conv ((void*) src, Line, len);
    WritePixelLine8 (RP, 0, line_no, len, Line, TempRPort);

    return;
}

static void flush_block_ham (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    int line_no;

    for (line_no = first_line; line_no <= last_line; line_no++)
	flush_line_ham (gfxinfo, line_no);
}

#ifdef USE_CYBERGFX
# ifndef USE_CYBERGFX_V41
static void flush_line_cgx (struct vidbuf_description *gfxinfo, int line_no)
{
    BltBitMapRastPort (CybBitMap,
		       0, line_no,
		       RP,
		       XOffset,
		       YOffset + line_no,
		       gfxinfo->width,
		       1,
		       0xc0);
}

static void flush_block_cgx (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    BltBitMapRastPort (CybBitMap,
		       0, first_line,
		       RP,
		       XOffset,
		       YOffset + first_line,
		       gfxinfo->width,
		       last_line - first_line + 1,
		       0xc0);
}
# else
static void flush_line_cgx_v41 (struct vidbuf_description *gfxinfo, int line_no)
{
  /* DONE! this for cloned windows!! */
    WritePixelArray (CybBuffer,
		     0 , line_no,
		     gfxinfo->rowbytes,
		     RP,
		     XOffset,
		     YOffset + line_no,
		     gfxinfo->width,
		     1,
		     RECTFMT_RAW);
    AWTRACE("flush_line_cgx_v41(%d, %d, %d, %d, %d, %d\n)",
            0 , line_no, XOffset, YOffset + line_no, gfxinfo->width, 1);
#if 0
    clone_area(0 , line_no,
               gfxinfo->width,1);
#endif
}

static void flush_block_cgx_v41 (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    WritePixelArray (CybBuffer,
		     0 , first_line,
		     gfxinfo->rowbytes,
		     RP,
		     XOffset,
		     YOffset + first_line,
		     gfxinfo->width,
		     last_line - first_line + 1,
		     RECTFMT_RAW);
    AWTRACE("flush_block_cgx_v41(%d, %d, %d, %d, %d, %d)\n",
            0, first_line, XOffset, YOffset + first_line, 
	    gfxinfo->width, last_line - first_line + 1);

#if 0
    clone_area(0 , first_line,
               gfxinfo->width, last_line - first_line + 1);
#endif
}
# endif
#endif

/****************************************************************************/

static void flush_clear_screen_gfxlib (struct vidbuf_description *gfxinfo)
{
    if (RP) {
#ifdef USE_CYBERGFX
      /* DONE */
	if (use_cyb) {
	     FillPixelArray (RP, W->BorderLeft, W->BorderTop,
			     W->Width - W->BorderLeft - W->BorderRight,
			     W->Height - W->BorderTop - W->BorderBottom,
			     0);
#if 0
	      clone_area(W->BorderLeft, W->BorderTop,
	                 W->Width - W->BorderLeft - W->BorderRight,
			 W->Height - W->BorderTop - W->BorderBottom);
#endif
	}
        else
#endif
	{
	    SetAPen  (RP, get_nearest_color (0,0,0));
	    RectFill (RP, W->BorderLeft, W->BorderTop, W->Width - W->BorderRight,
		      W->Height - W->BorderBottom);
	}
    }
    if (use_delta_buffer)
        memset (oldpixbuf, 0, gfxinfo->rowbytes * gfxinfo->height);
}

/****************************************************************************/


static int RPDepth (struct RastPort *RP)
{
#ifdef USE_CYBERGFX
    if (use_cyb)
	return GetCyberMapAttr (RP->BitMap, (LONG)CYBRMATTR_DEPTH);
#endif
    return RP->BitMap->Depth;
}

/****************************************************************************/

static int get_color (int r, int g, int b, xcolnr *cnp)
{
    int col;

    if (currprefs.amiga_use_grey)
	r = g = b = (77 * r + 151 * g + 29 * b) / 16;
    else {
	r *= 0x11;
	g *= 0x11;
	b *= 0x11;
    }

    r *= 0x01010101;
    g *= 0x01010101;
    b *= 0x01010101;
    col = ObtainColor (r, g, b);

    if (col == -1) {
	get_color_failed = 1;
	return 0;
    }

    *cnp = col;
    return 1;
}

/****************************************************************************/
/*
 * FIXME: find a better way to determine closeness of colors (closer to
 * human perception).
 */
static __inline__ void rgb2xyz (int r, int g, int b,
				int *x, int *y, int *z)
{
    *x = r * 1024 - (g + b) * 512;
    *y = 886 * (g - b);
    *z = (r + g + b) * 341;
}

static __inline__ int calc_err (int r1, int g1, int b1,
				int r2, int g2, int b2)
{
    int x1, y1, z1, x2, y2, z2;

    rgb2xyz (r1, g1, b1, &x1, &y1, &z1);
    rgb2xyz (r2, g2, b2, &x2, &y2, &z2);
    x1 -= x2; y1 -= y2; z1 -= z2;
    return x1 * x1 + y1 * y1 + z1 * z1;
}

/****************************************************************************/

static int get_nearest_color (int r, int g, int b)
{
    int i, best, err, besterr;
    int colors;
    int br=0,bg=0,bb=0;

   if (currprefs.amiga_use_grey)
	r = g = b = (77 * r + 151 * g + 29 * b) / 256;

    best    = 0;
    besterr = calc_err (0, 0, 0, 15, 15, 15);
    colors  = is_halfbrite ? 32 :(1 << RPDepth (RP));

    for (i = 0; i < colors; i++) {
	long rgb;
	int cr, cg, cb;

	rgb = GetRGB4 (CM, i);

	cr = (rgb >> 8) & 15;
	cg = (rgb >> 4) & 15;
	cb = (rgb >> 0) & 15;

	err = calc_err (r, g, b, cr, cg, cb);

	if(err < besterr) {
	    best = i;
	    besterr = err;
	    br = cr; bg = cg; bb = cb;
	}

	if (is_halfbrite) {
	    cr /= 2; cg /= 2; cb /= 2;
	    err = calc_err (r, g, b, cr, cg, cb);
	    if (err < besterr) {
		best = i + 32;
		besterr = err;
		br = cr; bg = cg; bb = cb;
	    }
	}
    }
    return best;
}

/****************************************************************************/

#ifdef USE_CYBERGFX
static int init_colors_cgx (const struct RastPort *rp)
{
    int redbits,  greenbits,  bluebits;
    int redshift, greenshift, blueshift;
    int byte_swap = FALSE;
    int pixfmt;
    int found = TRUE;

    pixfmt = GetCyberMapAttr (rp->BitMap, (LONG)CYBRMATTR_PIXFMT);

    switch (pixfmt) {
#ifdef WORDS_BIGENDIAN
	case PIXFMT_RGB15PC:
	    byte_swap = TRUE;
	case PIXFMT_RGB15:
	    redbits  = 5;  greenbits  = 5; bluebits  = 5;
	    redshift = 10; greenshift = 5; blueshift = 0;
	    break;
	case PIXFMT_RGB16PC:
	    byte_swap = TRUE;
	case PIXFMT_RGB16:
	    redbits  = 5;  greenbits  = 6;  bluebits  = 5;
	    redshift = 11; greenshift = 5;  blueshift = 0;
	    break;
	case PIXFMT_RGBA32:
	    redbits  = 8;  greenbits  = 8;  bluebits  = 8;
	    redshift = 24; greenshift = 16; blueshift = 8;
	    break;
	case PIXFMT_BGRA32:
	    redbits  = 8;  greenbits  = 8;  bluebits  = 8;
	    redshift = 8;  greenshift = 16; blueshift = 24;
	    break;
	case PIXFMT_ARGB32:
	    redbits  = 8;  greenbits  = 8;  bluebits  = 8;
	    redshift = 16; greenshift = 8;  blueshift = 0;
	    break;
#else
	case PIXFMT_RGB15:
	    byte_swap = TRUE;
	case PIXFMT_RGB15PC:
	    redbits  = 5;  greenbits  = 5;  bluebits  = 5;
	    redshift = 10; greenshift = 0;  blueshift = 0;
	    break;
	case PIXFMT_RGB16:
	    byte_swap = TRUE;
	case PIXFMT_RGB16PC:
	    redbits  = 5;  greenbits  = 6;  bluebits  = 5;
	    redshift = 11; greenshift = 5;  blueshift = 0;
	    break;
	case PIXFMT_BGRA32:
	    redbits  = 8;  greenbits  = 8;  bluebits  = 8;
	    redshift = 16; greenshift = 8;  blueshift = 0;
	    break;
	case PIXFMT_ARGB32:
	    redbits  = 8;  greenbits  = 8;  bluebits  = 8;
	    redshift = 8;  greenshift = 16; blueshift = 24;
	    break;
#endif
	default:
	    redbits  = 0;  greenbits  = 0;  bluebits  = 0;
	    redshift = 0;  greenshift = 0;  blueshift = 0;
	    found = FALSE;
	    break;
    }

    if (found) {
	alloc_colors64k (redbits,  greenbits,  bluebits,
			 redshift, greenshift, blueshift,
			 0, 0, 0, byte_swap);

	write_log ("AMIGFX: Using a %d-bit true-colour display.\n",
		   redbits + greenbits + bluebits);
    } else
	write_log ("AMIGFX: Unsupported pixel format.\n");

    return found;
}
#endif

static int init_colors (void)
{
    int success = TRUE;

    if (need_dither) {
	/* first try color allocation */
	int bitdepth = usepub ? 8 : RPDepth (RP);
	int maxcol;

	if (!currprefs.amiga_use_grey && bitdepth >= 3)
	    do {
		get_color_failed = 0;
		setup_dither (bitdepth, get_color);
		if (get_color_failed)
		    ReleaseColors ();
	    } while (get_color_failed && --bitdepth >= 3);

	if( !currprefs.amiga_use_grey && bitdepth >= 3) {
	    write_log ("AMIGFX: Color dithering with %d bits\n", bitdepth);
	    return 1;
	}

	/* if that fail then try grey allocation */
	maxcol = 1 << (usepub ? 8 : RPDepth (RP));

	do {
	    get_color_failed = 0;
	    setup_greydither_maxcol (maxcol, get_color);
	    if (get_color_failed)
		ReleaseColors ();
	} while (get_color_failed && --maxcol >= 2);

	/* extra pass with approximated colors */
	if (get_color_failed)
	    do {
		maxcol=2;
		use_approx_color = 1;
		get_color_failed = 0;
		setup_greydither_maxcol (maxcol, get_color);
		if (get_color_failed)
		    ReleaseColors ();
	    } while (get_color_failed && --maxcol >= 2);

	if (maxcol >= 2) {
	    write_log ("AMIGFX: Grey dithering with %d shades.\n", maxcol);
	    return 1;
	}

	return 0; /* everything failed :-( */
    }

    /* No dither */
    switch (RPDepth (RP)) {
	case 6:
	    if (is_halfbrite) {
		static int tab[]= {
		    0x000, 0x00f, 0x0f0, 0x0ff, 0x08f, 0x0f8, 0xf00, 0xf0f,
		    0x80f, 0xff0, 0xfff, 0x88f, 0x8f0, 0x8f8, 0x8ff, 0xf08,
		    0xf80, 0xf88, 0xf8f, 0xff8, /* end of regular pattern */
		    0xa00, 0x0a0, 0xaa0, 0x00a, 0xa0a, 0x0aa, 0xaaa,
		    0xfaa, 0xf6a, 0xa80, 0x06a, 0x6af
		};
		int i;
		for (i = 0; i < 32; ++i)
		    get_color (tab[i] >> 8, (tab[i] >> 4) & 15, tab[i] & 15, xcolors);
		for (i = 0; i < 4096; ++i) {
		    uae_u32 val = get_nearest_color (i >> 8, (i >> 4) & 15, i & 15);
		    xcolors[i] = val * 0x01010101;
		}
		write_log ("AMIGFX: Using 32 colours and half-brite\n");
		break;
	    } else if (is_ham) {
		int i;
		for (i = 0; i < 16; ++i)
		    get_color (i, i, i, xcolors);
		write_log ("AMIGFX: Using 12 bits pseudo-truecolor (HAM).\n");
		alloc_colors64k (4, 4, 4, 10, 5, 0, 0, 0, 0, 0);
		return init_ham ();
	    }
	    /* Fall through if !is_halfbrite && !is_ham */
	case 1: case 2: case 3: case 4: case 5: case 7: case 8: {
	    int maxcol = 1 << RPDepth (RP);
	    if (maxcol >= 8 && !currprefs.amiga_use_grey)
		do {
		    get_color_failed = 0;
		    setup_maxcol (maxcol);
		    alloc_colors256 (get_color);
		    if (get_color_failed)
			ReleaseColors ();
		} while (get_color_failed && --maxcol >= 8);
	    else {
		int i;
		for (i = 0; i < maxcol; ++i) {
		    get_color ((i * 15)/(maxcol - 1), (i * 15)/(maxcol - 1),
			       (i * 15)/(maxcol - 1), xcolors);
		}
	    }
	    write_log ("AMIGFX: Using %d colours.\n", maxcol);
	    for (maxcol = 0; maxcol < 4096; ++maxcol) {
		int val = get_nearest_color (maxcol >> 8, (maxcol >> 4) & 15, maxcol & 15);
		xcolors[maxcol] = val * 0x01010101;
	    }
	    break;
	}
#ifdef USE_CYBERGFX
	case 15:
	case 16:
	case 24:
	case 32:
	    success = init_colors_cgx (RP);
	    break;
#endif
    }
    return success;
}

/****************************************************************************/

static APTR blank_pointer;

/*
 * Initializes a pointer object containing a blank pointer image.
 * Used for hiding the mouse pointer
 */
static void init_pointer (void)
{
    static struct BitMap bitmap;
    static UWORD	 row[2] = {0, 0};

    InitBitMap (&bitmap, 2, 16, 1);
    bitmap.Planes[0] = (PLANEPTR) &row[0];
    bitmap.Planes[1] = (PLANEPTR) &row[1];

    blank_pointer = NewObject (NULL, POINTERCLASS,
			       POINTERA_BitMap,	(ULONG)&bitmap,
			       POINTERA_WordWidth,	1,
			       TAG_DONE);

    if (!blank_pointer)
	write_log ("Warning: Unable to allocate blank mouse pointer.\n");
}

/*
 * Free up blank pointer object
 */
static void free_pointer (void)
{
    if (blank_pointer) {
	DisposeObject (blank_pointer);
	blank_pointer = NULL;
    }
}

/*
 * Hide mouse pointer for window
 */
void hide_pointer (struct Window *w)
{
    SetWindowPointer (w, WA_Pointer, (ULONG)blank_pointer, TAG_DONE);
}

/*
 * Restore default mouse pointer for window
 */
static void show_pointer (struct Window *w)
{
    SetWindowPointer (w, WA_Pointer, 0, TAG_DONE);
}

#ifdef __amigaos4__ 
/*
 * Grab mouse pointer under OS4.0. Needs to be called periodically
 * to maintain grabbed status.
 */
static void grab_pointer (struct Window *w)
{
    struct IBox box = {
	W->BorderLeft,
	W->BorderTop,
	W->Width  - W->BorderLeft - W->BorderRight,
	W->Height - W->BorderTop  - W->BorderBottom
    };

    SetWindowAttrs (W, WA_MouseLimits, &box, sizeof box);
    SetWindowAttrs (W, WA_GrabFocus, mouseGrabbed ? GRAB_TIMEOUT : 0, sizeof (ULONG));
}
#endif

/****************************************************************************/

typedef enum {
    DONT_KNOW = -1,
    INSIDE_WINDOW,
    OUTSIDE_WINDOW
} POINTER_STATE;

static POINTER_STATE pointer_state;

static POINTER_STATE get_pointer_state (const struct Window *w, int mousex, int mousey)
{
    POINTER_STATE new_state = OUTSIDE_WINDOW;

    /*
     * Is pointer within the bounds of the inner window?
     */
    if ((mousex >= w->BorderLeft)
     && (mousey >= w->BorderTop)
     && (mousex < (w->Width - w->BorderRight))
     && (mousey < (w->Height - w->BorderBottom))) {
	/*
	 * Yes. Now check whetehr the window is obscured by
	 * another window at the pointer position
	 */
	struct Screen *scr = w->WScreen;
	struct Layer  *layer;

	/* Find which layer the pointer is in */
	LockLayerInfo (&scr->LayerInfo);
	layer = WhichLayer (&scr->LayerInfo, scr->MouseX, scr->MouseY);
	UnlockLayerInfo (&scr->LayerInfo);

	/* Is this layer our window's layer? */
	if (layer == w->WLayer) {
	    /*
	     * Yes. Therefore, pointer is inside the window.
	     */
	    new_state = INSIDE_WINDOW;
	}
    }
    return new_state;
}

/****************************************************************************/

#ifdef USE_CYBERGFX
/*
 * Try to find a CGX/P96 screen mode which suits the requested size and depth
 */
ULONG find_rtg_mode (ULONG *width, ULONG *height, ULONG depth)
{
    ULONG mode           = INVALID_ID;

    ULONG best_mode      = INVALID_ID;
    ULONG best_width     = (ULONG) -1L;
    ULONG best_height    = (ULONG) -1L;

    ULONG largest_mode   = INVALID_ID;
    ULONG largest_width  = 0;
    ULONG largest_height = 0;

    AWTRACE ("Looking for RTG mode: w:%d h:%d d:%d\n", *width, *height, depth);

    if (CyberGfxBase) {
	while ((mode = NextDisplayInfo (mode)) != (ULONG)INVALID_ID) {
	    if (IsCyberModeID (mode)) {
		ULONG cwidth  = GetCyberIDAttr (CYBRIDATTR_WIDTH, mode);
		ULONG cheight = GetCyberIDAttr (CYBRIDATTR_HEIGHT, mode);
		ULONG cdepth  = GetCyberIDAttr (CYBRIDATTR_DEPTH, mode);
		AWTRACE ("Checking mode:%08x w:%d h:%d d:%d -> ", mode, cwidth, cheight, cdepth);
		if (cdepth == depth) {
		    /*
		     * If this mode has the largest screen size we've seen so far,
		     * remember it, just in case we don't find one big enough
		     */
		    if (cheight >= largest_height && cwidth >= largest_width) {
			largest_mode   = mode;
			largest_width  = cwidth;
			largest_height = cheight;
		    }

		    /*
		     * Is it large enough for our requirements?
		     */
		    if (cwidth >= *width && cheight >= *height) {
			AWTRACE ("large enough\n");
			/*
			 * Yes. Is it the best fit that we've seen so far?
			 */
			if (cwidth <= best_width && cheight <= best_height) {
			    best_width  = cwidth;
			    best_height = cheight;
			    best_mode   = mode;
			}
		    }
		    else {
			AWTRACE ("too small\n");
		    }

		} /* if (cdepth == depth) */
		else {
		    AWTRACE ("wrong depth\n");
		}
	    } /* if (IsCyberModeID (mode)) */
	} /* while */

	if (best_mode != (ULONG)INVALID_ID) {
	    AWTRACE ("Found match!\n");
	    /* We found a match. Return it */
	    *height = best_height;
	    *width  = best_width;
	} else if (largest_mode != (ULONG)INVALID_ID) {
	    AWTRACE ("No match found!\n");
	    /* We didn't find a large enough mode. Return the largest
	     * mode found at the depth - if we found one */
	    best_mode = largest_mode;
	    *height   = largest_height;
	    *width    = largest_width;
	}
	else {
	    AWTRACE ("WTF?\n");
	}
	if (best_mode != (ULONG) INVALID_ID) {
	    AWTRACE ("Best mode: %08x w:%d h:%d d:%d\n", best_mode, *width, *height, depth);
	}
	else {
	    AWTRACE ("No best mode found \n");
	}
    }
    return best_mode;
}
#endif

static int setup_customscreen (void)
{
    static struct NewWindow NewWindowStructure = {
	0, 0, 800, 600, 0, 1,
	IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_DISKINSERTED | IDCMP_DISKREMOVED
		| IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_MOUSEMOVE
		| IDCMP_DELTAMOVE,
	WFLG_SMART_REFRESH | WFLG_BACKDROP | WFLG_RMBTRAP | WFLG_NOCAREREFRESH
	 | WFLG_BORDERLESS | WFLG_ACTIVATE | WFLG_REPORTMOUSE,
	NULL, NULL, NULL, NULL, NULL, 5, 5, 800, 600,
	CUSTOMSCREEN
    };

    ULONG width  = gfxvidinfo.width;
    ULONG height = gfxvidinfo.height;
    ULONG depth  = 0; // FIXME: Need to add some way of letting user specify preferred depth
    ULONG mode   = INVALID_ID;
    struct Screen *screen;
    ULONG error;

    AWTRACE("entered\n");
    AWTRACE("width x height: %d x %d\n",width,height);

#ifdef USE_CYBERGFX
    /* First try to find an RTG screen that matches the requested size  */
    {
	unsigned int i;
	const UBYTE preferred_depth[] = {24, 15, 16, 32, 8}; 
	/* Try depths in this order of preference */

	for (i = 0; i < sizeof preferred_depth && mode == (ULONG) INVALID_ID; i++) {
	    depth = preferred_depth[i];
	    mode = find_rtg_mode (&width, &height, depth);
	}
    }

    if (mode != (ULONG) INVALID_ID) {
	if (depth > 8)
	    use_cyb = 1;
    } else {
#endif
	/* No (suitable) RTG screen available. Try a native mode */
	depth = os39 ? 8 : (currprefs.gfx_lores ? 5 : 4);
	/* FIXME: should check whether to use PAL or NTSC.*/
	mode = PAL_MONITOR_ID; 

	if (currprefs.gfx_lores)
	    mode |= (gfxvidinfo.height > 256) ? LORESLACE_KEY : LORES_KEY;
	else
	    mode |= (gfxvidinfo.height > 256) ? HIRESLACE_KEY : HIRES_KEY;
#ifdef USE_CYBERGFX
    }
#endif

    /* If the screen is larger than requested, centre UAE's display */
    if (width > (ULONG) gfxvidinfo.width)
	XOffset = (width - gfxvidinfo.width) / 2;
    if (height > (ULONG) gfxvidinfo.height)
	YOffset = (height - gfxvidinfo.height) / 2;

    AWTRACE("width x height: %d x %d\n",width,height);
    AWTRACE("mode: %lx\n",mode);

    do {
      AWTRACE("depth: %d\n",depth);
      screen = OpenScreenTags (NULL,
			       SA_Width,     width,
			       SA_Height,    height,
			       SA_Depth,     depth,
			       SA_DisplayID, mode,
			       SA_Behind,    TRUE,
			       SA_ShowTitle, FALSE,
			       SA_Quiet,     TRUE,
			       SA_ErrorCode, (ULONG)&error,
			       TAG_DONE);
    } while (!screen && error == OSERR_TOODEEP && --depth > 1); 
    /* Keep trying until we find a supported depth */

    if (!screen) {
	/* TODO; Make this error report more useful based on the error code we got */
	AWTRACE ("Error opening screen:%ld\n", error);
	gui_message ("Cannot open custom screen for UAE.\n");
	return 0;
    }

    AWTRACE("screen: %lx (%d x %d)\n",screen,screen->Width,screen->Height);

    S  = screen;
    CM = screen->ViewPort.ColorMap;
    RP = &screen->RastPort;

    NewWindowStructure.Width  = screen->Width;
    NewWindowStructure.Height = screen->Height;
    NewWindowStructure.Screen = screen;

    W = (void*)OpenWindow (&NewWindowStructure);
    if (!W) {
	AWTRACE ("Cannot open UAE window on custom screen.\n");
	return 0;
    }

    hide_pointer (W);

    AWTRACE("done\n");

    return 1;
}

/****************************************************************************
 * show_uae_main_window()
 *
 * If we go from rootless to normal mode, we close all rootless windows
 * with close_all_janus_windows() and resize our main window with
 * show_uae_main_window().
 *
 ****************************************************************************/
void show_uae_main_window(void) {

  if(!W) {
    AWTRACE("ERROR: W == NULL ??\n");
    return;
  }

  AWTRACE("resize window to:\n");
  AWTRACE("   x,y: %d, %d\n", uae_main_window_Top, uae_main_window_Left);
  AWTRACE("   w,h: %d, %d\n", uae_main_window_Width, uae_main_window_Height);
  AWTRACE("\n");
  AWTRACE("   gfxvidinfo.width: %d\n",gfxvidinfo.width);
  AWTRACE("   gfxvidinfo.width+border: %d\n",gfxvidinfo.width+W->BorderLeft+W->BorderRight);
  AWTRACE("   gfxvidinfo.height: %d\n",gfxvidinfo.height);
  AWTRACE("   gfxvidinfo.height+border: %d\n",gfxvidinfo.height+W->BorderTop+W->BorderBottom);

  ChangeWindowBox(W, uae_main_window_Top, uae_main_window_Left,
		      uae_main_window_Width, uae_main_window_Height);

  uae_main_window_closed=FALSE;
}

/****************************************************************************
 * hide_uae_main_window()
 *
 * We move the main window to the lower right edge of our screen and
 * resize it to 1x1. We still need this window, otherwise quite some
 * access to non valid structures would happen.
 ****************************************************************************/
void hide_uae_main_window(void) {

  if(!W) {
    AWTRACE("ERROR: W == NULL ??\n");
    return;
  }

  uae_main_window_Width  = W->Width;
  uae_main_window_Height = W->Height;
  uae_main_window_Top    = W->TopEdge;
  uae_main_window_Left   = W->LeftEdge;

  AWTRACE("remember:\n");
  AWTRACE("   x,y: %d, %d\n", uae_main_window_Top, uae_main_window_Left);
  AWTRACE("   w,h: %d, %d\n", uae_main_window_Width, uae_main_window_Height);
  AWTRACE("\n");
  AWTRACE("   gfxvidinfo.width: %d\n",gfxvidinfo.width);
  AWTRACE("   gfxvidinfo.width+border: %d\n",gfxvidinfo.width+W->BorderLeft+W->BorderRight);
  AWTRACE("   gfxvidinfo.height: %d\n",gfxvidinfo.height);
  AWTRACE("   gfxvidinfo.height+border: %d\n",gfxvidinfo.height+W->BorderTop+W->BorderBottom);

  ChangeWindowBox(W, S->Width-1, S->Height-1, 1, 1);

  uae_main_window_closed=TRUE;
}

static int setup_publicscreen(void)
{
    UWORD ZoomArray[4] = {0, 0, 0, 0};
    char *pubscreen = strlen (currprefs.amiga_publicscreen)
	? currprefs.amiga_publicscreen : NULL;

    AWTRACE("entered (pubscreen >%s<)\n",currprefs.amiga_publicscreen);

    S = LockPubScreen ((UBYTE *) pubscreen);
    if (!S) {
	gui_message ("Cannot open UAE window on public screen '%s'\n",
		pubscreen ? pubscreen : "default");
	return 0;
    }

    ZoomArray[2] = 128;
    ZoomArray[3] = S->BarHeight + 1;

    CM = S->ViewPort.ColorMap;

    if ((S->ViewPort.Modes & (HIRES | LACE)) == HIRES) {
	if (gfxvidinfo.height + S->BarHeight + 1 >= S->Height) {
	    gfxvidinfo.height >>= 1;
	    currprefs.gfx_correct_aspect = 1;
	}
    }

    if(currprefs.jcoherence) {
      /*
       * as it seems, that a lot of stuff depends on a uae main window existing (RastPort etc),
       * we open one just for those variables to be valid. It is put into the right lower
       * border of the screen and size 1x1
       */

      W = OpenWindowTags (NULL,
			  WA_Title,        (ULONG)PACKAGE_NAME,
			  WA_AutoAdjust,   TRUE,
			  WA_Left,         S->Width,
			  WA_Top,          S->Height,
			  WA_Width,        1,
			  WA_Height,       1,
			  WA_PubScreen,    (ULONG)S,
			  WA_Zoom,         (ULONG)ZoomArray,
			  WA_IDCMP,        IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY
					 | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW
					 | IDCMP_MOUSEMOVE    | IDCMP_DELTAMOVE
					 | IDCMP_CLOSEWINDOW  | IDCMP_REFRESHWINDOW
					 | IDCMP_NEWSIZE      | IDCMP_INTUITICKS,
			  WA_Flags,	 WFLG_DRAGBAR     | WFLG_DEPTHGADGET
					 | WFLG_REPORTMOUSE | WFLG_RMBTRAP
					 | WFLG_ACTIVATE    | WFLG_CLOSEGADGET
					 | WFLG_SMART_REFRESH,
			  TAG_DONE);
      /* remember values (not used)*/
      uae_main_window_Left  =1;
      uae_main_window_Top   =1;
      uae_main_window_Width =gfxvidinfo.width  + W->BorderLeft + W->BorderRight;
      uae_main_window_Height=gfxvidinfo.height + W->BorderTop  + W->BorderBottom;

      uae_main_window_closed=TRUE;
    }
    else {

      W = OpenWindowTags (NULL,
			  WA_Title,        (ULONG)PACKAGE_NAME,
			  WA_AutoAdjust,   TRUE,
			  WA_InnerWidth,   gfxvidinfo.width,
			  WA_InnerHeight,  gfxvidinfo.height,
			  WA_PubScreen,    (ULONG)S,
			  WA_Zoom,         (ULONG)ZoomArray,
			  WA_IDCMP,        IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY
					 | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW
					 | IDCMP_MOUSEMOVE    | IDCMP_DELTAMOVE
					 | IDCMP_CLOSEWINDOW  | IDCMP_REFRESHWINDOW
					 | IDCMP_NEWSIZE      | IDCMP_INTUITICKS,
			  WA_Flags,	 WFLG_DRAGBAR     | WFLG_DEPTHGADGET
					 | WFLG_REPORTMOUSE | WFLG_RMBTRAP
					 | WFLG_ACTIVATE    | WFLG_CLOSEGADGET
					 | WFLG_SMART_REFRESH,
			  TAG_DONE);
      uae_main_window_closed=FALSE;

      gfxvidinfo.width  = (W->Width  - W->BorderRight - W->BorderLeft);   
      gfxvidinfo.height = (W->Height - W->BorderTop   - W->BorderBottom); 

    }

    UnlockPubScreen (NULL, S);


    if (!W) {
	write_log ("Can't open window on public screen!\n");
	CM = NULL;
	return 0;
    }

    AWTRACE("openen uae window %lx\n",W);

AWTRACE("gfxvidinfo.width: %d\n", gfxvidinfo.width);
AWTRACE("gfxvidinfo.height: %d\n",gfxvidinfo.height);
    XOffset = W->BorderLeft;
    YOffset = W->BorderTop;
AWTRACE("gfxvidinfo.width: %d\n", gfxvidinfo.width);
AWTRACE("gfxvidinfo.height: %d\n",gfxvidinfo.height);

    RP = W->RPort;

    appw_init (W);

#ifdef USE_CYBERGFX
    if (CyberGfxBase && GetCyberMapAttr (RP->BitMap, (LONG)CYBRMATTR_ISCYBERGFX) &&
			(GetCyberMapAttr (RP->BitMap, (LONG)CYBRMATTR_DEPTH) > 8)) {
	use_cyb = 1;
    }

#endif

    return 1;
}

/****************************************************************************/

static char *get_num (char *s, int *n)
{
   int i=0;
   while(isspace(*s)) ++s;
   if(*s=='0') {
     ++s;
     if(*s=='x' || *s=='X') {
       do {char c=*++s;
           if(c>='0' && c<='9') {i*=16; i+= c-'0';}    else
           if(c>='a' && c<='f') {i*=16; i+= c-'a'+10;} else
           if(c>='A' && c<='F') {i*=16; i+= c-'A'+10;} else break;
       } while(1);
     } else while(*s>='0' && *s<='7') {i*=8; i+= *s++ - '0';}
   } else {
     while(*s>='0' && *s<='9') {i*=10; i+= *s++ - '0';}
   }
   *n=i;
   while(isspace(*s)) ++s;
   return s;
}

/****************************************************************************/

static void get_displayid (ULONG *DI, LONG *DE)
{
   char *s;
   int di,de;

   *DI=INVALID_ID;
   s=getenv(UAESM);if(!s) return;
   s=get_num(s,&di);
   if(*s!=':') return;
   s=get_num(s+1,&de);
   if(!de) return;
   *DI=di; *DE=de;
}

/****************************************************************************/

static int setup_userscreen (void)
{
    struct ScreenModeRequester *ScreenRequest;
    ULONG DisplayID;


    LONG ScreenWidth = 0, ScreenHeight = 0, Depth = 0;
    UWORD OverscanType = OSCAN_STANDARD;
    BOOL AutoScroll = TRUE;
    int release_asl = 0;

    AWTRACE("entered\n");

    if (!AslBase) {
	AslBase = OpenLibrary (AslName, 36);
	if (!AslBase) {
	    write_log ("Can't open asl.library v36.\n");
	    return 0;
	} else {
#ifdef __amigaos4__
	    IAsl = (struct AslIFace *) GetInterface ((struct Library *)AslBase, "main", 1, NULL);
	    if (!IAsl) {
		CloseLibrary (AslBase);
		AslBase = 0;
		write_log ("Can't get asl.library interface\n");
	    }
#endif
	}
#ifdef __amigaos4__
    } else {
        IAsl->Obtain ();
        release_asl = 1;
#endif
    }

    ScreenRequest = AllocAslRequest (ASL_ScreenModeRequest, NULL);

    if (!ScreenRequest) {
	write_log ("Unable to allocate screen mode requester.\n");
	return 0;
    }

    get_displayid (&DisplayID, &Depth);

    if (DisplayID == (ULONG)INVALID_ID) {
	if (AslRequestTags (ScreenRequest,
			ASLSM_TitleText, (ULONG)"Select screen display mode",
			ASLSM_InitialDisplayID,    0,
			ASLSM_InitialDisplayDepth, 8,
			ASLSM_InitialDisplayWidth, gfxvidinfo.width,
			ASLSM_InitialDisplayHeight,gfxvidinfo.height,
			ASLSM_MinWidth,            320, //currprefs.gfx_width_win,
			ASLSM_MinHeight,           200, //currprefs.gfx_height_win,
			ASLSM_DoWidth,             TRUE,
			ASLSM_DoHeight,            TRUE,
			ASLSM_DoDepth,             TRUE,
			ASLSM_DoOverscanType,      TRUE,
			ASLSM_PropertyFlags,       0,
			ASLSM_PropertyMask,        DIPF_IS_DUALPF | DIPF_IS_PF2PRI,
			TAG_DONE)) {
	    ScreenWidth  = ScreenRequest->sm_DisplayWidth;
	    ScreenHeight = ScreenRequest->sm_DisplayHeight;
	    Depth        = ScreenRequest->sm_DisplayDepth;
	    DisplayID    = ScreenRequest->sm_DisplayID;
	    OverscanType = ScreenRequest->sm_OverscanType;
	    AutoScroll   = ScreenRequest->sm_AutoScroll;
	} else
	    DisplayID = INVALID_ID;
    }
    FreeAslRequest (ScreenRequest);

    if (DisplayID == (ULONG)INVALID_ID)
	return 0;

#ifdef USE_CYBERGFX
    if (CyberGfxBase && IsCyberModeID (DisplayID) && (Depth > 8)) {
	use_cyb = 1;

    }
#endif
    if ((DisplayID & HAM_KEY) && !use_cyb )
	Depth = 6; /* only ham6 for the moment */
#if 0
    if(DisplayID & DIPF_IS_HAM) Depth = 6; /* only ham6 for the moment */
#endif

    /* If chosen screen is smaller than UAE display size then clip
     * display to screen size */
    if (ScreenWidth  < gfxvidinfo.width)
	gfxvidinfo.width = ScreenWidth;
    if (ScreenHeight < gfxvidinfo.width)
	gfxvidinfo.height = ScreenHeight;

    /* If chosen screen is larger, than centre UAE's display */
    if (ScreenWidth > gfxvidinfo.width)
	XOffset = (ScreenWidth - gfxvidinfo.width) / 2;
    if (ScreenHeight > gfxvidinfo.width)
	YOffset = (ScreenHeight - gfxvidinfo.height) / 2;

    S = OpenScreenTags (NULL,
			SA_DisplayID,			 DisplayID,
			SA_Width,			 ScreenWidth,
			SA_Height,			 ScreenHeight,
			SA_Depth,			 Depth,
			SA_Overscan,			 OverscanType,
			SA_AutoScroll,			 AutoScroll,
			SA_ShowTitle,			 FALSE,
			SA_Quiet,			 TRUE,
			SA_Behind,			 TRUE,
			SA_PubName,			 (ULONG)"UAE",
			/* v39 stuff here: */
			(os39 ? SA_BackFill : TAG_DONE), (ULONG)LAYERS_NOBACKFILL,
			SA_SharePens,			 TRUE,
			SA_Exclusive,			 (use_cyb ? TRUE : FALSE),
			SA_Draggable,			 (use_cyb ? FALSE : TRUE),
			SA_Interleaved,			 TRUE,
			TAG_DONE);
    if (!S) {
	gui_message ("Unable to open the requested screen.\n");
	return 0;
    }

    CM           =  S->ViewPort.ColorMap;
    is_halfbrite = (S->ViewPort.Modes & EXTRA_HALFBRITE);
    is_ham       = (S->ViewPort.Modes & HAM);

    W = OpenWindowTags (NULL,
			WA_Width,		S->Width,
			WA_Height,		S->Height,
			WA_CustomScreen,	(ULONG)S,
			WA_Backdrop,		TRUE,
			WA_Borderless,		TRUE,
			WA_RMBTrap,		TRUE,
			WA_Activate,		TRUE,
			WA_ReportMouse,		TRUE,
			WA_IDCMP,		IDCMP_MOUSEBUTTONS
					      | IDCMP_RAWKEY
					      | IDCMP_DISKINSERTED
					      | IDCMP_DISKREMOVED
					      | IDCMP_ACTIVEWINDOW
					      | IDCMP_INACTIVEWINDOW
					      | IDCMP_MOUSEMOVE
					      | IDCMP_DELTAMOVE,
			(os39 ? WA_BackFill : TAG_IGNORE),   (ULONG) LAYERS_NOBACKFILL,
			TAG_DONE);

    if(!W) {
	AWTRACE ("Unable to open the window.\n");
	CloseScreen (S);
	S  = NULL;
	RP = NULL;
	CM = NULL;
	return 0;
    }

    hide_pointer (W);

    RP = W->RPort; /* &S->Rastport if screen is not public */

    /* make screen public !? */
    PubScreenStatus (S, 0);

    AWTRACE ("Using screenmode: 0x%lx:%ld (%lu:%ld)\n",
	DisplayID, Depth, DisplayID, Depth);

    return 1;
}

/****************************************************************************/

int graphics_setup (void)
{
  AWTRACE("graphics_setup\n");
    if (((struct ExecBase *)SysBase)->LibNode.lib_Version < 36) {
	write_log ("UAE needs OS 2.0+ !\n");
	return 0;
    }
    os39 = (((struct ExecBase *)SysBase)->LibNode.lib_Version >= 39);

    atexit (graphics_leave);

#if !defined GTKMUI
    IntuitionBase = (void*) OpenLibrary ("intuition.library", 0L);
#endif
    if (!IntuitionBase) {
	write_log ("No intuition.library ?\n");
	return 0;
    } else {
#ifdef __amigaos4__
	IIntuition = (struct IntuitionIFace *) GetInterface ((struct Library *) IntuitionBase, "main", 1, NULL);
	if (!IIntuition) {
	    CloseLibrary ((struct Library *) IntuitionBase);
	    IntuitionBase = 0;
	    return 0;
	}
#endif
    }

#if !defined GTKMUI
    GfxBase = (void*) OpenLibrary ("graphics.library", 0L);
#endif
    if (!GfxBase) {
	write_log ("No graphics.library ?\n");
	return 0;
    } else {
#ifdef __amigaos4__
	IGraphics = (struct GraphicsIFace *) GetInterface ((struct Library *) GfxBase, "main", 1, NULL);
	if (!IGraphics) {
	    CloseLibrary ((struct Library *) GfxBase);
	    GfxBase = 0;
	    return 0;
	}
#endif
    }

    LayersBase = OpenLibrary ("layers.library", 0L);
    if (!LayersBase) {
	write_log ("No layers.library\n");
	return 0;
    } else {
#ifdef __amigaos4__
	ILayers = (struct LayersIFace *) GetInterface (LayersBase, "main", 1, NULL);
	if (!ILayers) {
	    CloseLibrary (LayersBase);
	    LayersBase = 0;
	    return 0;
	}
#endif
    }

#ifdef USE_CYBERGFX
    if (!CyberGfxBase) {
        CyberGfxBase = OpenLibrary ("cybergraphics.library", 40);
#ifdef __amigaos4__
        if (CyberGfxBase) {
	   ICyberGfx = (struct CyberGfxIFace *) GetInterface (CyberGfxBase, "main", 1, NULL);
           if (!ICyberGfx) {
	       CloseLibrary (CyberGfxBase);
	       CyberGfxBase = 0;
	   }
	}
#endif
    }
#endif

/* we should have an j-uae define .. */
#if defined GTKMUI
  GadToolsBase = OpenLibrary( "gadtools.library", 37L );
  if(!GadToolsBase) {
    write_log ("gadtools.library missing? Needed for j-uae menu support!\n");
    return 0;
  }
#endif

    init_pointer ();

    initpseudodevices ();

    return 1;
}

/****************************************************************************/

static struct Window *saved_prWindowPtr;

static void set_prWindowPtr (struct Window *w)
{
   struct Process *self = (struct Process *) FindTask (NULL);

   if (!saved_prWindowPtr)
	saved_prWindowPtr = self->pr_WindowPtr;
   self->pr_WindowPtr = w;
}

static void restore_prWindowPtr (void)
{
   struct Process *self = (struct Process *) FindTask (NULL);

   if (saved_prWindowPtr)
	self->pr_WindowPtr = saved_prWindowPtr;
}

/****************************************************************************/

#ifdef USE_CYBERGFX
# ifdef USE_CYBERGFX_V41
/* Allocate and set-up off-screen buffer for rendering Amiga display to
 * when using CGX V41 or better
 *
 * gfxinfo - the buffer description (which gets filled in by this routine)
 * rp      - the Rastport this buffer will be blitted to
 */
static APTR setup_cgx41_buffer (struct vidbuf_description *gfxinfo, const struct RastPort *rp)
{
    int bytes_per_row   = GetCyberMapAttr (rp->BitMap, CYBRMATTR_XMOD);
    int bytes_per_pixel = GetCyberMapAttr (rp->BitMap, CYBRMATTR_BPPIX);
    APTR buffer;

    /* Note we allocate a buffer with the same width as the destination
     * bitmap - not the width of the output we want. This is because
     * WritePixelArray using RECTFMT_RAW seems to require the source
     * and destination modulos to be equal. It certainly goes all wobbly
     * on MorphOS at least when they differ.
     */
    buffer = AllocVec (bytes_per_row * gfxinfo->height, MEMF_ANY);

    if (buffer) {
	gfxinfo->bufmem      = buffer;
	gfxinfo->pixbytes    = bytes_per_pixel;
	gfxinfo->rowbytes    = bytes_per_row;

	gfxinfo->flush_line  = flush_line_cgx_v41;
	gfxinfo->flush_block = flush_block_cgx_v41;
    }
    return buffer;
}
# else
/* Allocate and set-up off-screen buffer for rendering Amiga display to
 * when using pre-CGX V41.
 *
 * gfxinfo - the buffer description (which gets filled in by this routine)
 * rp      - the Rastport this buffer will be blitted to
 */
static APTR setup_cgx_buffer (struct vidbuf_description *gfxinfo, const struct RastPort *rp)
{
    int pixfmt   = GetCyberMapAttr (rp->BitMap, CYBRMATTR_PIXFMT);
    int bitdepth = RPDepth (RP);
    struct BitMap *bitmap;

    bitmap = myAllocBitMap (gfxinfo->width, gfxinfo->height + 1,
			    bitdepth,
			    (pixfmt << 24) | BMF_SPECIALFMT | BMF_MINPLANES,
			    rp->BitMap);

    if (bitmap) {
	gfxinfo->bufmem   = (char *) GetCyberMapAttr (bitmap, CYBRMATTR_DISPADR);
	gfxinfo->rowbytes = 	     GetCyberMapAttr (bitmap, CYBRMATTR_XMOD);
	gfxinfo->pixbytes = 	     GetCyberMapAttr (bitmap, CYBRMATTR_BPPIX);

	gfxinfo->flush_line =        flush_line_cgx;
	gfxinfo->flush_block =       flush_block_cgx;
    }

    return bitmap;
}
# endif
#endif

int graphics_init (void)
{
    int i, bitdepth;

    AWTRACE("graphics_init\n");

    use_delta_buffer = 0;
    need_dither = 0;
    use_cyb = 0;

/* We'll ignore color_mode for now.
    if (currprefs.color_mode > 5) {
        write_log ("Bad color mode selected. Using default.\n");
        currprefs.color_mode = 0;
    }
*/

    gfxvidinfo.width  = currprefs.gfx_width_win;
    gfxvidinfo.height = currprefs.gfx_height_win;

    if (gfxvidinfo.width < 320)
	gfxvidinfo.width = 320;
    if (!currprefs.gfx_correct_aspect && (gfxvidinfo.width < 64))
	gfxvidinfo.width = 200;

    gfxvidinfo.width += 7;
    gfxvidinfo.width &= ~7;


    switch (currprefs.amiga_screen_type) {
	case UAESCREENTYPE_ASK:
	    AWTRACE("currprefs.amiga_screen_type: UAESCREENTYPE_ASK\n");
	    if (setup_userscreen ())
		break;
	    AWTRACE ("Trying on public screen...\n");
	    write_log ("Trying on public screen...\n");
	    /* fall trough */
	case UAESCREENTYPE_PUBLIC:
	    is_halfbrite = 0;
	    AWTRACE("currprefs.amiga_screen_type: UAESCREENTYPE_PUBLIC\n");
	    if (setup_publicscreen ()) {
		usepub = 1;
		break;
	    }
	    AWTRACE ("Trying on public screen...\n");
	    write_log ("Trying on custom screen...\n");
	    /* fall trough */
	case UAESCREENTYPE_CUSTOM:
	default:
	    AWTRACE("currprefs.amiga_screen_type: UAESCREENTYPE_CUSTOM\n");
	    if (!setup_customscreen ())
		return 0;
	    break;
    }

    set_prWindowPtr (W);

    Line = AllocVec ((gfxvidinfo.width + 15) & ~15, MEMF_ANY | MEMF_PUBLIC);
    if (!Line) {
	write_log ("Unable to allocate raster buffer.\n");
	return 0;
    }
    BitMap = myAllocBitMap (gfxvidinfo.width, 1, 8, BMF_CLEAR | BMF_MINPLANES, RP->BitMap);
    if (!BitMap) {
	write_log ("Unable to allocate BitMap.\n");
	return 0;
    }
    TempRPort = AllocVec (sizeof (struct RastPort), MEMF_ANY | MEMF_PUBLIC);
    if (!TempRPort) {
	write_log ("Unable to allocate RastPort.\n");
	return 0;
    }
    CopyMem (RP, TempRPort, sizeof (struct RastPort));
    TempRPort->Layer  = NULL;
    TempRPort->BitMap = BitMap;

    if (usepub)
	set_title ();

    bitdepth = RPDepth (RP);

    gfxvidinfo.emergmem = 0;
    gfxvidinfo.linemem  = 0;

#ifdef USE_CYBERGFX
    if (use_cyb) {
	/*
	 * If using P96/CGX for output try to allocate on off-screen bitmap
	 * as the display buffer
	 *
	 * We do this now, so if it fails we can easily fall back on using
	 * graphics.library and palette-based rendering.
	 */


# ifdef USE_CYBERGFX_V41
	CybBuffer = setup_cgx41_buffer (&gfxvidinfo, RP);

	if (!CybBuffer) {
# else
	CybBitMap = setup_cgx_buffer (&gfxvidinfo, RP);

	if (!CybBitMap) {
# endif
	    /*
	     * Failed to allocate bitmap - we need to fall back on gfx.lib rendering
	     */
	    gfxvidinfo.bufmem = NULL;
	    use_cyb = 0;
	    if (bitdepth > 8) {
		bitdepth = 8;
		write_log ("AMIGFX: Failed to allocate off-screen buffer - falling back on 8-bit mode\n");
	    }
	}
    }
#endif

    if (is_ham) {
	/* ham 6 */
	use_delta_buffer       = 0; /* needless as the line must be fully recomputed */
	need_dither            = 0;
	gfxvidinfo.pixbytes    = 2;
	gfxvidinfo.flush_line  = flush_line_ham;
	gfxvidinfo.flush_block = flush_block_ham;
    } else if (bitdepth <= 8) {
	/* chunk2planar is slow so we define use_delta_buffer for all modes */
	use_delta_buffer       = 1;
	need_dither            = currprefs.amiga_use_dither || (bitdepth <= 1);
	gfxvidinfo.pixbytes    = need_dither ? 2 : 1;
	gfxvidinfo.flush_line  = need_dither ? flush_line_planar_dither  : flush_line_planar_nodither;
	gfxvidinfo.flush_block = need_dither ? flush_block_planar_dither : flush_block_planar_nodither;
    }

    gfxvidinfo.flush_clear_screen = flush_clear_screen_gfxlib;
    gfxvidinfo.flush_screen       = dummy_flush_screen;
    gfxvidinfo.lockscr            = dummy_lock;
    gfxvidinfo.unlockscr          = dummy_unlock;     

    if (!use_cyb) {
	/*
	 * We're not using GGX/P96 for output, so allocate a dumb
	 * display buffer
	 */
	gfxvidinfo.rowbytes = gfxvidinfo.pixbytes * gfxvidinfo.width;
	gfxvidinfo.bufmem   = (uae_u8 *) calloc (gfxvidinfo.rowbytes, gfxvidinfo.height + 1);
	/*									       ^^^ */
	/*				       This is because DitherLine may read one extra row */
    }

    if (!gfxvidinfo.bufmem) {
	write_log ("AMIGFX: Not enough memory for video bufmem.\n");
	return 0;
    }


    if (use_delta_buffer) {
	oldpixbuf = (uae_u8 *) calloc (gfxvidinfo.rowbytes, gfxvidinfo.height);
	if (!oldpixbuf) {
	    write_log ("AMIGFX: Not enough memory for oldpixbuf.\n");
	    return 0;
	}
    }

    gfxvidinfo.maxblocklines = MAXBLOCKLINES_MAX;

    if (!init_colors ()) {
	write_log ("AMIGFX: Failed to init colors.\n");
	return 0;
    }

    if (!usepub)
	ScreenToFront (S);

    reset_drawing ();

    set_default_hotkeys (ami_hotkeys);

    pointer_state = DONT_KNOW;

   return 1;
}

/****************************************************************************/

void graphics_leave (void)
{
  int i;
    AWTRACE("graphics_leave\n");
    /* stop cloning and close all clone windows */
    changed_prefs.jcoherence=FALSE;
    close_all_janus_windows();
    i=0;
    /* wait till list is empty (all windows are closed) */
    while(janus_windows && i<5) {
      i++;
      AWTRACE("wait for windows to be closed (#%d)\n",i);
      sleep(1);
    }
    if(janus_windows) {
      write_log ("Janus: not all janus_windows could be closed\n");
    }
    
    closepseudodevices ();
    appw_exit ();

#ifdef USE_CYBERGFX
# ifdef USE_CYBERGFX_V41
    if (CybBuffer) {
	FreeVec (CybBuffer);
        CybBuffer = NULL;
    }
# else
    if (CybBitMap) {
	WaitBlit ();
	myFreeBitMap (CybBitMap);
	CybBitMap = NULL;
    }
# endif
#endif
    if (BitMap) {
	WaitBlit ();
	myFreeBitMap (BitMap);
	BitMap = NULL;
    }
    if (TempRPort) {
	FreeVec (TempRPort);
	TempRPort = NULL;
    }
    if (Line) {
	FreeVec (Line);
	Line = NULL;
    }
    if (CM) {
	ReleaseColors();
	CM = NULL;
    }
    if (W) {
	restore_prWindowPtr ();
	CloseWindow (W);
	W = NULL;
    }

    free_pointer ();

    if (!usepub && S) {
	if (!CloseScreen (S)) {
	    gui_message ("Please close all opened windows on UAE's screen.\n");
	    do
		Delay (50);
	    while (!CloseScreen (S));
	}
	S = NULL;
    }
    if (AslBase) {
	CloseLibrary( (void*) AslBase);
	AslBase = NULL;
    }

#if !defined GTKMUI
    if (GadToolsBase) {
	CloseLibrary( (void*) GadToolsBase);
	GadToolsBase = NULL;
    }

    if (GfxBase) {
	CloseLibrary ((void*)GfxBase);
	GfxBase = NULL;
    }
#endif
    if (LayersBase) {
	CloseLibrary (LayersBase);
	LayersBase = NULL;
    }
#if !defined GTKMUI
    if (IntuitionBase) {
	CloseLibrary ((void*)IntuitionBase);
	IntuitionBase = NULL;
    }
#endif
    if (CyberGfxBase) {
	CloseLibrary((void*)CyberGfxBase);
	CyberGfxBase = NULL;
    }
}

/****************************************************************************/

int do_inhibit_frame (int onoff)
{
  AWTRACE("do_inhibit_frame(%d)\n",onoff);
    if (onoff != -1) {
	inhibit_frame = onoff ? 1 : 0;
	if (inhibit_frame)
	    AWTRACE ("display disabled\n");
	else
	    AWTRACE ("display enabled\n");
	set_title ();
    }
    return inhibit_frame;
}

/***************************************************************************/

void graphics_notify_state (int state)
{
  AWTRACE("graphics_notify_state\n");
}

/***************************************************************************
 * This stuff needs an own file, too 
 ***************************************************************************/

UWORD get_LeftEdge(ULONG win) {

  return (UWORD) get_word(win+4); 
}

UWORD get_TopEdge(ULONG win) {

  return (UWORD) get_word(win+6); 
}

UWORD get_Width(ULONG win) {

  return (UWORD) get_word(win+8); 
}

UWORD get_Height(ULONG win) {

  return (UWORD) get_word(win+10); 
}

UWORD get_Flags(ULONG win) {

  return (UWORD) get_word(win+24); 
}

UWORD get_BorderLeft(ULONG win) {

  return (UWORD) get_byte(win+54); 
}

UWORD get_BorderTop(ULONG win) {

  return (UWORD) get_byte(win+55); 
}

UWORD get_BorderRight(ULONG win) {

  return (UWORD) get_byte(win+56); 
}

UWORD get_BorderBottom(ULONG win) {

  return (UWORD) get_byte(win+57); 
}


/* 
 * clone_window_area(jwin, x, y, width, height)
 *
 * update the area in the aros window with the contents of
 * the amigaOS3 window
 *
 * do nothing, if the area does not touch us
 *
 * TODO: jwin->plusx/y for area checks!
 *
 */

BOOL clone_window_area(JanusWin *jwin, 
                       WORD areax, WORD areay, 
		       UWORD areawidth, UWORD areaheight) {
  WORD winx, winy, winxend, winyend, areaxend, areayend;
  //UWORD width, height;
  WORD startx, starty, endx, endy; /* result */
  WORD width;
  struct Window* win;
  ULONG aos3win;

  win=jwin->aroswin;
  aos3win=(ULONG) jwin->aos3win;

  AWTRACE("clone_wia (%3d,%3d,%3d,%3d) (%lx)\n",
           areax,areay,areawidth,areaheight, jwin);

  if(!win || !aos3win) {
    AWTRACE("clone_wia (%3d,%3d,%3d,%3d): win %lx||aos3win %lx is NULL!\n", 
             areax,areay,areawidth,areaheight,win,aos3win);
    return FALSE;
  }

  areaxend=areax+areawidth;
  areayend=areay+areaheight;

  winy=get_TopEdge(aos3win)   + get_BorderTop(aos3win);
  winx=get_LeftEdge(aos3win)  + get_BorderLeft(aos3win);
  winyend=get_TopEdge(aos3win)+ get_Height(aos3win) - get_BorderBottom(aos3win);
  winxend=get_LeftEdge(aos3win) + get_Width(aos3win) - get_BorderRight(aos3win);

  AWTRACE("clone_wia window x,y,xe,ye: %3d,%3d,%3d,%3d\n",
           winx,winy,winxend,winyend);
  AWTRACE("clone_wia area   x,y,xe,ye: %3d,%3d,%3d,%3d\n",
           areax,areay,areaxend,areayend);

  /* window is above or below our area */
  if(winy > areayend || winyend < areay) {
    AWTRACE("clone_wia (%3d,%3d,%3d,%3d): y out of bounds\n", 
             areax,areay,areawidth,areaheight);

    return FALSE;
  }

  /* window is left or right of our area */
  if(winx > areaxend || winxend < areax) {
    AWTRACE("clone_wia (%3d,%3d,%3d,%3d): x out of bounds\n", 
             areax,areay,areawidth,areaheight);

    return FALSE;
  }

  /* seems, as if we have to do something.. */

  startx=MAX(areax, winx);
  starty=MAX(areay, winy);
  endx=MIN(areaxend, winxend);
  endy=MIN(areayend, winyend);

  AWTRACE("clone_wia: startx %3d starty %3d endx %3d endy %3d\n",
           startx,starty,endx,endy);
  AWTRACE("clone_wia: WritePixelArray(x %3d y %3d  width %3d height %3d)\n",
           startx-winx,starty-winy,endx-startx,endy-starty);

  /* this can happen, if we only have a window top border and no
   * window body at all..?
   */
  if((endx-startx < 0) || (endy-starty < 0)) {
    AWTRACE("WARNING: start - end < 0\n");
    return TRUE;
  }

  if(!uae_main_window_closed) {
    width= W->Width;
  }
  else {
    width=picasso_vidinfo.width;
  }

  WritePixelArray (
      picasso_memory, 
      startx, /* src x  */
      starty,  /* src y  */
      get_BytesPerRow(W),         /* srcmod */
      win->RPort, 
      startx - winx, starty - winy,
      endx - startx + jwin->plusx,
      endy - starty + jwin->plusy,
      RECTFMT_RAW
  );
  /* olioli*/
#if 0
  real_start=get_TopEdge(m68k_win)  + get_BorderTop(m68k_win);
  real_start_dest=0;
  real_lines=aros_win->Height;

  WritePixelArray (
      picasso_memory, 
      get_LeftEdge(m68k_win) + get_BorderLeft(m68k_win), /* src x  */
      real_start,  /* src y  */
      get_BytesPerRow(W),                                /* srcmod */
      /*native_window->RPort, */
      aros_win->RPort, 
      /* no WA_GimmeZeroZero
      aros_win->BorderLeft, aros_win->BorderTop,
      aros_win->Width  - aros_win->BorderLeft,
      aros_win->Height - aros_win->BorderTop,
      */
      0, real_start_dest,
      aros_win->Width,
      real_lines,
      RECTFMT_RAW
  );
#endif



  return TRUE;
}
 
/* 
 * clone_area(x, y, width, height)
 * we are on a picasso96 level, we don't have intuition windows
 * here any more, which is a pity. We need to clone activities
 * in our aros windows, which took place in amigaOS3 bitmaps.
 *
 * As it is hard (impossible?) to find out, in which aros window
 * a modification belongs, we just scan for all windows, which
 * cover this area and update the area in those windows. Should
 * be ok, we'll see.
 */
void clone_area(WORD x, WORD y, UWORD width, UWORD height) {
  GSList *elem;

  if(janus_windows) {
    AWTRACE("clone_area(%3d,%3d,%3d,%3d)\n",x,y,width,height);
    ObtainSemaphore(&sem_janus_window_list);
    elem=janus_windows;
    while(elem) {
      AWTRACE("clone_area: jwin %lx\n",elem);
      if(elem->data &&
 	 !(((JanusWin *)elem->data)->delay) &&
	 !(((JanusWin *)elem->data)->dead)  &&
	  ((JanusWin *)elem->data)->aos3win &&
	  ((JanusWin *)elem->data)->aroswin) {

	if(!clone_window_area((JanusWin *)elem->data, x, y, width, height)) {
	  AWTRACE("clone_area(%3d,%3d,%3d,%3d): done nothing\n",x,y,width,height);
	}
      }
      else {
	/* debug only */
	if(elem->data) {
	  if(((JanusWin *)elem->data)->delay) {
	    AWTRACE("clone_area: jwin->data->delay %d\n",((JanusWin *)elem->data)->delay);
	  }
	  else {
	    AWTRACE("clone_area:          ->: %d %d %lx %lx\n",
		   ((JanusWin *)elem->data)->delay,
		   ((JanusWin *)elem->data)->dead,
		   ((JanusWin *)elem->data)->aos3win,
		   ((JanusWin *)elem->data)->aroswin);
	  }
	}
	else {
	  AWTRACE("clone_area: jwin->data 0\n");
	}
      }
      elem=g_slist_next(elem);
    }
    ReleaseSemaphore(&sem_janus_window_list);
  }
}

/*
 * clone_window(m68k_win, aros_win)
 *
 * copy the bitmap of the m68k_win (without aos3.1 borders) to
 * the aros_win bitmap. The aros_win keeps its borders.
 *
 * m68k_win needs to be on the picasso96 wb screen (otherwise not tested).
 *
 */
static void clone_window(ULONG m68k_win, struct Window *aros_win, 
                         int start, int lines) {

  WORD src_y_start;
  WORD src_y_lines;
  WORD real_start;
  WORD real_start_dest;
  WORD real_lines;

  /* only clone, if this window has already an AROS window */
  if(!m68k_win || !aros_win) {
    return;
  }

  /* we only care for visible screen windows */
  if(aros_win->WScreen != IntuitionBase->FirstScreen) {
    AWTRACE("m68kwindow %lx aros_win %lx is not visible\n",m68k_win,aros_win);
    return;
  }

  src_y_start=get_TopEdge(m68k_win)  + get_BorderTop(m68k_win);
  src_y_lines=aros_win->Height;

  AWTRACE("clone_window(%lx,%lx) \n",m68k_win,aros_win);
  AWTRACE("WritePixelArray(%lx, %3d, %3d => %lx, %3d, %3d, ..\n",
		    picasso_memory,
		    get_LeftEdge(m68k_win) + get_BorderLeft(m68k_win), 
		    get_TopEdge(m68k_win)  + get_BorderTop(m68k_win), 
		    //get_BytesPerRow(W),                              
		    aros_win->RPort,
		    aros_win->Width,
		    aros_win->Height);

  /* modified range outside our window, nothing to do */
  if(src_y_start+src_y_lines < start ||
     src_y_start             > start + lines) {
    AWTRACE("CCC 0\n");
    return;
  }

  if(start < src_y_start) {
    /* modified area starts above us */
    real_start=src_y_start; /* clip above */
    real_start_dest=0; 
    if(start + lines > src_y_start + src_y_lines) {
      /* modified area ends below our window */
      real_lines=src_y_lines;
      AWTRACE("CCC 1\n");
    }
    else {
      /* modified area ends inside our window */
      real_lines= lines - (src_y_start - start);
      AWTRACE("CCC 2\n");
    }
  }
  else {
    /* modified are starts inside window */
    real_start=start;
    real_start_dest=start - src_y_start;
    if(start + lines > src_y_start + src_y_lines) {
      /* modified area ends below our window */
      AWTRACE("CCC 3\n");
      real_lines=src_y_start + src_y_lines - start;
    }
    else {
      /* modified area ends inside our window */
      real_lines=lines; /* copy all */
      AWTRACE("CCC 4\n");
    }
  }

  /* TODO: fix above, remove 3 lines below! */
  real_start=get_TopEdge(m68k_win)  + get_BorderTop(m68k_win);
  real_start_dest=0;
  real_lines=aros_win->Height;

  WritePixelArray (
      picasso_memory, 
      get_LeftEdge(m68k_win) + get_BorderLeft(m68k_win), /* src x  */
      real_start,  /* src y  */
      get_BytesPerRow(W),                      /* srcmod */
      /*native_window->RPort, */
      aros_win->RPort, 
      /* no WA_GimmeZeroZero
      aros_win->BorderLeft, aros_win->BorderTop,
      aros_win->Width  - aros_win->BorderLeft,
      aros_win->Height - aros_win->BorderTop,
      */
      0, real_start_dest,
      aros_win->Width,
      real_lines,
      RECTFMT_RAW
  );

}

/***************************************************************************/
/* same as DoMethod(uaedisplay, MUIM_UAEDisplay_Update, start, i)
 * and same as MUIM_Draw
 */

extern ULONG   aos3_task;
extern ULONG   aos3_task_signal;

int o1i_Draw_delay=0;


void o1i_clone_windows(/* hand over start/end koords of changed area?*/) {
  GSList *elem;
  UWORD start, lines;
  ULONG m68k_win;
  struct Window *aros_win;

  /* at startup, sem_janus_windows_list might not be initialized,
   * but it is, if we have at least one element in the
   * list.
   */
  if(janus_windows) {
    ObtainSemaphore(&sem_janus_window_list);
    elem=janus_windows;
    while(elem) {
      if(elem->data &&
 	 !(((JanusWin *)elem->data)->delay) &&
	 !(((JanusWin *)elem->data)->dead)  &&
	  ((JanusWin *)elem->data)->aos3win &&
	  ((JanusWin *)elem->data)->aroswin) {

	m68k_win=(ULONG) ((JanusWin *)elem->data)->aos3win;
	aros_win=((JanusWin *)elem->data)->aroswin;

      	start=get_TopEdge(m68k_win)  + get_BorderTop(m68k_win);
	lines=aros_win->Height;

	clone_window(m68k_win, aros_win, start, lines);

      }
      elem=g_slist_next(elem);
    }
    ReleaseSemaphore(&sem_janus_window_list);
  }
}

#if 0
void o1i_clone_windows_task() {

  while(TRUE) {
    WaitTOF();
    o1i_clone_windows();
  }

}
#endif

//static void o1i_Draw() {
static void o1i_Display_Update(int start,int i) {

  if(!picasso_memory) {
    AWTRACE("ERROR: no picasso_memory!?\n");
    return;
  }

  if(!W) {
    AWTRACE("ERROR: no Window W!?\n");
    return;
  }

  //o1i_clone_windows();
  //clone_area(0, start, MAXWIDTHHEIGHT, i); /* should clip accordingly */

  if(!j_stop_window_update) {
    /* should clip accordingly */
    clone_area(0, 0, MAXWIDTHHEIGHT, MAXWIDTHHEIGHT); 
  }

  if(!uae_main_window_closed) {
    WritePixelArray (
	picasso_memory, 0, start, get_BytesPerRow(W),
	W->RPort, 
	W->BorderLeft, W->BorderTop + start,
	W->Width - W->BorderLeft - W->BorderRight, 
	W->BorderTop + start + i,
	RECTFMT_RAW
    );
  }
}

int aros_daemon_runing() {
  return (aos3_task && aos3_task_signal);
}

/* 
 * in e-uae handle_events always handle the events of the main uae
 * window. j-uae has quite some windows more ;).
 *
 * if we have integrated windows, this is no problem at all. But
 * for custom screens, we need to call the according handle_custom_events_W.
 */
void handle_events(void) {
  AWTRACE("handle_events: dispatcher\n", W);

  if(custom_screen_active==NULL) {
    AWTRACE("custom_screen_active==NULL\n");
    handle_events_W(W);

  }
  else {
    /* custom screens only have one window */
    AWTRACE("custom screen %lx, window %lx\n", 
             custom_screen_active->arosscreen, 
             custom_screen_active->arosscreen->FirstWindow);
    handle_events_W(custom_screen_active->arosscreen->FirstWindow);
  }
}

/***************************************************
 * handle_events_W
 *
 * works with a local W, so it can be called
 * for a window on our own custom screen, for 
 * example
 ***************************************************/
void handle_events_W(struct Window *W) {

    struct IntuiMessage *msg;
    int dmx, dmy, mx, my, class, code, qualifier;

    AWTRACE("W: %lx\n", W);

   /* this function is called at each frame, so: */
    ++frame_num;       /* increase frame counter */
#if 0
    save_frame();      /* possibly save frame    */
#endif

#ifdef DEBUGGER
    /*
     * This is a hack to simulate ^C as is seems that break_handler
     * is lost when system() is called.
     */
    if (SetSignal (0L, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D) &
		(SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_D)) {
	activate_debugger ();
    }
#endif

    #ifdef PICASSO96
    if(aos3_task && aos3_task_signal) {
      AWTRACE("send signal to Wait of janusd (%lx)\n", aos3_task);
      uae_Signal(aos3_task, aos3_task_signal);
    }

    if (screen_is_picasso) {
        int i;

	//AWTRACE("handle_events->screen_is_picasso\n");

        picasso_invalid_lines[picasso_invalid_end+1] = 0;
        picasso_invalid_lines[picasso_invalid_end+2] = 1;

    	//o1i_Display_Update(0,0);

	/* o1i TODO: remove loops?*/
	for (i = picasso_invalid_start; i < picasso_invalid_end;)
	{
	    int start = i;

            for (;picasso_invalid_lines[i]; i++)
	        picasso_invalid_lines[i] = 0;

            //DoMethod(uaedisplay, MUIM_UAEDisplay_Update, start, i);
	    //AWTRACE("MUIM_UAEDisplay_Update TODO\n");
	    o1i_Display_Update(start,i);

	    for (; !picasso_invalid_lines[i]; i++);
	}

	picasso_invalid_start = picasso_vidinfo.height + 1;
	picasso_invalid_end   = -1;
    }
    #endif

    gui_handle_events();
    while ((msg = (struct IntuiMessage*) GetMsg (W->UserPort))) {
	class     = msg->Class;
	code      = msg->Code;
	dmx       = msg->MouseX;
	dmy       = msg->MouseY;
	mx        = msg->IDCMPWindow->MouseX; // Absolute pointer coordinates
	my        = msg->IDCMPWindow->MouseY; // relative to the window
	qualifier = msg->Qualifier;

	ReplyMsg ((struct Message*)msg);

	AWTRACE("W: %lx (%s)\n",W, W->Title);

	switch (class) {
	    case IDCMP_NEWSIZE:
		do_inhibit_frame ((W->Flags & WFLG_ZOOMED) ? 1 : 0);
		break;

	    case IDCMP_REFRESHWINDOW:
		if (use_delta_buffer) {
		    /* hack: this forces refresh */
		    uae_u8 *ptr = oldpixbuf;
		    int i, len = gfxvidinfo.width;
		    len *= gfxvidinfo.pixbytes;
		    for (i=0; i < currprefs.gfx_height_win; ++i) {
			ptr[00000] ^= 255;
			ptr[len-1] ^= 255;
			ptr += gfxvidinfo.rowbytes;
		    }
		}
		BeginRefresh (W);
		flush_block (0, currprefs.gfx_height_win - 1);
		EndRefresh (W, TRUE);
		break;

	    case IDCMP_CLOSEWINDOW:
		uae_quit ();
		break;

	    case IDCMP_RAWKEY: {
		int keycode = code & 127;
		int state   = code & 128 ? 0 : 1;
		int ievent;

		if ((qualifier & IEQUALIFIER_REPEAT) == 0) {
		    /* We just want key up/down events - not repeats */
		    if ((ievent = match_hotkey_sequence (keycode, state)))
			handle_hotkey_event (ievent, state);
		    else
			inputdevice_do_keyboard (keycode, state);
		}
		break;
	     }

	    case IDCMP_MOUSEMOVE:
	      /* classic mouse move, if either option is disabled or janusd is not (yet) running */
	      if( ((!changed_prefs.jmouse) || (aos3_task==NULL) ) && (!uae_main_window_closed)) {
		AWTRACE("classic mouse move enabled\n");
		setmousestate (0, 0, dmx, 0);
		setmousestate (0, 1, dmy, 0);

		if (usepub) {
		    POINTER_STATE new_state = get_pointer_state (W, mx, my);
		    if (new_state != pointer_state) {
			pointer_state = new_state;
			if (pointer_state == INSIDE_WINDOW)
			    hide_pointer (W);
			else
			    show_pointer (W);
		    }
		}
	      }
	      else {
		AWTRACE("classic mouse move disabled ((%d || %d) && %d)\n", (!changed_prefs.jmouse), (aos3_task==NULL), (!uae_main_window_closed));
	      }
      	      break;

	    case IDCMP_MOUSEBUTTONS:
		if (code == SELECTDOWN) setmousebuttonstate (0, 0, 1);
		if (code == SELECTUP)   setmousebuttonstate (0, 0, 0);
		if (code == MIDDLEDOWN) setmousebuttonstate (0, 2, 1);
		if (code == MIDDLEUP)   setmousebuttonstate (0, 2, 0);
		if (code == MENUDOWN)   setmousebuttonstate (0, 1, 1);
		if (code == MENUUP)     setmousebuttonstate (0, 1, 0);
      	      break;

	    /* Those 2 could be of some use later. */
	    case IDCMP_DISKINSERTED:
		/*printf("diskinserted(%d)\n",code);*/
		break;

	    case IDCMP_DISKREMOVED:
		/*printf("diskremoved(%d)\n",code);*/
		break;

	    case IDCMP_ACTIVEWINDOW:
		/* When window regains focus (presumably after losing focus at some
		 * point) UAE needs to know any keys that have changed state in between.
		 * A simple fix is just to tell UAE that all keys have been released.
		 * This avoids keys appearing to be "stuck" down.
		 */
		AWTRACE("IDCMP_ACTIVEWINDOW(%lx)\n", W);
		inputdevice_acquire ();
		inputdevice_release_all_keys ();
		reset_hotkeys ();
		copy_clipboard_to_amigaos();
		break;

	    case IDCMP_INACTIVEWINDOW:
		AWTRACE("IDCMP_INACTIVEWINDOW\n");
		AWTRACE("IntuitionBase->ActiveWindow: %lx (%s)\n",IntuitionBase->ActiveWindow, IntuitionBase->ActiveWindow->Title);
		copy_clipboard_to_aros();
		inputdevice_unacquire ();
		break;

	    case IDCMP_INTUITICKS:
#ifdef __amigaos4__ 
		grabTicks--;
		if (grabTicks < 0) {
		    grabTicks = GRAB_TIMEOUT;
		    #ifdef __amigaos4__ 
			if (mouseGrabbed)
			    grab_pointer (W);
		    #endif
		}
#endif
		break;

	    default:
		write_log ("Unknown event class: %x\n", class);
		AWTRACE("Unknown event class: %x\n", class);
		break;
        }
    }

    appw_events();
}

/***************************************************************************/

int debuggable (void)
{
    return 1;
}

/***************************************************************************/

int mousehack_allowed (void)
{
    return 0;
}

/***************************************************************************/

void LED (int on)
{
}

/***************************************************************************/

/* sam: need to put all this in a separate module */

#ifdef PICASSO96

/* o1i: */

/* WARNING(?): this will not work, if the uae window
 *          is not on the public screen!
 */
static int get_BytesPerPix(struct Window *win) {
  struct Screen *scr;
  int res;

  if(!win) {
    AWTRACE("\nERROR: win is NULL\n");
    kprintf("\nERROR: win is NULL\n");
    return 0;
  }

  scr=win->WScreen;

  if(!GetCyberMapAttr(scr->RastPort.BitMap, CYBRMATTR_ISCYBERGFX)) {
    AWTRACE("\nERROR: !CYBRMATTR_ISCYBERGFX\n");
    kprintf("\nERROR: !CYBRMATTR_ISCYBERGFX\n");
  }

  res=GetCyberMapAttr(scr->RastPort.BitMap, CYBRMATTR_BPPIX);

  return res;
}

/* get_BytesPerRow (TODO ??)
 *
 * some kind of magix, I still don't understand (o1i)
 */
static int get_BytesPerRow(struct Window *win) {
  WORD width;

#if 0
  if(!uae_main_window_closed) {
    width=W->Width;
    AWTRACE("1: %d * %d = %d\n",width,get_BytesPerPix(win),width*get_BytesPerPix(win));
  }
  else {
#endif
    width=picasso_vidinfo.width;
#if 0
    AWTRACE("2: %d * %d = %d\n",picasso_vidinfo.width,get_BytesPerPix(win),picasso_vidinfo.width*get_BytesPerPix(win));
  }
#endif

  return width * get_BytesPerPix(win);
}

static void set_screen_for_picasso(void) {

  WORD width, height;

  AWTRACE("set_screen_for_picasso\n");

  if(!uae_main_window_closed) {
    AWTRACE("  old window      : %d x %d\n",W->Width,W->Height);
    AWTRACE("  resize window to: %d x %d\n",picasso_vidinfo.width,picasso_vidinfo.height);

    AWTRACE("  W->BorderLeft:   %d\n",W->BorderLeft);
    AWTRACE("  W->BorderRight:  %d\n",W->BorderRight);
    AWTRACE("  W->BorderTop:    %d\n",W->BorderTop);
    AWTRACE("  W->BorderBottom: %d\n",W->BorderBottom);

    AWTRACE("  gfx_fullscreen_picasso: %d\n",currprefs.gfx_pfullscreen);
    AWTRACE("  S: %lx\n",S);
    if(currprefs.gfx_pfullscreen || !S) {
      /* center it */
      AWTRACE("  S->Width x S->Height: %d x %d\n",S->Width,S->Height);
      width =picasso_vidinfo.width;
      height=picasso_vidinfo.height;
      ChangeWindowBox(W, (S->Width  - picasso_vidinfo.width )/2,
			 (S->Height - picasso_vidinfo.height)/2,
			 width,
			 height);
    }
    else {
      width =picasso_vidinfo.width  + W->BorderLeft + W->BorderRight;
      height=picasso_vidinfo.height + W->BorderTop  + W->BorderBottom;
      ChangeWindowBox(W, W->LeftEdge, W->TopEdge, width, height);
    }
  }
  else {
    AWTRACE("window is hidden\n");
    width =picasso_vidinfo.width;
    height=picasso_vidinfo.height;
  }

  /* error checks in ChangeWindowBox possible? 
   * But should not be necessary, as we allowed only screenmodes,
   * which are smaller/equal than the wb screen in DX_FillResolutions.
   */

  /* TAKE CARE: ChangeWindowBox is an asynchron call, the window
   *            might still have the old size for a short while!
   */

  picasso_vidinfo.extra_mem = 1;

  picasso_vidinfo.rowbytes = get_BytesPerRow(W);
  picasso_vidinfo.pixbytes = get_BytesPerPix(W);

  if(picasso_memory) {
    AWTRACE("FreeVec(%lx)\n",picasso_memory);
    FreeVec(picasso_memory);
    picasso_memory=NULL;
  }
  picasso_memory=AllocVec(width * height * picasso_vidinfo.pixbytes, 
			  MEMF_CLEAR);

  picasso_invalid_start = picasso_vidinfo.height + 1;
  picasso_invalid_end   = -1;

  AWTRACE("  actual window size : %d x %d\n",W->Width,W->Height);
  AWTRACE("  new window size    : %d x %d\n",width,height);
  AWTRACE("  byte per row       : %d \n",picasso_vidinfo.rowbytes);
  AWTRACE("  byte per pix       : %d \n",picasso_vidinfo.pixbytes);

  /* wait here, until window has right size?? */
  AWTRACE("  set_screen_for_picasso done!\n");
}

/* un_set_screen_for_picasso
 *
 * If we are running rootless, this will not work as expected, as
 * we have no usefull W (this is with size 1x1 hidden and
 * does not get updated etc.
 *
 * So we need some magic here obviously.
 */
static void un_set_screen_for_picasso(void) {

  AWTRACE("entered\n");

  if(!W) {
    AWTRACE("no W !? \n");
    return;
  }

  if(!uae_main_window_closed) {
    AWTRACE("  old window      : %d x %d\n",W->Width,W->Height);
    AWTRACE("  picasso_vidinfo : %d x %d\n",
		    picasso_vidinfo.width,picasso_vidinfo.height);
    AWTRACE("  currprefs_win   : %d x %d\n",
		    currprefs.gfx_width_win,currprefs.gfx_height_win);
    ChangeWindowBox(W, 
		    W->LeftEdge, W->TopEdge,
		    currprefs.gfx_width_win  + W->BorderLeft + W->BorderRight,
		    currprefs.gfx_height_win + W->BorderTop  + W->BorderBottom);

  #if 0
    /* this causes a FreeMem guru? */
    if(gfxvidinfo.bufmem) {
      AWTRACE("un_set_screen_for_picasso: FreeVec(%lx)\n",gfxvidinfo.bufmem);
      FreeVec(gfxvidinfo.bufmem);
      /* too big ..? */
      gfxvidinfo.bufmem=AllocVec(W->Width * W->Height * get_BytesPerPix(W), MEMF_CLEAR);
    }
    #endif
  }
  else {
    AWTRACE(" we are rootless and we need a non-Picasso96 screen!??\n");
    AWTRACE("PRAY!!\n");
    AWTRACE("DIE !?!\n");
  }
}

void gfx_set_picasso_state (int on)
{
    AWTRACE("gfx_set_picasso_state(%d)\n", on);

    if (screen_is_picasso == on )
	return;

    AWTRACE("gfx_set_picasso_state: screen_is_picasso != on\n");
    screen_is_picasso = on;
    if (on)
    {
	AWTRACE("gfx_set_picasso_state: on!\n");
        set_screen_for_picasso();
    }
    else
    {
	AWTRACE("gfx_set_picasso_state: off!\n");
        un_set_screen_for_picasso();
#if 0
        if
	(
	   !SetAttrs
            (
                uaedisplay,
                MUIA_UAEDisplay_Width,  currprefs.gfx_width,
	        MUIA_UAEDisplay_Height, currprefs.gfx_height,
	        TAG_DONE
            )
	)
	{
            AWTRACE ("SetAttrs - 2 - aborting()\n");
	    abort();
	};

	gfxvidinfo.bufmem = (APTR)XGET(uaedisplay, MUIA_UAEDisplay_Memory);
#endif
    }
}

void DX_Invalidate (int first, int last)
{
  //AWTRACE("DX_Invalidate\n");

  if (first < picasso_invalid_start)
      picasso_invalid_start = first;

  if (last > picasso_invalid_end)
      picasso_invalid_end = last;

  memset(&picasso_invalid_lines[first], 1, last-first+1);
}

int DX_BitsPerCannon (void)
{
  AWTRACE("DX_BitsPerCannon\n");
    return 8;
}

void DX_SetPalette (int start, int count)
{
  AWTRACE("DX_SetPalette\n");
}

/* TODO: sync this with picasso96.c !! */

int DX_FillResolutions (uae_u16 *ppixel_format)
{
    int count = 0;
    struct Screen *screen;
    ULONG j;

    AWTRACE("DX_FillResolutions(%d)\n",ppixel_format);

    static struct
    {
        int width, height;
    } modes [] =
    {
#if 0
        { 320,  200 },
	{ 320,  240 },
	{ 320,  256 },
	{ 640,  200 },
	{ 640,  240 },
	{ 640,  256 },
	{ 640,  400 },
	{ 640,  480 },
	{ 640,  512 },
	{ 800,  600 },
	{ 1024, 768 }
#endif
    {  320, 200 },
    {  320, 240 },
    {  640, 400 },
    {  640, 480 },
    {  800, 600 },
    { 1024, 768 },
    { 1152, 864 },
    { 1280,1024 },
    { 1600,1280 },

    /* new modes */
 
    {  704, 480 },
    {  704, 576 },
    {  720, 480 },
    {  720, 576 },
    {  768, 483 },
    {  768, 576 },
    {  800, 480 },
    {  848, 480 },
    {  854, 480 },
    {  948, 576 },
    { 1024, 576 },
    { 1152, 768 },
    { 1152, 864 },
    { 1280, 720 },
    { 1280, 768 },
    { 1280, 800 },
    { 1280, 854 },
    { 1280, 960 },
    { 1366, 768 },
    { 1440, 900 },
    { 1440, 960 },
    { 1600,1200 },
    { 1680,1050 },
    { 1920,1080 },
    { 1920,1200 },
    { 2048,1152 },
    { 2048,1536 },
    { 2560,1600 },
    { 2560,2048 },
    {  400, 300 },
    {  512, 384 },
    {  640, 432 },
    { 1360, 768 },
    { 1360,1024 },
    { 1400,1050 },
    { 1792,1344 },
    { 1800,1440 },
    { 1856,1392 },
    { 1920,1440 },
    {  480, 360 },
    {  640, 350 },
    { 1600, 900 },
    {  960, 600 },
    { 1088, 612 }

    };

    screen=LockPubScreen(NULL);

    if(!screen) {
      write_log("ERROR: no public screen available !?\n");
      return 0;
    }

    //int bpx  = XGET(uaedisplay, MUIA_UAEDisplay_BytesPerPix);
    int bpx  = GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_BPPIX);

    //int maxw = XGET(uaedisplay, MUIA_UAEDisplay_MaxWidth);
    int maxw = screen->Width;
    //int maxh = XGET(uaedisplay, MUIA_UAEDisplay_MaxHeight);
    int maxh = screen->Height;

    write_log("maxheight = %d\n", maxh);
    write_log("maxwidth  = %d\n", maxw);


    for (j = 0; (j < (sizeof(modes)/sizeof(modes[0]))) && (j < MAX_PICASSO_MODES); j++)
    {
        static const int bpx2format[] = {0, RGBFF_CHUNKY, RGBFF_R5G6B5PC, 0, RGBFF_B8G8R8A8};

	if (modes[j].width > maxw || modes[j].height > maxh)
	    continue;

	DisplayModes[count].res.width  = modes[j].width;
        DisplayModes[count].res.height = modes[j].height;
        DisplayModes[count].depth      = bpx;
        DisplayModes[count].refresh    = 75;

        count++;
        *ppixel_format |= bpx2format[bpx];
    }

    UnlockPubScreen(NULL,screen);
    return count;
}

void gfx_set_picasso_modeinfo (int w, int h, int d, int rgbfmt)
{
    AWTRACE("gfx_set_picasso_modeinfo (w: %d, h: %d, d: %d, rgbfmt: %d)\n",w,h,d,rgbfmt);;

    picasso_vidinfo.width = w;
    picasso_vidinfo.height = h;
    picasso_vidinfo.depth = d;
    picasso_vidinfo.rgbformat = (d == 8 ? RGBFB_CHUNKY
				 : d == 16 ? RGBFB_R5G6B5PC
				 : RGBFB_B8G8R8A8);

    if (screen_is_picasso)
        set_screen_for_picasso();
}


uae_u8 *gfx_lock_picasso (void)
{
//    D(bug("gfx_lock_picasso()\n"));
  //AWTRACE("gfx_lock_picasso\n");
    return picasso_memory;
}
void gfx_unlock_picasso (void)
{
  //AWTRACE("gfx_unlock_picasso\n");
//    D(bug("gfx_unlock_picasso()\n"));
}

/* o1i end */
#endif

/***************************************************************************/

static int led_state[5];

#define WINDOW_TITLE PACKAGE_NAME " " PACKAGE_VERSION

static void set_title (void)
{
#if 0
    static char title[80];
    static char ScreenTitle[200];

    if (!usepub)
	return;

    sprintf (title,"%sPower: [%c] Drives: [%c] [%c] [%c] [%c]",
	     inhibit_frame? WINDOW_TITLE " (PAUSED) - " : WINDOW_TITLE,
	     led_state[0] ? 'X' : ' ',
	     led_state[1] ? '0' : ' ',
	     led_state[2] ? '1' : ' ',
	     led_state[3] ? '2' : ' ',
	     led_state[4] ? '3' : ' ');

    if (!*ScreenTitle) {
	sprintf (ScreenTitle,
                 "UAE-%d.%d.%d (%s%s%s)  by Bernd Schmidt & contributors, "
                 "Amiga Port by Samuel Devulder.",
		  UAEMAJOR, UAEMINOR, UAESUBREV,
		  currprefs.cpu_level==0?"68000":
		  currprefs.cpu_level==1?"68010":
		  currprefs.cpu_level==2?"68020":"68020/68881",
		  currprefs.address_space_24?" 24bits":"",
		  currprefs.cpu_compatible?" compat":"");
        SetWindowTitles(W, title, ScreenTitle);
    } else SetWindowTitles(W, title, (char*)-1);
#endif

    const char *title = inhibit_frame ? WINDOW_TITLE " (Display off)" : WINDOW_TITLE;
    SetWindowTitles (W, title, (char*)-1);
}

/****************************************************************************/

void main_window_led (int led, int on)                /* is used in amigui.c */
{
#if 0
    if (led >= 0 && led <= 4)
	led_state[led] = on;
#endif
    set_title ();
}

/****************************************************************************/
/*
 * Routines for OS2.0 (code taken out of mpeg_play by Michael Balzer)
 */
static struct BitMap *myAllocBitMap(ULONG sizex, ULONG sizey, ULONG depth,
                                    ULONG flags, struct BitMap *friend_bitmap)
{
    struct BitMap *bm;

#if !defined __amigaos4__ && !defined __MORPHOS__ && !defined __AROS__
    if (!os39) {
	unsigned long extra = (depth > 8) ? depth - 8 : 0;
	bm = AllocVec (sizeof *bm + (extra * 4), MEMF_CLEAR);
	if (bm) {
	    ULONG i;
	    InitBitMap (bm, depth, sizex, sizey);
	    for (i = 0; i<depth; i++) {
		if (!(bm->Planes[i] = AllocRaster (sizex, sizey))) {
		    while (i--)
			FreeRaster (bm->Planes[i], sizex, sizey);
		    FreeVec (bm);
		    bm = 0;
		    break;
		}
	    }
	}
    } else
#endif
	bm = AllocBitMap (sizex, sizey, depth, flags, friend_bitmap);

    return bm;
}

/****************************************************************************/

static void myFreeBitMap(struct BitMap *bm)
{
#if !defined __amigaos4__ && !defined __MORPHOS__ && !defined __AROS__
    if (!os39) {
	while(bm->Depth--)
	    FreeRaster(bm->Planes[bm->Depth], bm->BytesPerRow*8, bm->Rows);
	FreeVec(bm);
    } else
#endif
	FreeBitMap (bm);

    return;
}

/****************************************************************************/
/*
 * find the best appropriate color. return -1 if none is available
 */
static LONG ObtainColor (ULONG r,ULONG g,ULONG b)
{
    int i, crgb;
    int colors;

    if (os39 && usepub && CM) {
	i = ObtainBestPen (CM, r, g, b,
			   OBP_Precision, (use_approx_color ? PRECISION_GUI
							    : PRECISION_EXACT),
			   OBP_FailIfBad, TRUE,
			   TAG_DONE);
	if (i != -1) {
	    if (maxpen<256)
		pen[maxpen++] = i;
	    else
		i = -1;
        }
        return i;
    }

    colors = is_halfbrite ? 32 : (1 << RPDepth (RP));

    /* private screen => standard allocation */
    if (!usepub) {
	if (maxpen >= colors)
	    return -1; /* no more colors available */
	if (os39)
	    SetRGB32 (&S->ViewPort, maxpen, r, g, b);
	else
	    SetRGB4 (&S->ViewPort, maxpen, r >> 28, g >> 28, b >> 28);
	return maxpen++;
    }

    /* public => find exact match */
    r >>= 28; g >>= 28; b >>= 28;
    if (use_approx_color)
	return get_nearest_color (r, g, b);
    crgb = (r << 8) | (g << 4) | b;
    for (i = 0; i < colors; i++ ) {
	int rgb = GetRGB4 (CM, i);
	if (rgb == crgb)
	    return i;
    }
    return -1;
}

/****************************************************************************/
/*
 * free a color entry
 */
static void ReleaseColors(void)
{
    if (os39 && usepub && CM)
	while (maxpen > 0)
	    ReleasePen (CM, pen[--maxpen]);
    else
	maxpen = 0;
}

/****************************************************************************/

static int DoSizeWindow (struct Window *W, int wi, int he)
{
    register int x,y;
    int ret = 1;

    wi += W->BorderRight + W->BorderLeft;
    he += W->BorderBottom + W->BorderTop;
    x   = W->LeftEdge;
    y   = W->TopEdge;

    if (x + wi >= W->WScreen->Width)  x = W->WScreen->Width  - wi;
    if (y + he >= W->WScreen->Height) y = W->WScreen->Height - he;

    if (x < 0 || y < 0) {
	write_log ("Working screen too small to open window (%dx%d).\n", wi, he);
	if (x < 0) {
	    x = 0;
	    wi = W->WScreen->Width;
	}
	if (y < 0) {
	    y = 0;
	    he = W->WScreen->Height;
	}
	ret = 0;
    }

    x  -= W->LeftEdge;
    y  -= W->TopEdge;
    wi -= W->Width;
    he -= W->Height;

    if (x | y)	 MoveWindow (W, x, y);
    if (wi | he) SizeWindow (W, wi, he);

    return ret;
}

/****************************************************************************/
/* Here lies an algorithm to convert a 12bits truecolor buffer into a HAM
 * buffer. That algorithm is quite fast and if you study it closely, you'll
 * understand why there is no need for MMX cpu to subtract three numbers in
 * the same time. I can think of a quicker algorithm but it'll need 4096*4096
 * = 1<<24 = 16Mb of memory. That's why I'm quite proud of this one which
 * only need roughly 64Kb (could be reduced down to 40Kb, but it's not
 * worth as I use cidx as a buffer which is 128Kb long)..
 ****************************************************************************/

static int dist4 (LONG rgb1, LONG rgb2) /* computes distance very quickly */
{
    int d = 0, t;
    t = (rgb1&0xF00)-(rgb2&0xF00); t>>=8; if (t<0) d -= t; else d += t;
    t = (rgb1&0x0F0)-(rgb2&0x0F0); t>>=4; if (t<0) d -= t; else d += t;
    t = (rgb1&0x00F)-(rgb2&0x00F); t>>=0; if (t<0) d -= t; else d += t;
#if 0
    t = rgb1^rgb2;
    if(t&15) ++d; t>>=4;
    if(t&15) ++d; t>>=4;
    if(t&15) ++d;
#endif
    return d;
}

#define d_dst (00000+(UBYTE*)cidx) /* let's use cidx as a buffer */
#define d_cmd (16384+(UBYTE*)cidx)
#define h_buf (32768+(UBYTE*)cidx)

static int init_ham (void)
{
    int i,t,RGB;

    /* try direct color first */
    for (RGB = 0; RGB < 4096; ++RGB) {
	int c,d;
	c = d = 50;
	for (i = 0; i < 16; ++i) {
	    t = dist4 (i*0x111, RGB);
	    if (t<d) {
		d = t;
		c = i;
	    }
	}
	i = (RGB & 0x00F) | ((RGB & 0x0F0) << 1) | ((RGB & 0xF00) << 2);
	d_dst[i] = (d << 2) | 3; /* the "|3" is a trick to speedup comparison */
	d_cmd[i] = c;		 /* in the conversion process */
    }
    /* then hold & modify */
    for (i = 0; i < 32768; ++i) {
	int dr, dg, db, d, c;
	dr = (i>>10) & 0x1F; dr -= 0x10; if (dr < 0) dr = -dr;
	dg = (i>>5)  & 0x1F; dg -= 0x10; if (dg < 0) dg = -dg;
	db = (i>>0)  & 0x1F; db -= 0x10; if (db < 0) db = -db;
	c  = 0; d = 50;
	t = dist4 (0,  0*256 + dg*16 + db); if (t < d) {d = t; c = 0;}
	t = dist4 (0, dr*256 +  0*16 + db); if (t < d) {d = t; c = 1;}
	t = dist4 (0, dr*256 + dg*16 +  0); if (t < d) {d = t; c = 2;}
	h_buf[i] = (d<<2) | c;
    }
    return 1;
}

/* great algorithm: convert trucolor into ham using precalc buffers */
#undef USE_BITFIELDS
static void ham_conv (UWORD *src, UBYTE *buf, UWORD len)
{
    /* A good compiler (ie. gcc :) will use bfext/bfins instructions */
#ifdef __SASC
    union { struct { unsigned int _:17, r:5, g:5, b:5; } _;
	    int all;} rgb, RGB;
#else
    union { struct { ULONG _:17,r:5,g:5,b:5;} _; ULONG all;} rgb, RGB;
#endif
    rgb.all = 0;
    while(len--) {
        UBYTE c,t;
        RGB.all = *src++;
        c = d_cmd[RGB.all];
        /* cowabonga! */
        t = h_buf[16912 + RGB.all - rgb.all];
#ifndef USE_BITFIELDS
        if(t<=d_dst[RGB.all]) {
	    static int ht[]={32+10,48+5,16+0}; ULONG m;
	    t &= 3; m = 0x1F<<(ht[t]&15);
            m = ~m; rgb.all &= m;
            m = ~m; m &= RGB.all;rgb.all |= m;
	    m >>= ht[t]&15;
	    c = (ht[t]&~15) | m;
        } else {
	    rgb.all = c;
	    rgb.all <<= 5; rgb.all |= c;
	    rgb.all <<= 5; rgb.all |= c;
        }
#else
        if(t<=d_dst[RGB.all]) {
            t&=3;
            if(!t)        {c = 32; c |= (rgb._.r = RGB._.r);}
            else {--t; if(!t) {c = 48; c |= (rgb._.g = RGB._.g);}
            else              {c = 16; c |= (rgb._.b = RGB._.b);} }
        } else rgb._.r = rgb._.g = rgb._.b = c;
#endif
        *buf++ = c;
    }
}

/****************************************************************************/

int check_prefs_changed_gfx (void)
{
   return 0;
}

/****************************************************************************/

void toggle_mousegrab (void)
{
#ifdef __amigaos4__ 
    mouseGrabbed = 1 - mouseGrabbed;
    grabTicks    = GRAB_TIMEOUT;
    if (W)
	grab_pointer (W);
#else
    write_log ("Mouse grab not supported\n");
#endif
}

int is_fullscreen (void)
{
    return 0;
}

int is_vsync (void)
{
    return 0;
}
   
void toggle_fullscreen (void)
{
}

void screenshot (int type)
{
    write_log ("Screenshot not implemented yet\n");
}

/****************************************************************************
 *
 * Mouse inputdevice functions
 */

#define MAX_BUTTONS     3
#define MAX_AXES        3
#define FIRST_AXIS      0
#define FIRST_BUTTON    MAX_AXES

static int init_mouse (void)
{
   return 1;
}

static void close_mouse (void)
{
   return;
}

static int acquire_mouse (unsigned int num, int flags)
{
   return 1;
}

static void unacquire_mouse (unsigned int num)
{
   return;
}

static unsigned int get_mouse_num (void)
{
    return 1;
}

static const char *get_mouse_name (unsigned int mouse)
{
    return "Default mouse";
}

static unsigned int get_mouse_widget_num (unsigned int mouse)
{
    return MAX_AXES + MAX_BUTTONS;
}

static int get_mouse_widget_first (unsigned int mouse, int type)
{
    switch (type) {
        case IDEV_WIDGET_BUTTON:
            return FIRST_BUTTON;
        case IDEV_WIDGET_AXIS:
            return FIRST_AXIS;
    }
    return -1;
}

static int get_mouse_widget_type (unsigned int mouse, unsigned int num, char *name, uae_u32 *code)
{
    if (num >= MAX_AXES && num < MAX_AXES + MAX_BUTTONS) {
        if (name)
            sprintf (name, "Button %d", num + 1 + MAX_AXES);
        return IDEV_WIDGET_BUTTON;
    } else if (num < MAX_AXES) {
        if (name)
            sprintf (name, "Axis %d", num + 1);
        return IDEV_WIDGET_AXIS;
    }
    return IDEV_WIDGET_NONE;
}

static void read_mouse (void)
{
    /* We handle mouse input in handle_events() */
}

struct inputdevice_functions inputdevicefunc_mouse = {
    init_mouse,
    close_mouse,
    acquire_mouse,
    unacquire_mouse,
    read_mouse,
    get_mouse_num,
    get_mouse_name,
    get_mouse_widget_num,
    get_mouse_widget_type,
    get_mouse_widget_first
};

/*
 * Default inputdevice config for mouse
 */
void input_get_default_mouse (struct uae_input_device *uid)
{
    /* Supports only one mouse for now */
    uid[0].eventid[ID_AXIS_OFFSET + 0][0]   = INPUTEVENT_MOUSE1_HORIZ;
    uid[0].eventid[ID_AXIS_OFFSET + 1][0]   = INPUTEVENT_MOUSE1_VERT;
    uid[0].eventid[ID_AXIS_OFFSET + 2][0]   = INPUTEVENT_MOUSE1_WHEEL;
    uid[0].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
    uid[0].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
    uid[0].enabled = 1;
}

/****************************************************************************
 *
 * Keyboard inputdevice functions
 */
static unsigned int get_kb_num (void)
{
    return 1;
}

static const char *get_kb_name (unsigned int kb)
{
    return "Default keyboard";
}

static unsigned int get_kb_widget_num (unsigned int kb)
{
    return 128;
}

static int get_kb_widget_first (unsigned int kb, int type)
{
    return 0;
}

static int get_kb_widget_type (unsigned int kb, unsigned int num, char *name, uae_u32 *code)
{
    // fix me
    *code = num;
    return IDEV_WIDGET_KEY;
}

static int keyhack (int scancode, int pressed, int num)
{
    return scancode;
}

static void read_kb (void)
{
}

static int init_kb (void)
{
    return 1;
}

static void close_kb (void)
{
}

static int acquire_kb (unsigned int num, int flags)
{
    return 1;
}

static void unacquire_kb (unsigned int num)
{
}

struct inputdevice_functions inputdevicefunc_keyboard =
{
    init_kb,
    close_kb,
    acquire_kb,
    unacquire_kb,
    read_kb,
    get_kb_num,
    get_kb_name,
    get_kb_widget_num,
    get_kb_widget_type,
    get_kb_widget_first
};

int getcapslockstate (void)
{
    return 0;
}

void setcapslockstate (int state)
{
}

/****************************************************************************
 *
 * Handle gfx specific cfgfile options
 */

static const char *screen_type[] = { "custom", "public", "ask", 0 };

void gfx_default_options (struct uae_prefs *p)
{
    p->amiga_screen_type     = UAESCREENTYPE_PUBLIC;
    p->amiga_publicscreen[0] = '\0';
    p->amiga_use_dither      = 1;
    p->amiga_use_grey        = 0;
}

void gfx_save_options (FILE *f, const struct uae_prefs *p)
{
    cfgfile_write (f, GFX_NAME ".screen_type=%s\n",  screen_type[p->amiga_screen_type]);
    cfgfile_write (f, GFX_NAME ".publicscreen=%s\n", p->amiga_publicscreen);
    cfgfile_write (f, GFX_NAME ".use_dither=%s\n",   p->amiga_use_dither ? "true" : "false");
    cfgfile_write (f, GFX_NAME ".use_grey=%s\n",     p->amiga_use_grey ? "true" : "false");
}

int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    return (cfgfile_yesno  (option, value, "use_dither",   &p->amiga_use_dither)
	 || cfgfile_yesno  (option, value, "use_grey",	 &p->amiga_use_grey)
         || cfgfile_strval (option, value, "screen_type",  &p->amiga_screen_type, screen_type, 0)
         || cfgfile_string (option, value, "publicscreen", &p->amiga_publicscreen[0], 256)
    );
}

/****************************************************************************/

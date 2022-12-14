/*
 * UAE - The Un*x Amiga Emulator
 *
 * SDL graphics support
 *
 * Copyright 2001 Bernd Lachner (EMail: dev@lachner-net.de)
 * Copyright 2003-2007 Richard Drummond
 * Copyright 2006 Jochen Becher
 * Copyright 2017 Oliver Brunner
 *
 * Partialy based on the UAE X interface (xwin.c)
 *
 * Copyright 1995, 1996 Bernd Schmidt
 * Copyright 1996 Ed Hanway, Andre Beck, Samuel Devulder, Bruno Coste
 * Copyright 1998 Marcus Sundberg
 * DGA support by Kai Kollmorgen
 * X11/DGA merge, hotkeys and grabmouse by Marcus Sundberg
 * OpenGL support by Jochen Becher, Richard Drummond
 */

//#define JUAE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"
#include "keymap.h"
//#include "misc.h"
#include "rtgmodes.h"
#include "sdlkeys_dik.h"
#include "options.h"
#include "sdl_aros.h"
#include "picasso96_aros.h"
#include "gfx.h"

#include <SDL/SDL.h>
#include <SDL/SDL_endian.h>

#ifdef USE_GL
#include <GL/glut.h>
#endif

#define SDLGD(x) 

void inputdevice_release_all_keys (void);

/* internal members */
unsigned int shading_enabled = 0;

extern struct uae_prefs currprefs, changed_prefs;

/* internal prototypes */
void setupExtensions(void);
void setmaintitle (void);
int mousehack_allowed (void);
int is_vsync (void);
uae_u8 *gfx_lock_picasso (bool fullupdate, bool doclear);
void gfx_unlock_picasso (bool dorender);

//#define DEBUG_LOG bug
#define DEBUG_LOG DebOut

struct screen_resolution global_screen_resolution[256];

#if 0
struct gl_buffer_t
{
    GLuint   texture;
    GLsizei  texture_width;
    GLsizei  texture_height;

    GLenum   target;
    GLenum   format;
    GLenum   type;

    uae_u8  *pixels;
    uae_u32  width;
    uae_u32  pitch;
};

void flush_gl_buffer (const struct gl_buffer_t *buffer, int first_line, int last_line);
void render_gl_buffer (const struct gl_buffer_t *buffer, int first_line, int last_line);

#endif /* USE_GL */

//#include "cfgfile.h"
#include "uae.h"
#include "xwin.h"
#include "custom.h"
#include "drawing.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include "debug.h"
#include "picasso96_aros.h"
#include "rtgmodes.h"
#include "inputdevice.h"
//#include "hotkeys.h"

#include "sdlgfx.h"

/* More internal prototypes that need the above includes first */
void gfx_default_options (struct uae_prefs *p);
void gfx_save_options (struct zfile *f, const struct uae_prefs *p);
int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value);

/* More external prototypes that need the above includes first */
extern void setid    (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt);
extern void setid_af (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, int af);

/* Uncomment for debugging output */
//#define DEBUG
#if 0
#ifdef DEBUG
#define DEBUG_LOG write_log
#else
#define DEBUG_LOG(...) { }
#endif
#endif

#ifdef __AROS__
#define DEBUG_LOG DebOut
#endif

SDL_Surface *display = NULL;
SDL_Surface *screen = NULL;

/* Supported SDL screen modes */
#define MAX_SDL_SCREENMODE 32
static SDL_Rect screenmode[MAX_SDL_SCREENMODE];
static int mode_count;

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

#ifdef PICASSO96
extern int screen_was_picasso;
static char picasso_invalid_lines[1201];
static int picasso_has_invalid_lines;
static int picasso_invalid_start, picasso_invalid_stop;
static int picasso_maxw = 0, picasso_maxh = 0;
#endif

static int bitdepth, bytes_per_pixel;

#define MAX_MAPPINGS 256

#define DID_MOUSE 1
#define DID_JOYSTICK 2
#define DID_KEYBOARD 3

#define DIDC_DX 1
#define DIDC_RAW 2
#define DIDC_WIN 3
#define DIDC_CAT 4
#define DIDC_PARJOY 5

#define AXISTYPE_NORMAL 0
#define AXISTYPE_POV_X 1
#define AXISTYPE_POV_Y 2
#define AXISTYPE_SLIDER 3
#define AXISTYPE_DIAL 4

struct didata {
  int type;
  int acquired;
  int priority;
  int superdevice;
//  GUID iguid;
//  GUID pguid;
  TCHAR *name;
  bool fullname;
  TCHAR *sortname;
  TCHAR *configname;
  int vid, pid, mi;

  int connection;
//  LPDIRECTINPUTDEVICE8 lpdi;
//  HANDLE rawinput;
//  HIDP_CAPS hidcaps;
//  HIDP_VALUE_CAPS hidvcaps[MAX_MAPPINGS];
//  PCHAR hidbuffer, hidbufferprev;
//  PHIDP_PREPARSED_DATA hidpreparseddata;
  int maxusagelistlength;
//  PUSAGE_AND_PAGE usagelist, prevusagelist;

  int wininput;
  int catweasel;
  int coop;

//  HANDLE parjoy;
//  PAR_QUERY_INFORMATION oldparjoystatus;

  uae_s16 axles;
  uae_s16 buttons, buttons_real;
  uae_s16 axismappings[MAX_MAPPINGS];
  TCHAR *axisname[MAX_MAPPINGS];
  uae_s16 axissort[MAX_MAPPINGS];
  uae_s16 axistype[MAX_MAPPINGS];
  bool analogstick;

  uae_s16 buttonmappings[MAX_MAPPINGS];
  TCHAR *buttonname[MAX_MAPPINGS];
  uae_s16 buttonsort[MAX_MAPPINGS];
  uae_s16 buttonaxisparent[MAX_MAPPINGS];
  uae_s16 buttonaxisparentdir[MAX_MAPPINGS];

};

static struct didata di_mouse[MAX_INPUT_DEVICES];
static struct didata di_keyboard[MAX_INPUT_DEVICES];
static struct didata di_joystick[MAX_INPUT_DEVICES];
static int num_mouse, num_keyboard, num_joystick;
static int dd_inited, mouse_inited, keyboard_inited, joystick_inited;

#define MAX_KEYCODES 256
static uae_u8 di_keycodes[MAX_INPUT_DEVICES][MAX_KEYCODES];
static int keyboard_german;

/* If we have to lock the SDL surface, then we remember the address
 * of its pixel data - and recalculate the row maps only when this
 * address changes */
static void *old_pixels;
static uae_u8 scrlinebuf[4096 * 4]; /* this is too large, but let's rather play on the safe side here */

static SDL_Color arSDLColors[256];
#ifdef PICASSO96
static SDL_Color p96Colors[256];
#endif
static int ncolors;

static int fullscreen;
static int vsync;
static int mousegrab;
static int mousehack;

static int is_hwsurface;
static int have_rawkeys;
static int refresh_necessary;
static int last_state = -1;

int alt_pressed;
unsigned int mouse_capture;

extern TCHAR config_filename[256];

/*
 * What graphics platform are we running on . . .?
 *
 * Yes, SDL is supposed to abstract away from the underlying
 * platform, but we need to know this to be able to map raw keys
 * and to work around any platform-specific quirks . . .
 *
 * On AROS SDL_VideoDriverName seems not to return a reasonable name .. 
 *
 * not used at the moment
 */
int get_sdlgfx_type (void) {

  char name[16] = "";
  static int driver = SDLGFX_DRIVER_UNKNOWN;
  static int search_done = 0;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  if (!search_done) {
    if (SDL_VideoDriverName (name, sizeof name)) {
      if (strcmp (name, "x11")==0)
          driver = SDLGFX_DRIVER_X11;
      else if (strcmp (name, "dga") == 0)
          driver = SDLGFX_DRIVER_DGA;
      else if (strcmp (name, "svgalib") == 0)
          driver = SDLGFX_DRIVER_SVGALIB;
      else if (strcmp (name, "fbcon") == 0)
          driver = SDLGFX_DRIVER_FBCON;
      else if (strcmp (name, "directfb") == 0)
          driver = SDLGFX_DRIVER_DIRECTFB;
      else if (strcmp (name, "Quartz") == 0)
          driver = SDLGFX_DRIVER_QUARTZ;
      else if (strcmp (name, "bwindow") == 0)
          driver = SDLGFX_DRIVER_BWINDOW;
      else if (strcmp (name, "CGX") == 0)
          driver = SDLGFX_DRIVER_CYBERGFX;
      else if (strcmp (name, "OS4") == 0)
          driver = SDLGFX_DRIVER_AMIGAOS4;
    }
    search_done = 1;

    DEBUG_LOG ("SDL video driver: %s\n", name);
  }

  return driver;
}

STATIC_INLINE unsigned long bitsInMask (unsigned long mask) {

  /* count bits in mask */
  unsigned long n = 0;
  while (mask) {
    n += mask & 1;
    mask >>= 1;
  }
  return n;
}

STATIC_INLINE unsigned long maskShift (unsigned long mask) {

  /* determine how far mask is shifted */
  unsigned long n = 0;
  while (!(mask & 1)) {
    n++;
    mask >>= 1;
  }
  return n;
}

static int get_color (int r, int g, int b, xcolnr *cnp)
{
  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  DEBUG_LOG ("Function: get_color\n");

  arSDLColors[ncolors].r = r << 4;
  arSDLColors[ncolors].g = g << 4;
  arSDLColors[ncolors].b = b << 4;
  *cnp = ncolors++;
  return 1;
}

static void init_colors (void) {

  int i;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  DEBUG_LOG ("Function: init_colors\n");

#ifdef USE_GL
  if (currprefs.use_gl) {
    DEBUG_LOG ("SDLGFX: bitdepth = %d\n", bitdepth);
    if (bitdepth <= 8) {
      write_log ("SDLGFX: bitdepth %d to small\n", bitdepth);
      abort();
    }
  }
#endif /* USE_GL */

  if (bitdepth > 8) {
    red_bits    = bitsInMask (display->format->Rmask);
    green_bits  = bitsInMask (display->format->Gmask);
    blue_bits   = bitsInMask (display->format->Bmask);
    red_shift   = maskShift (display->format->Rmask);
    green_shift = maskShift (display->format->Gmask);
    blue_shift  = maskShift (display->format->Bmask);
    alpha_bits = 0;
    alpha_shift = 0;

    alloc_colors64k (red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, alpha_bits, alpha_shift, alpha, 0);
  } else {
    alloc_colors256 (get_color);
    SDL_SetColors (screen, arSDLColors, 0, 256);
  }
}

/*
 * Test whether the screen mode <width>x<height>x<depth> is
 * available. If not, find a supported screen mode which best
 * matches.
 */
static int find_best_mode (int *width, int *height, int depth, int fullscreen) {

  int found = 0;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  DEBUG_LOG ("Function: find_best_mode(%d,%d,%d)\n", *width, *height, depth);

  /* First test whether the specified mode is supported */
  found = SDL_VideoModeOK (*width, *height, depth, fullscreen ? SDL_FULLSCREEN : 0);

  if (!found && mode_count > 0) {
    /* The specified mode wasn't available, so we'll try and find
     * a supported resolution which best matches it.
     */
    int i;
    write_log ("SDLGFX: Requested mode (%dx%d%d) not available.\n", *width, *height, depth);

    /* Note: the screenmode array should already be sorted from largest to smallest, since
     * that's the order SDL gives us the screenmodes. */
    for (i = mode_count - 1; i >= 0; i--) {
      if (screenmode[i].w >= *width && screenmode[i].h >= *height)
        break;
    }

    /* If we didn't find a mode, use the largest supported mode */
    if (i < 0)
      i = 0;

    *width  = screenmode[i].w;
    *height = screenmode[i].h;
    found   = 1;

    write_log ("SDLGFX: Using mode (%dx%d)\n", *width, *height);
  }
  return found;
}

#ifdef PICASSO96

int picasso_palette (void) {

  //SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  DebOut("entered\n");
  int i = 0, changed = 0;

  for ( ; i < 256; i++) {
    int r = picasso96_state.CLUT[i].Red;
    int g = picasso96_state.CLUT[i].Green;
    int b = picasso96_state.CLUT[i].Blue;
    uae_u32 v = (doMask256 (r, red_bits, red_shift)
            | doMask256 (g, green_bits, green_shift)
            | doMask256 (b, blue_bits, blue_shift))
            | doMask256 (0xff, alpha_bits, alpha_shift);
    if (v !=  picasso_vidinfo.clut[i]) {
      picasso_vidinfo.clut[i] = v;
      changed = 1;
    }
  }
  DebOut("left\n");
  return changed;
}

#endif

/*
 * Build list of full-screen screen-modes supported by SDL
 * with the specified pixel format.
 *
 * Returns a count of the number of supported modes, -1 if any mode is supported,
 * or 0 if there are no modes with this pixel format.
 */
static long find_screen_modes (struct SDL_PixelFormat *vfmt, SDL_Rect *mode_list, struct screen_resolution* globel_screen_resolutions, int mode_list_size) {

  long count = 0;
  SDL_Rect **modes = SDL_ListModes (vfmt, SDL_FULLSCREEN | SDL_HWSURFACE);

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  if (modes != 0 && modes != (SDL_Rect**)-1) {
    unsigned int i;
    int w = -1;
    int h = -1;

    /* Filter list of modes SDL gave us and ignore duplicates */
    for (i = 0; modes[i] && count < mode_list_size; i++) {
      if ( (modes[i]->w != w || modes[i]->h != h) &&
          (modes[i]->w > 320 && modes[i]->h > 240) ){
        mode_list[count].w = w = modes[i]->w;
        mode_list[count].h = h = modes[i]->h;
        global_screen_resolution[count].width =modes[i]->w;
        global_screen_resolution[count].height=modes[i]->h;
        count++;

        write_log ("SDLGFX: Found screenmode: %dx%d.\n", w, h);
        }
    }
  } else {
    count = (long) modes;
  }

  global_screen_resolution[count].width =-1;
  global_screen_resolution[count].height=-1;

  return count;
}

/**
 ** Buffer methods for SDL surfaces that don't need locking
 **/

static int sdl_lock_nolock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  return 1;
}

static void sdl_unlock_nolock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)
}

STATIC_INLINE void sdl_flush_block_nolock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line) {

  SDLGD(
    bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);
    bug("[JUAE:SDL] %s: display 0x%p vidbuf_description @ 0x%p\n", __PRETTY_FUNCTION__, display, gfxinfo);
    bug("[JUAE:SDL] %s: 0, %d ->  %d, %d\n", __PRETTY_FUNCTION__, first_line, currentmode->current_width, last_line - first_line + 1);
  )

  if(first_line >= last_line) {
// warning first_line >= last_line should not happen at all?
    SDLGD(bug("[JUAE:SDL] WARNING: %d >= %d\n", first_line, last_line);)
  }
  else {
    SDL_UpdateRect (display, 0, first_line, currentmode->current_width, last_line - first_line + 1);
  }
}

/**
 ** Buffer methods for SDL surfaces that must be locked
 **/

static int sdl_lock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb) {

  int success = 0;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  DEBUG_LOG ("Function: lock\n");

  if (SDL_LockSurface (display) == 0) {
    /* TODO !? */

#if 0
    gfxinfo->bufmem   = (uae_u8 *)display->pixels;
    gfxinfo->rowbytes = display->pitch;

    if (display->pixels != old_pixels) {
      /* If the address of the pixel data has
       * changed, recalculate the row maps */
      init_row_map ();
      old_pixels = display->pixels;
    }
#endif
    success = 1;
  }
  return success;
}

static void sdl_unlock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  DEBUG_LOG ("Function: unlock\n");

  SDL_UnlockSurface (display);
}

static void sdl_flush_block (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line) {

  //SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  //DEBUG_LOG ("Function: flush_block %d %d\n", first_line, last_line);
  
  TODO();

  SDL_UnlockSurface (display);

  sdl_flush_block_nolock (gfxinfo, vb, first_line, last_line);

  SDL_LockSurface (display);
}

static void sdl_flush_screen_dummy (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line) {

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)
}

//#include "hrtimer.h"

static void sdl_flush_screen_flip (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line) {

  frame_time_t start_time;
  frame_time_t sleep_time;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  SDL_BlitSurface (display, 0, screen,0);

  start_time = read_processor_time ();

  SDL_Flip (screen);

  sleep_time = read_processor_time () - start_time;
  idletime += sleep_time;
}

static void sdl_flush_clear_screen (struct vidbuf_description *gfxinfo, struct vidbuffer *vb) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);)

  DEBUG_LOG ("Function: flush_clear_screen\n");

  if (display) {
    SDL_Rect rect = { 0, 0, (Uint16) display->w, (Uint16) display->h };
    SDL_FillRect (display, &rect, SDL_MapRGB (display->format, 0,0,0));
    SDL_UpdateRect (display, 0, 0, rect.w, rect.h);
  }
}

void flush_screen (struct vidbuffer *vb, int first_line, int last_line) {

  //SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  //DebOut("%d -> %d\n", first_line, last_line);

#ifndef USE_GL
  SDL_UpdateRect (display, 0, first_line, currentmode->current_width, last_line - first_line + 1);
#else
  render_gl_buffer (&glbuffer, first_line, last_line);
#endif
}

static int graphics_setup_success=0;

extern void add_mode (struct MultiDisplay *md, int w, int h, int d, int freq, int lace);

/********************************************************************
 * First we add all SDL native modes (AROS host modes) to the 
 * list of modes supported by our virtual gfx card.
 *
 * Later on in picasso96_alloc2 eventually missing (non-native)
 * modes get added.
 *
 * (At least, this is, how I think it should work. Some 
 *  documentation would be really nice here.)
 ********************************************************************/
void fill_DisplayModes(struct MultiDisplay *md) {
  int result = 0;
  unsigned int i;

  DebOut("md: %p\n", md);

  max_uae_width = 8192;
  max_uae_height = 8192;


  if (SDL_InitSubSystem (SDL_INIT_VIDEO) == 0) {

    const SDL_version   *version = SDL_Linked_Version ();
    const SDL_VideoInfo *info    = SDL_GetVideoInfo ();

    write_log ("SDLGFX: Initialized.\n");
    write_log ("SDLGFX: Using SDL version %d.%d.%d.\n", version->major, version->minor, version->patch);
#ifdef USE_GL
    write_log ("SDLGFX: Using GL version %s\n", glGetString(GL_VERSION));
#endif

    /* Find default display depth */
    bitdepth = info->vfmt->BitsPerPixel;
    bytes_per_pixel = info->vfmt->BytesPerPixel;
    if(bytes_per_pixel==3) {
      /* sorry, this is simply wrong on AROS.. */
      bytes_per_pixel=4;
    }

    write_log ("SDLGFX: Display is %d bits deep.\n", bitdepth);
    write_log ("SDLGFX: Display has %d bytes per pixel.\n", info->vfmt->BytesPerPixel);

    /* Build list of screenmodes */
    DebOut("DisplayModes build list with native modes\n");
    /* called before picasso96_alloc2!! */
    mode_count = find_screen_modes (info->vfmt, &screenmode[0], &global_screen_resolution[0], MAX_SDL_SCREENMODE);

    /* init and terminate list */
    md->DisplayModes = xcalloc (struct PicassoResolution, MAX_PICASSO_MODES);
    md->DisplayModes[0].depth=-1;
    md->DisplayModes[0].residx = -1;

    for (i=0; i<mode_count; i++) {
      add_mode(md, screenmode[i].w, screenmode[i].h, bitdepth, 50, 0);

      DebOut("%dx%d, %d-bit\n", screenmode[i].w, screenmode[i].h, bitdepth);
      _stprintf (md->DisplayModes[i].name, _T("%dx%d, %d-bit"), screenmode[i].w, screenmode[i].h, bitdepth);
    }

    graphics_setup_success=1;
  }

  return;
}

int graphics_setup(void) {

  DebOut("graphics_setup (does nothing)\n");
#ifdef USE_GL
  currprefs.use_gl=false;
#endif

#ifdef PICASSO96
  InitPicasso96 ();
#endif

  /* fill_DisplayModes was hopefully called before! */
  return graphics_setup_success;
}

static int graphics_subinit (void)
{
  DebOut("entered\n");

#ifdef USE_GL

  currprefs.use_gl=true;

  if (currprefs.use_gl) {
    if (graphics_subinit_gl () == 0) {
      DebOut("graphics_subinit_gl failed!\n");
      return 0;
    }
  } else {
#endif /* USE_GL */

    Uint32 uiSDLVidModFlags = 0;
  
    DEBUG_LOG ("Function: graphics_subinit\n");

    if (bitdepth == 8) {
      uiSDLVidModFlags |= SDL_HWPALETTE;
    }
    if (fullscreen) {
      uiSDLVidModFlags |= SDL_FULLSCREEN | SDL_HWSURFACE;
      if (!screen_is_picasso) {
        uiSDLVidModFlags |= SDL_DOUBLEBUF;
      }
  }

  if(bitdepth==24) {
    /* otherwise we get vertical "black stripes" on AROS :( */
    DEBUG_LOG ("force bitdepth of 32 instead of 24\n");
    bitdepth=32;
  }
  DEBUG_LOG ("Resolution     : %d x %d x %d\n", currentmode->current_width, currentmode->current_height, bitdepth);

  screen = SDL_SetVideoMode (currentmode->current_width, currentmode->current_height, bitdepth, uiSDLVidModFlags);

  if (screen == NULL) {
    gui_message ("Unable to set video mode: %s\n", SDL_GetError ());
    return 0;
  } else {
    /* Just in case we didn't get exactly what we asked for . . . */
    fullscreen   = ((screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);
    is_hwsurface = ((screen->flags & SDL_HWSURFACE)  == SDL_HWSURFACE);

    /* We assume that double-buffering is vsynced, but we have no way of
      * knowing if it really is. */
    vsync        = ((screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF);

    /* Are these values what we expected? */
#ifdef PICASSO96
    DEBUG_LOG ("P96 screen?    : %d\n", screen_is_picasso);
#endif
    DEBUG_LOG ("Fullscreen?    : %d\n", fullscreen);
    DEBUG_LOG ("Mouse grabbed? : %d\n", mousegrab);
    DEBUG_LOG ("HW surface?    : %d\n", is_hwsurface);
    DEBUG_LOG ("Must lock?     : %d\n", SDL_MUSTLOCK (screen));
    DEBUG_LOG ("Bytes per Pixel: %d\n", screen->format->BytesPerPixel);
    DEBUG_LOG ("Bytes per Line : %d\n", screen->pitch);


    /* Set up buffer methods */
    if (SDL_MUSTLOCK (screen)) {
      DebOut("SDL_MUSTLOCK!\n");
      gfxvidinfo.drawbuffer.lockscr     = sdl_lock;
      gfxvidinfo.drawbuffer.unlockscr   = sdl_unlock;
      gfxvidinfo.drawbuffer.flush_block = sdl_flush_block;
    } else {
      gfxvidinfo.drawbuffer.lockscr     = sdl_lock_nolock;
      gfxvidinfo.drawbuffer.unlockscr   = sdl_unlock_nolock;
      gfxvidinfo.drawbuffer.flush_block = sdl_flush_block_nolock;
    }
    gfxvidinfo.drawbuffer.flush_clear_screen = sdl_flush_clear_screen;


    if (vsync) {
      display = SDL_CreateRGBSurface(SDL_HWSURFACE, screen->w, screen->h, screen->format->BitsPerPixel, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0);
      gfxvidinfo.drawbuffer.flush_screen = sdl_flush_screen_flip;
    } else {
      display = screen;
      gfxvidinfo.drawbuffer.flush_screen = sdl_flush_screen_dummy;
    }

    gfxvidinfo.drawbuffer.width_allocated =screen->w;
    gfxvidinfo.drawbuffer.height_allocated=screen->h;
    DEBUG_LOG ("Window width   : %d\n", gfxvidinfo.drawbuffer.width_allocated);
    DEBUG_LOG ("Window height  : %d\n", gfxvidinfo.drawbuffer.height_allocated);
#ifdef USE_GL
    DEBUG_LOG ("use_gl         : %d\n", currprefs.use_gl);
#endif

#ifdef PICASSO96
    if (!screen_is_picasso) {
#endif
        /* Initialize structure for Amiga video modes */
      if (is_hwsurface) {
        SDL_LockSurface (display);
        gfxvidinfo.drawbuffer.bufmem    = 0;
        gfxvidinfo.drawbuffer.emergmem  = (uae_u8 *)malloc (display->pitch);
        SDL_UnlockSurface (display);
      }

      if (!is_hwsurface) {
        gfxvidinfo.drawbuffer.bufmem    = (uae_u8 *)display->pixels;
        gfxvidinfo.drawbuffer.emergmem  = 0;
      }
      gfxvidinfo.maxblocklines    = MAXBLOCKLINES_MAX;
      gfxvidinfo.drawbuffer.linemem      = 0;
      gfxvidinfo.drawbuffer.pixbytes     = display->format->BytesPerPixel;
      SDLGD(bug("gfxvidinfo.pixbytes: %d\n", gfxvidinfo.drawbuffer.pixbytes));
      gfxvidinfo.drawbuffer.pixbytes     = 4;
      DebOut("WARNING: force gfxvidinfo.drawbuffer.pixbytes to 4!!\n");

      gfxvidinfo.drawbuffer.rowbytes     = display->pitch;


      SDL_SetColors (display, arSDLColors, 0, 256);

      reset_drawing ();

      /* Force recalculation of row maps - if we're locking */
      old_pixels = (void *)-1;
#ifdef PICASSO96
    } else {
      /* Initialize structure for Picasso96 video modes */
      picasso_vidinfo.rowbytes    = display->pitch;
      picasso_vidinfo.extra_mem   = 1;
      picasso_vidinfo.depth   = bitdepth;
      picasso_has_invalid_lines   = 0;
      picasso_invalid_start   = picasso_vidinfo.height + 1;
      picasso_invalid_stop    = -1;

      memset (picasso_invalid_lines, 0, sizeof picasso_invalid_lines);
    }
#endif
  }


#ifdef USE_GL
  }
#endif /* USE_GL */

  bug("force gfxvidinfo.drawbuffer.pixbytes %d to 4\n", gfxvidinfo.drawbuffer.pixbytes);
  DebOut("WARNING: force gfxvidinfo.drawbuffer.pixbytes to 4!!\n");
  gfxvidinfo.drawbuffer.pixbytes=4;
  /* Set UAE window title and icon name */
  setmaintitle ();

  /* Mouse is now always grabbed when full-screen - to work around
   * problems with full-screen mouse input in some SDL implementations */
  if (fullscreen) {
    SDL_WM_GrabInput (SDL_GRAB_ON);
  }
  else {
    SDL_WM_GrabInput (mousegrab ? SDL_GRAB_ON : SDL_GRAB_OFF);
  }


  /* Hide mouse cursor */
  SDL_ShowCursor ((currprefs.input_magic_mouse_cursor!=1) || fullscreen || mousegrab ? SDL_DISABLE : SDL_ENABLE);

  mousehack = !fullscreen && !mousegrab;

  inputdevice_release_all_keys ();
// warning reset_hotkeys disabled
  //reset_hotkeys ();
  init_colors ();

  return 1;
}

void update_gfxparams(void);

int graphics_init (bool b)
{
  int success = 0;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  DEBUG_LOG ("Function: graphics_init\n");

  if (currprefs.color_mode > 5) {
    write_log ("Bad color mode selected. Using default.\n");
    currprefs.color_mode = 0;
  }

#ifdef PICASSO96
  screen_is_picasso = 0;
  screen_was_picasso = 0;
#endif
  fullscreen = isfullscreen();
  mousegrab = 0;

  fixup_prefs_dimensions (&currprefs);

#if OLD
  //current_width  = currprefs.gfx_size_win.width;
  //current_height = currprefs.gfx_size_win.height;
  current_width  = currprefs.gfx_size_win.width;
  current_height = currprefs.gfx_size_win.height;

  SDLGD(bug("current_width : %d\n", current_width));
  SDLGD(bug("current_height: %d\n", current_height));
  

  if (find_best_mode (&current_width, &current_height, bitdepth, fullscreen)) {
    currprefs.gfx_size_win.width  = current_width;
    currprefs.gfx_size_win.height = current_height;
#else
    update_gfxparams();
#endif

    if (graphics_subinit ()) {
      success = 1;
    }
#if OLD
  }
#endif
  return success;
}

static void graphics_subshutdown (void)
{
  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  DEBUG_LOG ("Function: graphics_subshutdown\n");

#ifdef USE_GL
  if (currprefs.use_gl)
  free_gl_buffer (&glbuffer);
  else
#endif /* USE_GL */
  {
  SDL_FreeSurface (display);
  if (display != screen)
      SDL_FreeSurface (screen);
  }
  display = screen = 0;
  mousehack = 0;

//xfree (gfxvidinfo.emergmem);

#if 0
// This breaks catastrophically on some systems. Better work-around needed.
#ifdef USE_GL
    if (currprefs.use_gl) {
    /*
     * Nasty work-around.
     *
     * We don't seem to be able reset GL attributes such as
     * SDL_GL_DOUBLEBUFFER unless we tear down the video driver.
     */
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    }
#endif
#endif
}

void graphics_leave (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    DEBUG_LOG ("Function: graphics_leave\n");

    graphics_subshutdown ();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    dumpcustom ();
}

void graphics_notify_state (int state)
{
  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  if (last_state != state) {
    last_state = state;
    if (display) {
      setmaintitle ();
    }
  }
}

/* see aros.cpp */
void figure_processor_speed(void);

bool handle_events (void)
{
  SDL_Event rEvent = { SDL_NOEVENT };
  int istest = inputdevice_istest ();
  int scancode     = 0;
  static int cnt1, cnt2;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  while (SDL_PollEvent (&rEvent)) {
    switch (rEvent.type) {
      case SDL_QUIT:
        DEBUG_LOG ("Event: quit\n");
        uae_quit ();
        break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP: {
        int state = (rEvent.type == SDL_MOUSEBUTTONDOWN);
        int buttonno = -1;

        DEBUG_LOG ("Event: mouse button %d %s\n", rEvent.button.button, state ? "down" : "up");

        switch (rEvent.button.button) {
          case SDL_BUTTON_LEFT:      buttonno = 0; break;
          case SDL_BUTTON_MIDDLE:    buttonno = 2; break;
          case SDL_BUTTON_RIGHT:     buttonno = 1; break;
#ifdef SDL_BUTTON_WHEELUP
          case SDL_BUTTON_WHEELUP:   if (state) record_key (0x7a << 1); break;
          case SDL_BUTTON_WHEELDOWN: if (state) record_key (0x7b << 1); break;
#endif
        }
        if (buttonno >= 0) {
          setmousebuttonstate (0, buttonno, rEvent.type == SDL_MOUSEBUTTONDOWN ? 1:0);
        }
        break;
      }

      case SDL_KEYUP:
      case SDL_KEYDOWN: {
        int state = (rEvent.type == SDL_KEYDOWN);
        int keycode;
        int ievent;

        // Hack -- Alt + Tab
/*
        if (rEvent.key.keysym.sym == SDLK_LALT) alt_pressed = rEvent.key.type;
        if (rEvent.key.keysym.sym == SDLK_RALT) alt_pressed = rEvent.key.type;
        if ((rEvent.key.keysym.sym == SDLK_TAB) && (alt_pressed == SDL_KEYDOWN)) {
            alt_pressed = SDL_KEYUP;

            if (mouse_capture) {
                        SDL_WM_GrabInput(SDL_GRAB_ON);
                        SDL_ShowCursor(SDL_DISABLE);
                } else {
                        SDL_WM_GrabInput(SDL_GRAB_OFF);
                        SDL_ShowCursor(SDL_ENABLE);
                }
            break;
        }
*/
#if 0
//fixme
        if (currprefs.map_raw_keys) {
            keycode = rEvent.key.keysym.scancode;
            // Hack - OS4 keyup events have bit 7 set.
#ifdef TARGET_AMIGAOS
            keycode &= 0x7F;
#endif
            modifier_hack (&keycode, &state);
        } else
            keycode = rEvent.key.keysym.sym;
#endif
        keycode = rEvent.key.keysym.sym;
        SDLGD(bug("keycode: %d\n", keycode));

//              write_log ("Event: key: %d to: %d  %s\n", keycode, sdlk2dik (keycode), state ? "down" : "up");
/*              if (!istest)
                scancode = keyhack (keycode, state, 0);
            if (scancode < 0)
                continue;*/
        di_keycodes[0][keycode] = state;
        my_kbd_handler (0, sdlk2dik (keycode), state);
        break;
    }

    case SDL_MOUSEMOTION:

      if (!fullscreen && !mousegrab) {
        setmousestate (0, 0,rEvent.motion.x, 1);
        setmousestate (0, 1,rEvent.motion.y, 1);
        //DEBUG_LOG ("Event: mouse motion abs (%d, %d)\n", rEvent.motion.x, rEvent.motion.y);
      } else {
        setmousestate (0, 0, rEvent.motion.xrel, 0);
        setmousestate (0, 1, rEvent.motion.yrel, 0);
        //DEBUG_LOG ("Event: mouse motion rel (%d, %d)\n", rEvent.motion.xrel, rEvent.motion.yrel);
      }
      break;

    case SDL_ACTIVEEVENT:
      if (rEvent.active.state & SDL_APPINPUTFOCUS && !rEvent.active.gain) {
        DEBUG_LOG ("Lost input focus\n");
        inputdevice_release_all_keys ();
        // warning reset_hotkeys disabled
        //reset_hotkeys ();
      }
      break;

      case SDL_VIDEOEXPOSE:
        notice_screen_contents_lost ();
        break;
    } /* end switch() */
  } /* end while() */

//abant
  inputdevicefunc_keyboard.read ();
  inputdevicefunc_mouse.read ();
  inputdevicefunc_joystick.read ();
  inputdevice_handle_inputcode ();
  check_prefs_changed_gfx ();

#ifdef PICASSO96
# ifdef USE_GL
  if (!currprefs.use_gl) {
# endif /* USE_GL */
    if (screen_is_picasso && refresh_necessary) {
      SDL_UpdateRect (screen, 0, 0, picasso_vidinfo.width, picasso_vidinfo.height);
      refresh_necessary = 0;
      memset (picasso_invalid_lines, 0, sizeof picasso_invalid_lines);
    } else if (screen_is_picasso && picasso_has_invalid_lines) {
      int i;
      int strt = -1;
# define OPTIMIZE_UPDATE_RECS
# ifdef OPTIMIZE_UPDATE_RECS
      static SDL_Rect *updaterecs = 0;
      static int updaterecssize = 0;
      int urc = 0;

      if (picasso_vidinfo.height / 2 + 1 > updaterecssize) {
      xfree (updaterecs);
      updaterecssize = picasso_vidinfo.height / 2 + 1;
      updaterecs = (SDL_Rect *) calloc (updaterecssize, sizeof (SDL_Rect));
      }
# endif
      picasso_invalid_lines[picasso_vidinfo.height] = 0;
      for (i = picasso_invalid_start; i < picasso_invalid_stop + 2; i++) {
        if (picasso_invalid_lines[i]) {
          picasso_invalid_lines[i] = 0;
          if (strt != -1)
          continue;
          strt = i;
        } else {
          if (strt == -1)
            continue;
# ifdef OPTIMIZE_UPDATE_RECS
          updaterecs[urc].x = 0;
          updaterecs[urc].y = strt;
          updaterecs[urc].w = picasso_vidinfo.width;
          updaterecs[urc].h = i - strt;
          urc++;
# else
          SDL_UpdateRect (screen, 0, strt, picasso_vidinfo.width, i - strt);
# endif
          strt = -1;
        }
      }
      if (strt != -1)
        abort ();
# ifdef OPTIMIZE_UPDATE_RECS
      SDL_UpdateRects (screen, urc, updaterecs);
# endif
    }
    picasso_has_invalid_lines = 0;
    picasso_invalid_start = picasso_vidinfo.height + 1;
    picasso_invalid_stop = -1;
# ifdef USE_GL
  } else {
    if (screen_is_picasso && refresh_necessary) {

      flush_gl_buffer (&glbuffer, 0, picasso_vidinfo.height - 1);
      render_gl_buffer (&glbuffer, 0, picasso_vidinfo.height - 1);

      glFlush ();
      SDL_GL_SwapBuffers ();

      refresh_necessary = 0;
      memset (picasso_invalid_lines, 0, sizeof picasso_invalid_lines);
  } else if (screen_is_picasso && picasso_has_invalid_lines) {
    int i;
    int strt = -1;

    picasso_invalid_lines[picasso_vidinfo.height] = 0;
    for (i = picasso_invalid_start; i < picasso_invalid_stop + 2; i++) {
      if (picasso_invalid_lines[i]) {
        picasso_invalid_lines[i] = 0;
        if (strt != -1)
        continue;
        strt = i;
      } else {
        if (strt != -1) {
        flush_gl_buffer (&glbuffer, strt, i - 1);
        strt = -1;
        }
      }
    }
    if (strt != -1)
      abort ();

    render_gl_buffer (&glbuffer, picasso_invalid_start, picasso_invalid_stop);

    glFlush();
    SDL_GL_SwapBuffers();
  }

  picasso_has_invalid_lines = 0;
  picasso_invalid_start = picasso_vidinfo.height + 1;
  picasso_invalid_stop = -1;
  }
#endif /* USE_GL */
#endif /* PICASSSO96 */

/*
  return pause_emulation != 0;
*/


  cnt1--;
  if (cnt1 <= 0) {
    figure_processor_speed ();
    flush_log ();
    cnt1 = 50 * 5;
  }

  return 0;
}

static void switch_keymaps (void) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

#if 0
    if (currprefs.map_raw_keys) {
        if (have_rawkeys) {
            set_default_hotkeys (get_default_raw_hotkeys ());
            write_log ("Using raw keymap\n");
        } else {
            currprefs.map_raw_keys = changed_prefs.map_raw_keys = 0;
            write_log ("Raw keys not supported\n");
        }
    }
    if (!currprefs.map_raw_keys) {
        set_default_hotkeys (get_default_cooked_hotkeys ());
        write_log ("Using cooked keymap\n");
    }
#endif
}

int check_prefs_changed_gfx (void) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

#if 0
  if (changed_prefs.map_raw_keys != currprefs.map_raw_keys) {
    switch_keymaps ();
    currprefs.map_raw_keys = changed_prefs.map_raw_keys;
  }
#endif

  if (changed_prefs.gfx_size_win.width        != currprefs.gfx_size_win.width
      || changed_prefs.gfx_size_win.height    != currprefs.gfx_size_win.height
      || changed_prefs.gfx_size_fs.width      != currprefs.gfx_size_fs.width
      || changed_prefs.gfx_size_fs.height     != currprefs.gfx_size_fs.height) {
      fixup_prefs_dimensions (&changed_prefs);
  } else if (changed_prefs.gfx_lores_mode == currprefs.gfx_lores_mode
      && changed_prefs.gfx_vresolution    == currprefs.gfx_vresolution
      && changed_prefs.gfx_xcenter        == currprefs.gfx_xcenter
      && changed_prefs.gfx_ycenter        == currprefs.gfx_ycenter) {
      return 0;
  }

  DEBUG_LOG ("Function: check_prefs_changed_gfx\n");
    DebOut ("Function: check_prefs_changed_gfx\n");

#ifdef PICASSO96
    if (!screen_is_picasso)
        graphics_subshutdown ();
#endif

  currprefs.gfx_size_win.width    = changed_prefs.gfx_size_win.width;
  currprefs.gfx_size_win.height   = changed_prefs.gfx_size_win.height;
  currprefs.gfx_size_fs.width = changed_prefs.gfx_size_fs.width;
  currprefs.gfx_size_fs.height    = changed_prefs.gfx_size_fs.height;
  currprefs.gfx_lores_mode    = changed_prefs.gfx_lores_mode;
  currprefs.gfx_vresolution   = changed_prefs.gfx_vresolution;
  currprefs.gfx_xcenter       = changed_prefs.gfx_xcenter;
  currprefs.gfx_ycenter       = changed_prefs.gfx_ycenter;
#if 0
  currprefs.monitoremu        = changed_prefs.monitoremu;
#endif
  currprefs.leds_on_screen    = changed_prefs.leds_on_screen;

#ifdef PICASSO96
  if (!screen_is_picasso)
#endif
    graphics_subinit ();

  return 0;
}

int debuggable (void) {

    return 1;
}

int mousehack_allowed (void) {

    return mousehack;
}

void LED (int on) {

}

#ifdef PICASSO96
/* TODO: x and width */

void DX_Invalidate (int x, int y, int width, int height) {

  int last, lastx;

  DEBUG_LOG ("Function: DX_Invalidate(x %d, y %d, width %d, height %d)\n", x, y, width, height);

  if (width == 0 || height == 0)
    return;
  if (y < 0 || height < 0) {
    y = 0;
    height = picasso_vidinfo.height;
  }
  if (x < 0 || width < 0) {
    x = 0;
    width = picasso_vidinfo.width;
  }
  last = y + height - 1;
  lastx = x + width - 1;
#if 0
  /* new */
  p96_double_buffer_first = y;
  p96_double_buffer_last  = last;
  p96_double_buffer_firstx = x;
  p96_double_buffer_lastx = lastx;
  p96_double_buffer_needs_flushing = 1;
#endif

  picasso_has_invalid_lines = 1;
  picasso_invalid_start = y;
  picasso_invalid_stop = last;
}

static int palette_update_start = 256;
static int palette_update_end   = 0;

void DX_SetPalette (int start, int count) {

  DEBUG_LOG ("Function: DX_SetPalette (start %d, count %d)\n", start, count);

#if 0
    if (! screen_is_picasso || picasso96_state.RGBFormat != RGBFB_CHUNKY) {
      DebOut("!screen_is_picasso: %d || picasso96_state.RGBFormat %d != %d\n", screen_is_picasso, picasso96_state.RGBFormat, RGBFB_CHUNKY);
        return;
    }
#endif
  if (!screen_is_picasso) {
    DebOut("nothing to do, !screen_is_picasso\n");
    return;
  }

  DebOut("picasso_vidinfo.pixbytes: %d\n", picasso_vidinfo.pixbytes);
  if (picasso_vidinfo.pixbytes != 1) {
    /* This is the case when we're emulating a 256 color display. */
    while (count-- > 0) {
      int r = picasso96_state.CLUT[start].Red;
      int g = picasso96_state.CLUT[start].Green;
      int b = picasso96_state.CLUT[start].Blue;
      picasso_vidinfo.clut[start++] = (doMask256 (r, red_bits, red_shift) | doMask256 (g, green_bits, green_shift) | doMask256 (b, blue_bits, blue_shift));
        }
    } else {
      int i;
      for (i = start; i < start+count && i < 256;  i++) {
        p96Colors[i].r = picasso96_state.CLUT[i].Red;
        p96Colors[i].g = picasso96_state.CLUT[i].Green;
        p96Colors[i].b = picasso96_state.CLUT[i].Blue;
      }
      SDL_SetColors (screen, &p96Colors[start], start, count);
  }

  DebOut("done\n");
}

void DX_SetPalette_vsync(void) {

  if (palette_update_end > palette_update_start) {
      DX_SetPalette (palette_update_start, palette_update_end - palette_update_start);
      palette_update_end   = 0;
      palette_update_start = 0;
  }
}
#endif

void DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color) {

  DebOut("dstx %d, dsty %d, width %d, height %d, color %lx\n", dstx, dsty, width, height, color);
  SDLGD(bug("entered\n"));
#ifdef USE_GL /* TODO think about optimization for GL */
  if (!currprefs.use_gl) {
#endif /* USE_GL */
    SDL_Rect rect = {(Sint16) dstx, (Sint16) dsty, (Uint16) width, (Uint16) height};

    DEBUG_LOG ("DX_Fill (x:%d y:%d w:%d h:%d color=%08x)\n", dstx, dsty, width, height, color);

    if (SDL_FillRect (screen, &rect, color) == 0) {
      DX_Invalidate (0, dsty, width, height - 1);
    }
#ifdef USE_GL
  }
#endif /* USE_GL */
  return;
}

#if 0
int DX_Blit (int srcx, int srcy, int dstx, int dsty, int width, int height, BLIT_OPCODE opcode)
{
    /* not implemented yet */
    return 0;
}
#endif

static void set_window_for_picasso (void)
{
  TODO();
  SDLGD(bug("entered\n"));
  DEBUG_LOG ("Function: set_window_for_picasso\n");

  if (screen_was_picasso && currentmode->current_width == picasso_vidinfo.width && currentmode->current_height == picasso_vidinfo.height)
    return;

  screen_was_picasso = 1;
  graphics_subshutdown();
  currentmode->current_width  = picasso_vidinfo.width;
  currentmode->current_height = picasso_vidinfo.height;
  graphics_subinit();
}

void clearscreen (void) {
  Uint32 black;

  DebOut("clear screen ..\n");
  black=SDL_MapRGB(screen->format, 0, 0, 0);

  SDL_FillRect(screen, NULL, black);
  SDL_Flip(screen);
}

static void updatemodes (void) {
  DebOut("currentmode->native_width:   %d\n", currentmode->native_width);
  DebOut("currentmode->native_height:  %d\n", currentmode->native_height);
  DebOut("currentmode->current_width:  %d\n", currentmode->current_width);
  DebOut("currentmode->current_height: %d\n", currentmode->current_height);

  /* native_width/height => screen/window dimensions of host display */
  /* current_width/height => screen/window dimensions of emulated display */

  currentmode->native_width = currentmode->current_width;
  currentmode->native_height = currentmode->current_height;
}


void updatedisplayarea (void) {

  DebOut("entered\n");

  if (!screen) {
    DebOut("screen is NULL\n");
    return;
  }

  SDL_Flip(screen);
  /* right thing to do here? */
  DX_Invalidate(0, 0, -1, -1);
}

bool target_graphics_buffer_update (void) {

  static bool graphicsbuffer_retry;
  int w, h;

  DebOut("entered\n");
  
  graphicsbuffer_retry = false;
  if (screen_is_picasso) {
    w = picasso96_state.Width > picasso_vidinfo.width ? picasso96_state.Width : picasso_vidinfo.width;
    h = picasso96_state.Height > picasso_vidinfo.height ? picasso96_state.Height : picasso_vidinfo.height;
  } else {
    struct vidbuffer *vb = gfxvidinfo.drawbuffer.tempbufferinuse ? &gfxvidinfo.tempbuffer : &gfxvidinfo.drawbuffer;
    gfxvidinfo.outbuffer = vb;
    w = vb->outwidth;
    h = vb->outheight;
  }
  
  /* WinUAE sets up the scaling here */
#if 0
  if (currentmode->flags & DM_SWSCALE) {
    DebOut("WARNING: DM_SWSCALE not supported\n");
  }
#endif

  /* clear screen */
  clearscreen();

  write_log (_T("Buffer size (%d*%d) %s\n"), w, h, screen_is_picasso ? _T("RTG") : _T("Native"));
  DebOut("Buffer size (%d*%d) %s\n", w, h, screen_is_picasso ? _T("RTG") : _T("Native"));

  updatedisplayarea();
  return TRUE;
}


void gfx_set_picasso_modeinfo (uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{

#if 0
  if(!screen_is_picasso) {
    DebOut("not screen_is_picasso\n");
    return;
  }
#endif

  clearscreen();
  gfx_set_picasso_colors(rgbfmt);
  updatemodes();

  TODO();
  DEBUG_LOG ("Function: gfx_set_picasso_modeinfo w: %i h: %i depth: %i rgbfmt: %i\n", w, h, depth, rgbfmt);

  picasso_vidinfo.width = w;
  picasso_vidinfo.height = h;
  picasso_vidinfo.depth = depth;
  DebOut("bytes_per_pixel: %d\n", bytes_per_pixel);
  picasso_vidinfo.pixbytes = bytes_per_pixel;
  DebOut("gfxvidinfo.pixbytes: %d\n", picasso_vidinfo.pixbytes);
  SDLGD(bug("gfxvidinfo.pixbytes: %d\n", picasso_vidinfo.pixbytes));
  DebOut("WARNING: force gfxvidinfo.drawbuffer.pixbytes to 4!!\n");
  picasso_vidinfo.pixbytes = 4;
  if (screen_is_picasso) {
    set_window_for_picasso();
  }
}

/* Color management */

//static xcolnr xcol8[4096];

void gfx_set_picasso_colors (RGBFTYPE rgbfmt)
{
  alloc_colors_picasso (red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt);
}

void gfx_set_picasso_state (int on)
{
  DEBUG_LOG ("Function: gfx_set_picasso_state: %d\n", on);

  if (on == screen_is_picasso) {
    return;
  }

  /* We can get called by drawing_init() when there's
   * no window opened yet... */
  if (display == 0) {
    return;
  }

  graphics_subshutdown ();
  screen_was_picasso = screen_is_picasso;
  DebOut("screen_was_picasso: %d\n", screen_was_picasso);
  screen_is_picasso = on;
  DebOut("screen_is_picasso: %d\n", screen_is_picasso);

// warning Set height, width for Amiga gfx
#if 0
  if (on) {
#endif
      // Set height, width for Picasso gfx
#if OLD
  currentmode->current_width  = picasso_vidinfo.width;
  currentmode->current_height = picasso_vidinfo.height;
#else
  update_gfxparams();
#endif
      graphics_subinit ();
#if 0
  } else {
      // Set height, width for Amiga gfx
      current_width  = gfxvidinfo.width_allocated;
      current_height = gfxvidinfo.height_allocated;
      current_width  = uaemode.xres;
      current_height = uaemode.yres;
      graphics_subinit ();
  }
#endif

  if (on)
      DX_SetPalette (0, 256);
}

/*******************************************************************
 * lock host window system (here: sdl) and return pointer to
 * pixel data
 *******************************************************************/
uae_u8 *gfx_lock_picasso (bool fullupdate, bool doclear) {

  //DEBUG_LOG ("Function: gfx_lock_picasso\n");

#ifdef USE_GL
    if (!currprefs.use_gl) {
#endif /* USE_GL */
      if (SDL_MUSTLOCK (screen)) {
        DebOut("SDL_LockSurface\n");
        SDL_LockSurface (screen);
      }
      picasso_vidinfo.rowbytes = screen->pitch;
      return (uae_u8 *)screen->pixels;
#ifdef USE_GL
    } else {
      picasso_vidinfo.rowbytes = display->pitch;
      return (uint8_t *) display->pixels;
    }
#endif /* USE_GL */
}

void gfx_unlock_picasso (bool dorender)
{
  //DEBUG_LOG ("Function: gfx_unlock_picasso\n");

#ifdef USE_GL
  if (!currprefs.use_gl) {
#endif /* USE_GL */
    if (SDL_MUSTLOCK (screen)) {
      DebOut("SDL_UnockSurface\n");
      SDL_UnlockSurface (screen);
    }
#ifdef USE_GL
  }
#endif /* USE_GL */
  SDL_Flip(screen);
}
//#endif /* PICASSO96 */

int is_fullscreen (void) {

  return fullscreen;
}

int is_vsync (void) {

  return vsync;
}

void toggle_fullscreen (int mode)
{
  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  /* FIXME: Add support for separate full-screen/windowed sizes */
  fullscreen = 1 - fullscreen;

  /* Close existing window and open a new one (with the new fullscreen setting) */
  graphics_subshutdown ();
  graphics_subinit ();

  notice_screen_contents_lost ();
  if (screen_is_picasso) {
    refresh_necessary = 1;
  }

  DEBUG_LOG ("ToggleFullScreen: %d\n", fullscreen );
}

void toggle_mousegrab (void) {

  SDLGD(
    bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__);
    bug("[JUAE:SDL] %s: fullscreen: %d\n", __PRETTY_FUNCTION__, fullscreen);
  )

  if (!fullscreen) {
    if (SDL_WM_GrabInput (SDL_GRAB_QUERY) == SDL_GRAB_OFF) {
      if (SDL_WM_GrabInput (SDL_GRAB_ON) == SDL_GRAB_ON) {
        mousegrab = 1;
        mousehack = 0;
        SDL_ShowCursor (SDL_DISABLE);
      }
    } else {
      if (SDL_WM_GrabInput (SDL_GRAB_OFF) == SDL_GRAB_OFF) {
        mousegrab = 0;
        mousehack = 1;

        SDL_ShowCursor ((currprefs.input_magic_mouse_cursor!=1) ? SDL_DISABLE : SDL_ENABLE);
        //SDL_ShowCursor (currprefs.hide_cursor ?  SDL_DISABLE : SDL_ENABLE);
      }
    }
  }
}

/*
 * Mouse inputdevice functions
 */

/* Hardwire for 3 axes and 3 buttons - although SDL doesn't
 * currently support a Z-axis as such. Mousewheel events are supplied
 * as buttons 4 and 5
 */


int getcapslockstate (void) {

  return SDL_GetModState() & KMOD_CAPS;
}

void setcapslockstate (int state) {

  TODO();
  //SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

}

static void setid (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, bool gp) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  if (gp) {
    inputdevice_sparecopy (&uid[i], slot, 0);
  }
  uid[i].eventid[slot][sub] = evt;
  uid[i].port[slot][sub] = port + 1;
}

static void setid (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, int af, bool gp) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  setid (uid, i, slot, sub, port, evt, gp);
  uid[i].flags[slot][sub] &= ~ID_FLAG_AUTOFIRE_MASK;
  if (af >= JPORT_AF_NORMAL)
    uid[i].flags[slot][sub] |= ID_FLAG_AUTOFIRE;
  if (af == JPORT_AF_TOGGLE)
    uid[i].flags[slot][sub] |= ID_FLAG_TOGGLE;
  if (af == JPORT_AF_ALWAYS)
    uid[i].flags[slot][sub] |= ID_FLAG_INVERTTOGGLE;
}

/*
 * Default inputdevice config for SDL mouse
 */
int input_get_default_mouse (struct uae_input_device *uid, int num, int port, int af, bool gp, bool wheel, bool joymouseswap) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  SDLGD(bug("uid %lx, num %d, port %d, af %d\n", uid, num, port, af));
  SDLGD(bug("input_get_default_mouse: ignore parameter gp %d, wheel %d, joymouseswap %d\n", gp, wheel, joymouseswap));
  /* SDL supports only one mouse */
  setid (uid, num, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_MOUSE2_HORIZ : INPUTEVENT_MOUSE1_HORIZ, gp);
  setid (uid, num, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_MOUSE2_VERT : INPUTEVENT_MOUSE1_VERT, gp);
  setid (uid, num, ID_AXIS_OFFSET + 2, 0, port, port ? 0 : INPUTEVENT_MOUSE1_WHEEL, gp);
  setid (uid, num, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
  setid (uid, num, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
  setid (uid, num, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);
  if (port == 0) { /* map back and forward to ALT+LCUR and ALT+RCUR */
    setid (uid, num, ID_BUTTON_OFFSET + 3, 0, port, INPUTEVENT_KEY_ALT_LEFT, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 3, 1, port, INPUTEVENT_KEY_CURSOR_LEFT, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 4, 0, port, INPUTEVENT_KEY_ALT_LEFT, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 4, 1, port, INPUTEVENT_KEY_CURSOR_RIGHT, gp);
  }
  if (num == 0) {
    return 1;
  }
  return 0;
}

/*
 * Handle gfx specific cfgfile options
 */
void gfx_default_options (struct uae_prefs *p)
{
  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));
  TODO();

#if 0
  int type = get_sdlgfx_type ();

//fixme
  if (type == SDLGFX_DRIVER_AMIGAOS4 || type == SDLGFX_DRIVER_CYBERGFX || type == SDLGFX_DRIVER_BWINDOW  || type == SDLGFX_DRIVER_QUARTZ)
      p->map_raw_keys = 1;
  else
      p->map_raw_keys = 0;
#endif
#ifdef USE_GL
//  p->use_gl = 0;
#endif /* USE_GL */
}

void gfx_save_options (struct zfile *f, const struct uae_prefs *p) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

#if 0
  cfgfile_write (f, GFX_NAME ".map_raw_keys=%s\n", p->map_raw_keys ? "true" : "false");
#endif
#ifdef USE_GL
  cfgfile_write (f, GFX_NAME ".use_gl=%s\n", p->use_gl ? "true" : "false");
#endif /* USE_GL */
}

int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value) {

  int result = 0;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

#if 0
  result = (cfgfile_yesno (option, value, "map_raw_keys", &(p->map_raw_keys)));
#endif

#ifdef USE_GL
  result = result || (cfgfile_yesno (option, value, "use_gl", &(p->use_gl)));
#endif /* USE_GL */

  return result;
}

int target_checkcapslock (int scancode, int *state) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  if (scancode != DIK_CAPITAL && scancode != DIK_NUMLOCK && scancode != DIK_SCROLL)
    return 0;
  if (*state == 0)
    return -1;

  if (scancode == DIK_CAPITAL)
    *state = SDL_GetModState() & KMOD_CAPS;
  if (scancode == DIK_NUMLOCK)
    *state = SDL_GetModState() & KMOD_NUM;
//        if (scancode == DIK_SCROLL)
//                *state = host_scrolllockstate;
  return 1;
}

/* od-aros/main.cpp */
extern TCHAR VersionStr[];
void makeverstr (TCHAR *s);

void setmaintitle (void) {

  TCHAR txt[1000], txt2[500];

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  makeverstr(VersionStr);
  txt[0] = 0;
#ifdef INPREC
  inprec_getstatus (txt);
#endif
  /*if (currprefs.config_window_title[0]) {
    _tcscat (txt, currprefs.config_window_title);
    _tcscat (txt, " - ");
  } else*/ 
  if (config_filename[0]) {
    _tcscat (txt, "[");
    _tcscat (txt, config_filename);
    _tcscat (txt, "] - ");
  }
  //_tcscat (txt, title);
  _tcscat (txt, VersionStr);
  txt2[0] = 0;
  if (txt2[0]) {
    _tcscat (txt, " - ");
    _tcscat (txt, txt2);
  }

  SDL_WM_SetCaption(txt, txt);
}

extern struct vidbuf_description gfxvidinfo;

void flush_block (struct vidbuffer *vb, int first, int last) {

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));
  SDLGD(bug("[JUAE:SDL] %s: vidbuffer @ 0x%p [%d -> %d]\n", __PRETTY_FUNCTION__, vb,  first, last));

#ifndef USE_GL
  sdl_flush_block_nolock (&gfxvidinfo, vb, first, last);
#else
  flush_gl_buffer (&glbuffer, first, last);
#endif
}


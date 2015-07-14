/*
 * UAE - The Un*x Amiga Emulator
 *
 * SDL graphics support
 *
 * Copyright 2001 Bernd Lachner (EMail: dev@lachner-net.de)
 * Copyright 2003-2007 Richard Drummond
 * Copyright 2006 Jochen Becher
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

#include "sysconfig.h"
#include "sysdeps.h"
#include "keymap.h"
//#include "misc.h"
#include "sdlkeys_dik.h"
#include "options.h"
#include "sdl_aros.h"
#include "gfx.h"

#include <SDL.h>
#include <SDL_endian.h>

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

#ifdef USE_GL
#define NO_SDL_GLEXT
# include <SDL_opengl.h>
/* These are not defined in the current version of SDL_opengl.h. */
# ifndef GL_TEXTURE_STORAGE_HINT_APPLE
#  define GL_TEXTURE_STORAGE_HINT_APPLE 0x85BC
#  endif
# ifndef GL_STORAGE_SHARED_APPLE
#  define GL_STORAGE_SHARED_APPLE 0x85BF
# endif

#ifdef GL_SHADER
#ifdef __APPLE__
  #include <OpenGL/glu.h>
  #include <OpenGL/glext.h>
  void setupExtensions(void)
  { shading_enabled = 1; } // OS X already has these extensions
#elifdef __X11__
  #include <GL/glx.h>
  #include <GL/glxext.h>
  #define uglGetProcAddress(x) (*glXGetProcAddressARB)((const GLubyte*)(x))
  #define X11_GL
#else
  void setupExtensions(void)
  { shading_enabled = 0; } // just fail otherwise?
#endif
#endif

#define DEBUG_LOG bug

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

#define DEBUG_LOG bug

SDL_Surface *display = NULL;
SDL_Surface *screen = NULL;

/* Standard P96 screen modes */
#define MAX_SCREEN_MODES 12
static int x_size_table[MAX_SCREEN_MODES] = { 320, 320, 320, 320, 640, 640, 640, 800, 1024, 1152, 1280 };
static int y_size_table[MAX_SCREEN_MODES] = { 200, 240, 256, 400, 350, 480, 512, 600, 768,  864,  1024 };

/* Supported SDL screen modes */
#define MAX_SDL_SCREENMODE 32
static SDL_Rect screenmode[MAX_SDL_SCREENMODE];
static int mode_count;

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

#ifdef PICASSO96
extern int screen_is_picasso;
extern int screen_was_picasso;
static char picasso_invalid_lines[1201];
static int picasso_has_invalid_lines;
static int picasso_invalid_start, picasso_invalid_stop;
static int picasso_maxw = 0, picasso_maxh = 0;
#endif

static int bitdepth, bit_unit;
static int current_width, current_height;

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

#if defined(X11_GL)
PFNGLCREATEPROGRAMOBJECTARBPROC     glCreateProgramObjectARB = NULL;
PFNGLDELETEOBJECTARBPROC            glDeleteObjectARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC      glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC            glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC           glCompileShaderARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC    glGetObjectParameterivARB = NULL;
PFNGLATTACHOBJECTARBPROC            glAttachObjectARB = NULL;
PFNGLGETINFOLOGARBPROC              glGetInfoLogARB = NULL;
PFNGLLINKPROGRAMARBPROC             glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC        glUseProgramObjectARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC      glGetUniformLocationARB = NULL;
PFNGLUNIFORM1FARBPROC               glUniform1fARB = NULL;
PFNGLUNIFORM1IARBPROC               glUniform1iARB = NULL;

unsigned int findString(char* in, char* list)
{
  int thisLength = strlen(in);
  while (*list != 0)
  {
    int length = strcspn(list," ");
    
    if( thisLength == length )
      if (!strncmp(in,list,length))
        return 1;
        
    list += length + 1;
  }
  return 0;
}

void setupExtensions(void)
{
  char* extensionList = (char*)glGetString(GL_EXTENSIONS);

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  if( findString("GL_ARB_shader_objects",extensionList) &&
      findString("GL_ARB_shading_language_100",extensionList) &&
      findString("GL_ARB_vertex_shader",extensionList) &&
      findString("GL_ARB_fragment_shader",extensionList) )
  {
    glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)
      uglGetProcAddress("glCreateProgramObjectARB");
    glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)
      uglGetProcAddress("glDeleteObjectARB");
    glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)
     uglGetProcAddress("glCreateShaderObjectARB");
    glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)
      uglGetProcAddress("glShaderSourceARB");
    glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)
      uglGetProcAddress("glCompileShaderARB");
    glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)
      uglGetProcAddress("glGetObjectParameterivARB");
    glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)
      uglGetProcAddress("glAttachObjectARB");
    glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)
      uglGetProcAddress("glGetInfoLogARB");
    glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)
      uglGetProcAddress("glLinkProgramARB");
    glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)
      uglGetProcAddress("glUseProgramObjectARB");
    glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)
      uglGetProcAddress("glGetUniformLocationARB");
    glUniform1fARB = (PFNGLUNIFORM1FARBPROC)
      uglGetProcAddress("glUniform1fARB");
    glUniform1iARB = (PFNGLUNIFORM1IARBPROC)
      uglGetProcAddress("glUniform1iARB");

    if( 0
     || glActiveTextureARB == NULL
     || glMultiTexCoord2fARB == NULL
     || glCreateProgramObjectARB == NULL
     || glDeleteObjectARB == NULL
     || glCreateShaderObjectARB == NULL
     || glShaderSourceARB == NULL
     || glCompileShaderARB == NULL
     || glGetObjectParameterivARB == NULL
     || glAttachObjectARB == NULL
     || glGetInfoLogARB == NULL
     || glLinkProgramARB == NULL
     || glUseProgramObjectARB == NULL
     || glGetUniformLocationARB == NULL
     || glUniform1fARB == NULL
     || glUniform1iARB == NULL
      )
      shading_enabled = 1;
    else shading_enabled = 1;
  } else
    shading_enabled = 0;
}
#endif /* defined(X11_GL) */

/*
 * What graphics platform are we running on . . .?
 *
 * Yes, SDL is supposed to abstract away from the underlying
 * platform, but we need to know this to be able to map raw keys
 * and to work around any platform-specific quirks . . .
 *
 * On AROS SDL_VideoDriverName seems not to return a reasonable name .. 
 */
int get_sdlgfx_type (void)
{
    char name[16] = "";
    static int driver = SDLGFX_DRIVER_UNKNOWN;
    static int search_done = 0;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

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

STATIC_INLINE unsigned long bitsInMask (unsigned long mask)
{
    /* count bits in mask */
    unsigned long n = 0;
    while (mask) {
        n += mask & 1;
        mask >>= 1;
    }
    return n;
}

STATIC_INLINE unsigned long maskShift (unsigned long mask)
{
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
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    DEBUG_LOG ("Function: get_color\n");

    arSDLColors[ncolors].r = r << 4;
    arSDLColors[ncolors].g = g << 4;
    arSDLColors[ncolors].b = b << 4;
    *cnp = ncolors++;
    return 1;
}

static void init_colors (void)
{
    int i;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

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
static int find_best_mode (int *width, int *height, int depth, int fullscreen)
{
    int found = 0;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

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

int picasso_palette (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  TODO();
  return false;
#if 0
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
        return changed;
#endif
}
/*
 * Map an SDL pixel format to a P96 pixel format
 */
static int get_p96_pixel_format (const struct SDL_PixelFormat *fmt)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    if (fmt->BitsPerPixel == 8)
        return RGBFB_CLUT;

#ifdef WORDS_BIGENDIAN
    if (fmt->BitsPerPixel == 24) {
        if (fmt->Rmask == 0x00FF0000 && fmt->Gmask == 0x0000FF00 && fmt->Bmask == 0x000000FF)
            return RGBFB_R8G8B8;
        if (fmt->Rmask == 0x000000FF && fmt->Gmask == 0x0000FF00 && fmt->Bmask == 0x00FF0000)
            return RGBFB_B8G8R8;
    } else if (fmt->BitsPerPixel == 32) {
        if (fmt->Rmask == 0xFF000000 && fmt->Gmask == 0x00FF0000 && fmt->Bmask == 0x0000FF00)
            return RGBFB_R8G8B8A8;
        if (fmt->Rmask == 0x00FF0000 && fmt->Gmask == 0x0000FF00 && fmt->Bmask == 0x000000FF)
            return RGBFB_A8R8G8B8;
        if (fmt->Bmask == 0x00FF0000 && fmt->Gmask == 0x0000FF00 && fmt->Rmask == 0x000000FF)
            return RGBFB_A8B8G8R8;
        if (fmt->Bmask == 0xFF000000 && fmt->Gmask == 0x00FF0000 && fmt->Rmask == 0x0000FF00)
            return RGBFB_B8G8R8A8;
    } else if (fmt->BitsPerPixel == 16) {
        if (get_sdlgfx_type () == SDLGFX_DRIVER_QUARTZ) {
            /* The MacOS X port of SDL lies about it's default pixel format
             * for high-colour display. It's always R5G5B5. */
            return RGBFB_R5G5B5;
        } else {
            if (fmt->Rmask == 0xf800 && fmt->Gmask == 0x07e0 && fmt->Bmask == 0x001f)
                return RGBFB_R5G6B5;
            if (fmt->Rmask == 0x7C00 && fmt->Gmask == 0x03e0 && fmt->Bmask == 0x001f)
                return RGBFB_R5G5B5;
        }
    } else if (fmt->BitsPerPixel == 15) {
        if (fmt->Rmask == 0x7C00 && fmt->Gmask == 0x03e0 && fmt->Bmask == 0x001f)
            return RGBFB_R5G5B5;
    }
#else
    if (fmt->BitsPerPixel == 24) {
        if (fmt->Rmask == 0x00FF0000 && fmt->Gmask == 0x0000FF00 && fmt->Bmask == 0x000000FF)
            return RGBFB_B8G8R8;
        if (fmt->Rmask == 0x000000FF && fmt->Gmask == 0x0000FF00 && fmt->Bmask == 0x00FF0000)
            return RGBFB_R8G8B8;
    } else if (fmt->BitsPerPixel == 32) {
        if (fmt->Rmask == 0xFF000000 && fmt->Gmask == 0x00FF0000 && fmt->Bmask == 0x0000FF00)
            return RGBFB_A8B8G8R8;
        if (fmt->Rmask == 0x00FF0000 && fmt->Gmask == 0x0000FF00 && fmt->Bmask == 0x000000FF)
            return RGBFB_B8G8R8A8;
        if (fmt->Bmask == 0x00FF0000 && fmt->Gmask == 0x0000FF00 && fmt->Rmask == 0x000000FF)
            return RGBFB_R8G8B8A8;
        if (fmt->Bmask == 0xFF000000 && fmt->Gmask == 0x00FF0000 && fmt->Rmask == 0x0000FF00)
            return RGBFB_A8R8G8B8;
    } else if (fmt->BitsPerPixel == 16) {
        if (fmt->Rmask == 0xf800 && fmt->Gmask == 0x07e0 && fmt->Bmask == 0x001f)
            return RGBFB_R5G6B5PC;
        if (fmt->Rmask == 0x7C00 && fmt->Gmask == 0x03e0 && fmt->Bmask == 0x001f)
            return RGBFB_R5G5B5PC;
    } else if (fmt->BitsPerPixel == 15) {
        if (fmt->Rmask == 0x7C00 && fmt->Gmask == 0x03e0 && fmt->Bmask == 0x001f)
            return RGBFB_R5G5B5PC;
    }
#endif

    return RGBFB_NONE;
}
#endif

/*
 * Build list of full-screen screen-modes supported by SDL
 * with the specified pixel format.
 *
 * Returns a count of the number of supported modes, -1 if any mode is supported,
 * or 0 if there are no modes with this pixel format.
 */
static long find_screen_modes (struct SDL_PixelFormat *vfmt, SDL_Rect *mode_list, int mode_list_size)
{
    long count = 0;
    SDL_Rect **modes = SDL_ListModes (vfmt, SDL_FULLSCREEN | SDL_HWSURFACE);

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    if (modes != 0 && modes != (SDL_Rect**)-1) {
        unsigned int i;
        int w = -1;
        int h = -1;

        /* Filter list of modes SDL gave us and ignore duplicates */
        for (i = 0; modes[i] && count < mode_list_size; i++) {
            if (modes[i]->w != w || modes[i]->h != h) {
                mode_list[count].w = w = modes[i]->w;
                mode_list[count].h = h = modes[i]->h;
                count++;

                write_log ("SDLGFX: Found screenmode: %dx%d.\n", w, h);
            }
        }
    } else
        count = (long) modes;

    return count;
}

#ifdef USE_GL

/**
 ** Support routines for using an OpenGL texture as the display buffer.
 **
 ** TODO: Make this independent of the window system and factor it out
 **       to a new module shareable by all graphics drivers.
 **/

struct gl_buffer_t glbuffer;
static int have_texture_rectangles;
static int have_apple_client_storage;
static int have_apple_texture_range;

static int round_up_to_power_of_2 (int value)
{
    int result = 1;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    while (result < value)
        result *= 2;

    return result;
}

static void check_gl_extensions (void)
{
    static int done = 0;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    if (!done) {
        const char *extensions = (const char *) glGetString(GL_EXTENSIONS);

        have_texture_rectangles   = strstr (extensions, "ARB_texture_rectangle") ? 1 : 0;
        have_apple_client_storage = strstr (extensions, "APPLE_client_storage")  ? 1 : 0;
        have_apple_texture_range  = strstr (extensions, "APPLE_texture_range")   ? 1 : 0;
    }
}

static void init_gl_display (GLsizei width, GLsizei height)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    glViewport (0, 0, width, height);

    glColor3f (1.0f, 1.0f, 1.0f);
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel (GL_FLAT);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_ALPHA_TEST);
    glDisable (GL_LIGHTING);

    if (have_texture_rectangles)
        glEnable (GL_TEXTURE_RECTANGLE_ARB);
    else
        glEnable (GL_TEXTURE_2D);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (0, width, height, 0, -1.0, 1.0);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    return;
}

static void free_gl_buffer (struct gl_buffer_t *buffer)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    glBindTexture (buffer->target, 0);
    glDeleteTextures (1, &buffer->texture);

    SDL_FreeSurface (display);
    display = 0;
}

static int alloc_gl_buffer (struct gl_buffer_t *buffer, int width, int height, int want_16bit)
{
    GLenum tex_intformat;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    buffer->width          = width;
    if (have_texture_rectangles) {
        buffer->texture_width  = width;
        buffer->texture_height = height;
        buffer->target         = GL_TEXTURE_RECTANGLE_ARB;
    } else {
        buffer->texture_width  = round_up_to_power_of_2 (width);
        buffer->texture_height = round_up_to_power_of_2 (height);
        buffer->target         = GL_TEXTURE_2D;
    }

    /* TODO: Should allocate buffer after we've successfully created the texture */
    if (want_16bit) {
#if defined (__APPLE__)
        display = SDL_CreateRGBSurface (SDL_SWSURFACE, buffer->texture_width, buffer->texture_height, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00000000);
#else
        display = SDL_CreateRGBSurface (SDL_SWSURFACE, buffer->texture_width, buffer->texture_height, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000);
#endif
    } else {
        display = SDL_CreateRGBSurface (SDL_SWSURFACE, buffer->texture_width, buffer->texture_height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
    }

    if (!display)
        return 0;

    buffer->pixels = display->pixels;
    buffer->pitch  = display->pitch;

    glGenTextures   (1, &buffer->texture);
    glBindTexture   (buffer->target, buffer->texture);
    if (have_apple_client_storage)
        glPixelStorei (GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
    if (have_apple_texture_range)
        glTexParameteri (buffer->target, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);

    glTexParameteri (buffer->target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (buffer->target, GL_TEXTURE_WRAP_T, GL_CLAMP);

    /* TODO: Better method of deciding on the best texture format to use is needed. */
    if (want_16bit) {
#if defined (__APPLE__)
        tex_intformat     = GL_RGB5;
        buffer->format    = GL_BGRA;
        buffer->type      = GL_UNSIGNED_SHORT_1_5_5_5_REV;
#else
        tex_intformat     = GL_RGB;
        buffer->format    = GL_RGB;
        buffer->type      = GL_UNSIGNED_SHORT_5_6_5;
#endif
    } else {
        tex_intformat     = GL_RGBA8;
        buffer->format    = GL_BGRA;
        buffer->type      = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    glTexImage2D (buffer->target, 0, tex_intformat, buffer->texture_width, buffer->texture_height, 0, buffer->format, buffer->type, buffer->pixels);

    if (glGetError () != GL_NO_ERROR) {
        write_log ("SDLGFX: Failed to allocate texture.\n");
        free_gl_buffer (buffer);
        return 0;
    }
    else
        return 1;
}

void flush_gl_buffer (const struct gl_buffer_t *buffer, int first_line, int last_line)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    glTexSubImage2D (buffer->target, 0, 0, first_line, buffer->texture_width, last_line - first_line + 1, buffer->format, buffer->type, buffer->pixels + buffer->pitch * first_line);
}

void render_gl_buffer (const struct gl_buffer_t *buffer, int first_line, int last_line)
{
    float tx0, ty0, tx1, ty1; //source buffer coords
    float wx0, wy0, wx1, wy1; //destination text coords
    float left_crop, right_crop, top_crop, bottom_crop;  //buffer cropping
    float amiga_real_w,amiga_real_h ; //used to calulate cropping
    float bottomledspace = 0;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    last_line++;

    if (currprefs.leds_on_screen) bottomledspace=14; //reserve some space for drawing leds

    if (currprefs.gfx_lores_mode) {
        amiga_real_w = 362;
    } else {
        amiga_real_w = 724;
    }
    if (currprefs.gfx_vresolution) {
        amiga_real_h = 568;
    } else {
        amiga_real_h = 284;
    }

    if ((current_width >= amiga_real_w ) &&  (current_height >= amiga_real_h )  && (!screen_is_picasso)  ) {
        right_crop  = (current_width - amiga_real_w);
        left_crop   = 0;
        bottom_crop = (current_height - amiga_real_h);
        top_crop    = 0;

        bottom_crop     = bottom_crop - bottomledspace ;
    } else {
        right_crop = 0;
        left_crop = 0;
        bottom_crop = 0;
        top_crop = 0;
    }

    if (have_texture_rectangles) {
        tx0 =                 left_crop;
        tx1 = (float) buffer->width - right_crop;
        ty0 = (float) first_line + top_crop;
        ty1 = (float) last_line - bottom_crop;
    } else {
        tx0 = 0.0f;
        ty0 = (float) first_line    / (float) buffer->texture_height;
        tx1 = (float) buffer->width / (float) buffer->texture_width;
        ty1 = (float) last_line     / (float) buffer->texture_height;
    }

    glBegin (GL_QUADS);
    glTexCoord2f (tx0, ty0); glVertex2f (0.0f,                  (float) first_line);
    glTexCoord2f (tx1, ty0); glVertex2f ((float) buffer->width, (float) first_line);
    glTexCoord2f (tx1, ty1); glVertex2f ((float) buffer->width, (float) last_line);
    glTexCoord2f (tx0, ty1); glVertex2f (0.0f,                  (float) last_line);
    glEnd ();
}

#endif /* USE_GL */

/**
 ** Buffer methods not implemented for this driver.
 **/

static void sdl_flush_line (struct vidbuf_description *gfxinfo, int line_no)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

}


/**
 ** Buffer methods for SDL surfaces that don't need locking
 **/

static int sdl_lock_nolock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 1;
}

static void sdl_unlock_nolock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

}

STATIC_INLINE void sdl_flush_block_nolock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line) {
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    SDLGD(bug("[JUAE:SDL] %s: display 0x%p vidbuf_description @ 0x%p\n", __PRETTY_FUNCTION__, display, gfxinfo));
    SDLGD(bug("[JUAE:SDL] %s: 0, %d ->  %d, %d\n", __PRETTY_FUNCTION__, first_line, current_width, last_line - first_line + 1));

    if(first_line >= last_line) {
#warning first_line >= last_line should not happen at all?
        SDLGD(bug("[JUAE:SDL] WARNING: %d >= %d\n", first_line, last_line));
    }
    else {
        SDL_UpdateRect (display, 0, first_line, current_width, last_line - first_line + 1);
    }
}

/**
 ** Buffer methods for SDL surfaces that must be locked
 **/

static int sdl_lock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
    int success = 0;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

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

static void sdl_unlock (struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    DEBUG_LOG ("Function: unlock\n");

    SDL_UnlockSurface (display);
}

static void sdl_flush_block (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    DEBUG_LOG ("Function: flush_block %d %d\n", first_line, last_line);

    SDL_UnlockSurface (display);

    sdl_flush_block_nolock (gfxinfo, vb, first_line, last_line);

    SDL_LockSurface (display);
}

static void sdl_flush_screen_dummy (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));
}

//#include "hrtimer.h"

static void sdl_flush_screen_flip (struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line)
{
    frame_time_t start_time;
    frame_time_t sleep_time;

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    SDL_BlitSurface (display, 0, screen,0);

    start_time = read_processor_time ();

    SDL_Flip (screen);

    sleep_time = read_processor_time () - start_time;
    idletime += sleep_time;
}

static void sdl_flush_clear_screen (struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    DEBUG_LOG ("Function: flush_clear_screen\n");

    if (display) {
        SDL_Rect rect = { 0, 0, display->w, display->h };
        SDL_FillRect (display, &rect, SDL_MapRGB (display->format, 0,0,0));
        SDL_UpdateRect (display, 0, 0, rect.w, rect.h);
    }
}

void flush_screen (struct vidbuffer *vb, int first_line, int last_line) {

    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    SDLGD(bug("[JUAE:SDL] %s: vidbuffer @0x%p (%d -> %d)\n", __PRETTY_FUNCTION__, vb, first_line, last_line));

  SDL_UpdateRect (display, 0, first_line, current_width, last_line - first_line + 1);


#if 0
	if (dx_islost ())
		return;
	flushymin = 0;
	flushymax = currentmode->amiga_height;
	if (currentmode->flags & DM_D3D) {
		D3D_flip ();
#ifdef GFXFILTER
	} else if (currentmode->flags & DM_SWSCALE) {
		S2X_render ();
		DirectDraw_Flip (1);
#endif
	} else if (currentmode->flags & DM_DDRAW) {
		DirectDraw_Flip (1);
	}
#endif
}




#ifdef USE_GL

/**
 ** Buffer methods for SDL GL surfaces
 **/

static int sdl_gl_lock (struct vidbuf_description *gfxinfo)
{
    return 1;
}

static void sdl_gl_unlock (struct vidbuf_description *gfxinfo)
{
}

static void sdl_gl_flush_block (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    DEBUG_LOG ("Function: sdl_gl_flush_block %d %d\n", first_line, last_line);

    flush_gl_buffer (&glbuffer, first_line, last_line);
}

/* Single-buffered flush-screen method */
static void sdl_gl_flush_screen (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    render_gl_buffer (&glbuffer, first_line, last_line);
    glFlush ();
}

/* Double-buffered flush-screen method */
static void sdl_gl_flush_screen_dbl (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    render_gl_buffer (&glbuffer, 0, display->h - 1);
    SDL_GL_SwapBuffers ();
}

/* Double-buffered, vsynced flush-screen method */
static void sdl_gl_flush_screen_vsync (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
    frame_time_t start_time;
    frame_time_t sleep_time;

    render_gl_buffer (&glbuffer, 0, display->h - 1);

    start_time = read_processor_time ();

    SDL_GL_SwapBuffers ();

    sleep_time = read_processor_time () - start_time;
    idletime += sleep_time;
}

static void sdl_gl_flush_clear_screen (struct vidbuf_description *gfxinfo)
{
    DEBUG_LOG ("Function: sdl_gl_flush_clear_screen\n");

    if (display) {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapBuffers ();
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapBuffers ();
    }
}

#endif /* USE_GL */

static int graphics_setup_success=0;

void fill_DisplayModes(struct MultiDisplay *md) {
  int result = 0;
  unsigned int i;

  SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

  max_uae_width = 8192;
  max_uae_height = 8192;


  if (SDL_InitSubSystem (SDL_INIT_VIDEO) == 0) {

      const SDL_version   *version = SDL_Linked_Version ();
      const SDL_VideoInfo *info    = SDL_GetVideoInfo ();

      write_log ("SDLGFX: Initialized.\n");
      write_log ("SDLGFX: Using SDL version %d.%d.%d.\n", version->major, version->minor, version->patch);

      /* Find default display depth */
      bitdepth = info->vfmt->BitsPerPixel;
      SDLGD(bug("[JUAE:SDL] %s: bitdepth: %d\n", __PRETTY_FUNCTION__, bitdepth));
      bit_unit = info->vfmt->BytesPerPixel * 8;

      write_log ("SDLGFX: Display is %d bits deep.\n", bitdepth);

      /* Build list of screenmodes */
      mode_count = find_screen_modes (info->vfmt, &screenmode[0], MAX_SDL_SCREENMODE);
      md->DisplayModes = xmalloc (struct PicassoResolution, MAX_PICASSO_MODES);
      for (i=0; i<mode_count; i++) {
        //md->DisplayModes[i]=(struct PicassoResolution *) malloc(sizeof(struct PicassoResolution));
        md->DisplayModes[i].res.width=screenmode[i].w;
        md->DisplayModes[i].res.height=screenmode[i].h;
        md->DisplayModes[i].depth=bitdepth;
        md->DisplayModes[i].lace=0;
        md->DisplayModes[i].residx=i;
        DebOut("%dx%d, %d-bit\n", screenmode[i].w, screenmode[i].h, bitdepth);
        _stprintf (md->DisplayModes[i].name, _T("%dx%d, %d-bit"), screenmode[i].w, screenmode[i].h, bitdepth);
      }
      /* terminat list */
      md->DisplayModes[i+1].depth = -1;
      md->DisplayModes[i+1].residx = -1;

      graphics_setup_success=1;
  }

  return;
}

int graphics_setup(void) {

  /* fill_DisplayModes was hopefully called before! */
  return graphics_setup_success;
}

#ifdef USE_GL

/*
 * Mergin this function into graphics_subinit_gl is possible but gives us a lot of
 * ifdefs.
 */
static int graphics_subinit_gl (void)
{
    Uint32 uiSDLVidModFlags = 0;
    int dblbuff = 0;
    int want_16bit;

    vsync = 0;

    DEBUG_LOG ("Function: graphics_subinit_gl\n");

    /* Request double-buffered mode only when vsyncing
     * is requested and we can determine whether vsyncing
     * is supported.
     */
#if SDL_VERSION_ATLEAST(1, 2, 10)
    if (!screen_is_picasso) {
        DEBUG_LOG ("Want double-buffering.\n");
        SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute (SDL_GL_SWAP_CONTROL, 1);
    } else {
        SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 0);
        SDL_GL_SetAttribute (SDL_GL_SWAP_CONTROL, 0);
    }
#else
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 0);
#endif

    if (bitdepth <= 16 || (!screen_is_picasso && !(currprefs.chipset_mask & CSMASK_AGA))) {
        DEBUG_LOG ("Want 16-bit framebuffer.\n");
        SDL_GL_SetAttribute (SDL_GL_RED_SIZE,   5);
        SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 5);
        SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE,  5);
        want_16bit = 1;
    } else {
        SDL_GL_SetAttribute (SDL_GL_RED_SIZE,   8);
        SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE,  8);
        want_16bit = 0;
    }

    // TODO: introduce a virtual resolution with scaling
    uiSDLVidModFlags = SDL_OPENGL;
    if (fullscreen) {
        uiSDLVidModFlags |= SDL_FULLSCREEN;
    }
    DEBUG_LOG ("Resolution     : %d x %d \n", current_width, current_height);
    screen = SDL_SetVideoMode (current_width, current_height, 0, uiSDLVidModFlags);

    if (screen == NULL) {
        gui_message ("Unable to set video mode: %s\n", SDL_GetError ());
        return 0;
    } else {
    /* Just in case we didn't get exactly what we asked for . . . */
    fullscreen   = ((screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);
    is_hwsurface = 0;

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

    SDL_GL_GetAttribute (SDL_GL_DOUBLEBUFFER, &dblbuff);
#if SDL_VERSION_ATLEAST(1, 2, 10)
    if (dblbuff)
        SDL_GL_GetAttribute (SDL_GL_SWAP_CONTROL, &vsync);
#endif

    if (!vsync)
        write_log ("SDLGFX: vsynced output not supported.\n");

    gfxvidinfo.lockscr = sdl_gl_lock;
    gfxvidinfo.unlockscr = sdl_gl_unlock;
    gfxvidinfo.flush_block = sdl_gl_flush_block;
    gfxvidinfo.flush_clear_screen = sdl_gl_flush_clear_screen;

    if (dblbuff) {
        if (vsync) {
            write_log ("SDLGFX: Using double-buffered, vsynced output.\n");
            gfxvidinfo.flush_screen = sdl_gl_flush_screen_vsync;
        } else {
            write_log ("SDLGFX: Using double-buffered, unsynced output.\n");
            gfxvidinfo.flush_screen = sdl_gl_flush_screen_dbl;
        }
    } else {
        write_log ("SDLGFX: Using single-buffered output.\n");
        gfxvidinfo.flush_screen = sdl_gl_flush_screen;
    }

    check_gl_extensions ();

    init_gl_display (current_width, current_height);
    if (!alloc_gl_buffer (&glbuffer, current_width, current_height, want_16bit))
        return 0;

#ifdef PICASSO96
    if (!screen_is_picasso) {
#endif
        gfxvidinfo.bufmem = display->pixels;
        gfxvidinfo.emergmem = 0;
        gfxvidinfo.maxblocklines = MAXBLOCKLINES_MAX;
        gfxvidinfo.linemem = 0;
        gfxvidinfo.pixbytes = display->format->BytesPerPixel;
        SDLGD(bug("gfxvidinfo.pixbytes: %d\n", gfxvidinfo.pixbytes);
        gfxvidinfo.rowbytes = display->pitch;
        SDL_SetColors (display, arSDLColors, 0, 256);
        reset_drawing ();
        old_pixels = (void *) -1;
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
        refresh_necessary           = 1;
    }
#endif
    }

    SDLGD(bug("gfxvidinfo.pixbytes: %d\n", gfxvidinfo.pixbytes);

    gfxvidinfo.emergmem = scrlinebuf; // memcpy from system-memory to video-memory

    //xfree (gfxvidinfo.realbufmem);
    gfxvidinfo.realbufmem = NULL;
//  gfxvidinfo.bufmem = NULL;
    gfxvidinfo.bufmem_allocated = NULL;
    gfxvidinfo.bufmem_lockable = false;

    return 1;
}

#endif /* USE_GL */

static int graphics_subinit (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

#ifdef USE_GL
    if (currprefs.use_gl) {
        if (graphics_subinit_gl () == 0)
            return 0;
    } else {
#endif /* USE_GL */

    Uint32 uiSDLVidModFlags = 0;

    DEBUG_LOG ("Function: graphics_subinit\n");

    if (bitdepth == 8)
    uiSDLVidModFlags |= SDL_HWPALETTE;
    if (fullscreen) {
        uiSDLVidModFlags |= SDL_FULLSCREEN | SDL_HWSURFACE;
        if (!screen_is_picasso)
            uiSDLVidModFlags |= SDL_DOUBLEBUF;
    }

    if(bitdepth==24) {
      DEBUG_LOG ("force bitdepth of 32 instead of 24\n");
      bitdepth=32;
    }
    DEBUG_LOG ("Resolution     : %d x %d x %d\n", current_width, current_height, bitdepth);

    screen = SDL_SetVideoMode (current_width, current_height, bitdepth, uiSDLVidModFlags);

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

    //bug("force gfxvidinfo.pixbytes %d to 4\n", gfxvidinfo.pixbytes);
    //gfxvidinfo.pixbytes=4;
    /* Set UAE window title and icon name */
    setmaintitle ();

    /* Mouse is now always grabbed when full-screen - to work around
     * problems with full-screen mouse input in some SDL implementations */
    if (fullscreen)
        SDL_WM_GrabInput (SDL_GRAB_ON);
    else
        SDL_WM_GrabInput (mousegrab ? SDL_GRAB_ON : SDL_GRAB_OFF);


    /* Hide mouse cursor */
    SDL_ShowCursor ((currprefs.input_magic_mouse_cursor!=1) || fullscreen || mousegrab ? SDL_DISABLE : SDL_ENABLE);

    mousehack = !fullscreen && !mousegrab;

    inputdevice_release_all_keys ();
#warning reset_hotkeys disabled
    //reset_hotkeys ();
    init_colors ();

    return 1;
}

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

    //current_width  = currprefs.gfx_size_win.width;
    //current_height = currprefs.gfx_size_win.height;
    current_width  = currprefs.gfx_size_win.width;
    current_height = currprefs.gfx_size_win.height;

    SDLGD(bug("current_width : %d\n", current_width));
    SDLGD(bug("current_height: %d\n", current_height));
    

    if (find_best_mode (&current_width, &current_height, bitdepth, fullscreen)) {
      currprefs.gfx_size_win.width  = current_width;
      currprefs.gfx_size_win.height = current_height;

      if (graphics_subinit ()) {
        success = 1;
      }
    }
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
    if (display)
        setmaintitle ();
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
            if (buttonno >= 0)
                setmousebuttonstate (0, buttonno, rEvent.type == SDL_MOUSEBUTTONDOWN ? 1:0);
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
                DEBUG_LOG ("Event: mouse motion abs (%d, %d)\n", rEvent.motion.x, rEvent.motion.y);
            } else {
                setmousestate (0, 0, rEvent.motion.xrel, 0);
                setmousestate (0, 1, rEvent.motion.yrel, 0);
                DEBUG_LOG ("Event: mouse motion rel (%d, %d)\n", rEvent.motion.xrel, rEvent.motion.yrel);
            }
            break;

      case SDL_ACTIVEEVENT:
            if (rEvent.active.state & SDL_APPINPUTFOCUS && !rEvent.active.gain) {
                DEBUG_LOG ("Lost input focus\n");
                inputdevice_release_all_keys ();
#warning reset_hotkeys disabled
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

static void switch_keymaps (void)
{
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

int check_prefs_changed_gfx (void)
{
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

int debuggable (void)
{
    return 1;
}

int mousehack_allowed (void)
{
    return mousehack;
}

void LED (int on)
{
}

#ifdef PICASSO96
/* TODO: x and width */
void DX_Invalidate (int x, int y, int width, int height)
{

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
  
#if 0
/* was: */
    if (is_hwsurface)
        return;

    if (first > last)
        return;

    picasso_has_invalid_lines = 1;
    if (first < picasso_invalid_start)
        picasso_invalid_start = first;
    if (last > picasso_invalid_stop)
        picasso_invalid_stop = last;

    while (first <= last) {
        picasso_invalid_lines[first] = 1;
        first++;
    }
#endif
}

static int palette_update_start = 256;
static int palette_update_end   = 0;

void DX_SetPalette (int start, int count)
{
    DEBUG_LOG ("Function: DX_SetPalette\n");

    if (! screen_is_picasso || picasso96_state.RGBFormat != RGBFB_CHUNKY)
        return;
#if 0

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
#endif
    TODO();
}

void DX_SetPalette_vsync(void)
{
    if (palette_update_end > palette_update_start) {
        DX_SetPalette (palette_update_start, palette_update_end - palette_update_start);
        palette_update_end   = 0;
        palette_update_start = 0;
    }
}

void DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color)
{
    SDLGD(bug("entered\n"));
#ifdef USE_GL /* TODO think about optimization for GL */
    if (!currprefs.use_gl) {
#endif /* USE_GL */
    SDL_Rect rect = {dstx, dsty, width, height};

    DEBUG_LOG ("DX_Fill (x:%d y:%d w:%d h:%d color=%08x)\n", dstx, dsty, width, height, color);

    if (SDL_FillRect (screen, &rect, color) == 0) {
        DX_Invalidate (0, dsty, width, height - 1);
    }
#ifdef USE_GL
    }
#endif /* USE_GL */
    return;
}

int DX_Blit (int srcx, int srcy, int dstx, int dsty, int width, int height, BLIT_OPCODE opcode)
{
    /* not implemented yet */
    return 0;
}

static void set_window_for_picasso (void)
{
    SDLGD(bug("entered\n"));
    DEBUG_LOG ("Function: set_window_for_picasso\n");

    if (screen_was_picasso && current_width == picasso_vidinfo.width && current_height == picasso_vidinfo.height)
        return;

    screen_was_picasso = 1;
    graphics_subshutdown();
    current_width  = picasso_vidinfo.width;
    current_height = picasso_vidinfo.height;
    graphics_subinit();
}

void gfx_set_picasso_modeinfo (uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{
    DEBUG_LOG ("Function: gfx_set_picasso_modeinfo w: %i h: %i depth: %i rgbfmt: %i\n", w, h, depth, rgbfmt);
    bug    ("Function: gfx_set_picasso_modeinfo w: %i h: %i depth: %i rgbfmt: %i\n", w, h, depth, rgbfmt);

    picasso_vidinfo.width = w;
    picasso_vidinfo.height = h;
    picasso_vidinfo.depth = depth;
    picasso_vidinfo.pixbytes = bit_unit >> 3;
    SDLGD(bug("gfxvidinfo.pixbytes: %d\n", picasso_vidinfo.pixbytes));
    if (screen_is_picasso)
        set_window_for_picasso();
}

/* Color management */

//static xcolnr xcol8[4096];

#if 0
static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;
#endif

void gfx_set_picasso_colors (RGBFTYPE rgbfmt)
{
    alloc_colors_picasso (red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt);
}

void gfx_set_picasso_state (int on)
{
    DEBUG_LOG ("Function: gfx_set_picasso_state: %d\n", on);

    if (on == screen_is_picasso)
        return;

    /* We can get called by drawing_init() when there's
     * no window opened yet... */
    if (display == 0)
        return;

    graphics_subshutdown ();
    screen_was_picasso = screen_is_picasso;
    screen_is_picasso = on;

#warning Set height, width for Amiga gfx
#if 0
    if (on) {
#endif
        // Set height, width for Picasso gfx
        current_width  = picasso_vidinfo.width;
        current_height = picasso_vidinfo.height;
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

uae_u8 *gfx_lock_picasso (bool fullupdate, bool doclear)
{
    DEBUG_LOG ("Function: gfx_lock_picasso\n");

#ifdef USE_GL
    if (!currprefs.use_gl) {
#endif /* USE_GL */
        if (SDL_MUSTLOCK (screen))
            SDL_LockSurface (screen);
        picasso_vidinfo.rowbytes = screen->pitch;
        return (uae_u8 *)screen->pixels;
#ifdef USE_GL
    } else {
        picasso_vidinfo.rowbytes = display->pitch;
        return display->pixels;
    }
#endif /* USE_GL */
}

/* doclear is not used at all ? */
uae_u8 *gfx_lock_picasso (bool fullupdate) {
  return gfx_lock_picasso (fullupdate, false);
}

void gfx_unlock_picasso (bool dorender)
{
    DEBUG_LOG ("Function: gfx_unlock_picasso\n");

#ifdef USE_GL
    if (!currprefs.use_gl) {
#endif /* USE_GL */
        if (SDL_MUSTLOCK (screen))
            SDL_UnlockSurface (screen);
#ifdef USE_GL
    }
#endif /* USE_GL */
}
#endif /* PICASSO96 */

int is_fullscreen (void)
{
    return fullscreen;
}

int is_vsync (void)
{

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
    if (screen_is_picasso)
        refresh_necessary = 1;

    DEBUG_LOG ("ToggleFullScreen: %d\n", fullscreen );
}

void toggle_mousegrab (void)
{
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

void screenshot (int mode, int doprepare)
{
    write_log ("Screenshot not supported yet\n");
}

/*
 * Mouse inputdevice functions
 */

/* Hardwire for 3 axes and 3 buttons - although SDL doesn't
 * currently support a Z-axis as such. Mousewheel events are supplied
 * as buttons 4 and 5
 */
#define MAX_BUTTONS 3
#define MAX_AXES    3
#define FIRST_AXIS  0
#define FIRST_BUTTON    MAX_AXES

static int init_mouse (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 1;
}

static void close_mouse (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return;
}

static int acquire_mouse (int num, int flags)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    /* SDL supports only one mouse */
    return 1;
}

static void unacquire_mouse (int num)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return;
}

static int get_mouse_num (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 1;
}

static TCHAR *get_mouse_friendlyname (int mouse)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return (TCHAR *)"Default mouse";
}
static TCHAR *get_mouse_uniquename (int mouse)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return (TCHAR *)"DEFMOUSE1";
}

static int get_mouse_widget_num (int mouse)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return MAX_AXES + MAX_BUTTONS;
}

static int get_mouse_widget_first (int mouse, int type)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    switch (type) {
    case IDEV_WIDGET_BUTTON:
        return FIRST_BUTTON;
    case IDEV_WIDGET_AXIS:
        return FIRST_AXIS;
    }
    return -1;
}

static int get_mouse_widget_type (int mouse, int num, TCHAR *name, uae_u32 *code)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

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
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    /* We handle mouse input in handle_events() */
}

static int get_mouse_flags (int num)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 0;
}

struct inputdevice_functions inputdevicefunc_mouse = {
    init_mouse,
    close_mouse,
    acquire_mouse,
    unacquire_mouse,
    read_mouse,
    get_mouse_num,
    get_mouse_friendlyname,
    get_mouse_uniquename,
    get_mouse_widget_num,
    get_mouse_widget_type,
    get_mouse_widget_first,
    get_mouse_flags
};

/*
 * Keyboard inputdevice functions
 */
static int get_kb_num (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    /* SDL supports only one keyboard */
    return 1;
}

static TCHAR *get_kb_friendlyname (int kb)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return (TCHAR *)"Default keyboard";
}
static TCHAR *get_kb_uniquename (int kb)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return (TCHAR *)"DEFKEYB1";
}

static int get_kb_widget_num (int kb)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 255; // fix me
}

static int get_kb_widget_first (int kb, int type)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 0;
}

static int get_kb_widget_type (int kb, int num, TCHAR *name, uae_u32 *code)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    if (name)
        _tcscpy (name, di_keyboard[kb].buttonname[num]);
    if (code)
        *code = di_keyboard[kb].buttonmappings[num];
    return IDEV_WIDGET_KEY;
}

static int init_kb (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    if (keyboard_inited)
        return 1;
    //oldusedleds = -1;
    keyboard_inited = 1;

    int i = 0;
    for ( ; i < num_keyboard; i++) {
        struct didata *did = &di_keyboard[i];
    /*  if (did->connection == DIDC_DX) {
            hr = g_lpdi->CreateDevice (did->iguid, &lpdi, NULL);
            if (SUCCEEDED (hr)) {
                hr = lpdi->SetDataFormat (&c_dfDIKeyboard);
                if (FAILED (hr))
                    write_log (_T("keyboard setdataformat failed, %s\n"), DXError (hr));
                memset (&dipdw, 0, sizeof (dipdw));
                dipdw.diph.dwSize = sizeof (DIPROPDWORD);
                dipdw.diph.dwHeaderSize = sizeof (DIPROPHEADER);
                dipdw.diph.dwObj = 0;
                dipdw.diph.dwHow = DIPH_DEVICE;
                dipdw.dwData = DI_KBBUFFER;
                hr = lpdi->SetProperty (DIPROP_BUFFERSIZE, &dipdw.diph);
                if (FAILED (hr))
                    write_log (_T("keyboard setpropertry failed, %s\n"), DXError (hr));
                lpdi->EnumObjects (EnumObjectsCallback, did, DIDFT_ALL);
                sortobjects (did);
                did->lpdi = lpdi;
            } else
                write_log (_T("keyboard CreateDevice failed, %s\n"), DXError (hr));
        }*/
    }

    keyboard_german = 0;
//  if ((LOWORD(GetKeyboardLayout (0)) & 0x3ff) == 7)
//      keyboard_german = 1;

    struct didata *did = &di_keyboard[0];
  int k;
    for (k = 0; k < 254; k++) {
        TCHAR tmp[100];
        tmp[0] = 0;
//          if (rawkeyboardlabels[k] != NULL && rawkeyboardlabels[k][0])
//              _tcscpy (tmp, rawkeyboardlabels[k]);
        if (!tmp[0])
            _stprintf (tmp, _T("KEY_%02X"), k + 1);
        did->buttonname[k] = my_strdup (tmp);
        did->buttonmappings[k] = k + 1;
        did->buttonsort[k] = k + 1;
    }

    return 1;
}

static void close_kb (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    if (keyboard_inited == 0)
        return;
    keyboard_inited = 0;

/*
    unsigned int i;
    for (i = 0; i < num_keyboard; i++)
        di_dev_free (&di_keyboard[i]);
    di_free ();
*/
//  superkb = normalkb = rawkb = 0;
}

static int keyhack (int scancode, int pressed, int num)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return scancode;
}

static void read_kb (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

}

static int acquire_kb (int num, int flags)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 1;
}

static void unacquire_kb (int num)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

}

static int get_kb_flags (int num)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return 0;
}

struct inputdevice_functions inputdevicefunc_keyboard =
{
    init_kb,
    close_kb,
    acquire_kb,
    unacquire_kb,
    read_kb,
    get_kb_num,
    get_kb_friendlyname,
    get_kb_uniquename,
    get_kb_widget_num,
    get_kb_widget_type,
    get_kb_widget_first,
    get_kb_flags
};

//static int capslockstate;

int getcapslockstate (void)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    return SDL_GetModState() & KMOD_CAPS;
}

void setcapslockstate (int state)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

    //TODO:
}


static void setid (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, bool gp)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

	if (gp)
		inputdevice_sparecopy (&uid[i], slot, 0);
	uid[i].eventid[slot][sub] = evt;
	uid[i].port[slot][sub] = port + 1;
}
static void setid (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, int af, bool gp)
{
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
//int input_get_default_mouse (struct uae_input_device *uid, int num, int port, int af)
int input_get_default_mouse (struct uae_input_device *uid, int num, int port, int af, bool gp, bool wheel, bool joymouseswap)
{
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
//                if (isrealbutton (did, 3)) {
      setid (uid, num, ID_BUTTON_OFFSET + 3, 0, port, INPUTEVENT_KEY_ALT_LEFT, gp);
      setid (uid, num, ID_BUTTON_OFFSET + 3, 1, port, INPUTEVENT_KEY_CURSOR_LEFT, gp);
//                        if (isrealbutton (did, 4)) {
      setid (uid, num, ID_BUTTON_OFFSET + 4, 0, port, INPUTEVENT_KEY_ALT_LEFT, gp);
      setid (uid, num, ID_BUTTON_OFFSET + 4, 1, port, INPUTEVENT_KEY_CURSOR_RIGHT, gp);
//                        }
//                }
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

    int type = get_sdlgfx_type ();

#if 0
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

void gfx_save_options (struct zfile *f, const struct uae_prefs *p)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

#if 0
    cfgfile_write (f, GFX_NAME ".map_raw_keys=%s\n", p->map_raw_keys ? "true" : "false");
#endif
#ifdef USE_GL
    cfgfile_write (f, GFX_NAME ".use_gl=%s\n", p->use_gl ? "true" : "false");
#endif /* USE_GL */
}

int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    SDLGD(bug("[JUAE:SDL] %s()\n", __PRETTY_FUNCTION__));

#if 0
    int result = (cfgfile_yesno (option, value, "map_raw_keys", &(p->map_raw_keys)));
#endif
    int result;
#ifdef USE_GL
    result = result || (cfgfile_yesno (option, value, "use_gl", &(p->use_gl)));
#endif /* USE_GL */
    return result;
}

int target_checkcapslock (int scancode, int *state)
{
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

void setmaintitle (void)
{
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
    } else*/ if (config_filename[0]) {
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

  sdl_flush_block_nolock (&gfxvidinfo, vb, first, last);
}



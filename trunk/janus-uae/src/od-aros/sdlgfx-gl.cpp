
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
 *
 * OpenGL functions moved to own file by Oliver Brunner
 */

#ifdef USE_GL

//#define JUAE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"
#include "keymap.h"
#include "sdlkeys_dik.h"
#include "options.h"
#ifdef __AROS__
#include "sdl_aros.h"
#include "picasso96_aros.h"
#endif
#include "gfx.h"

#include <SDL.h>
#include <SDL_endian.h>

#include <GL/glut.h>

#define SDLGD(x) x


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

    DebOut("width: %d, height: %d\n", width, height);


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

    DebOut("Version: %s\n", glGetString(GL_VERSION));

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

    buffer->pixels = (uint8_t*) display->pixels;
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

    if ((currentmode->current_width >= amiga_real_w ) &&  (currentmode->current_height >= amiga_real_h )  && (!screen_is_picasso)  ) {
        right_crop  = (currentmode->current_width - amiga_real_w);
        left_crop   = 0;
        bottom_crop = (currentmode->current_height - amiga_real_h);
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


/**
 ** Buffer methods for SDL GL surfaces
 **/

static int sdl_gl_lock (struct vidbuf_description *gfxinfo, struct vidbuffer* vidb)
{
    return 1;
}

static void sdl_gl_unlock (struct vidbuf_description *gfxinfo, struct vidbuffer* vidb)
{
}

static void sdl_gl_flush_block (struct vidbuf_description *gfxinfo, struct vidbuffer* vidb, int first_line, int last_line)
{
    DEBUG_LOG ("Function: sdl_gl_flush_block %d %d\n", first_line, last_line);

    flush_gl_buffer (&glbuffer, first_line, last_line);
}

/* Single-buffered flush-screen method */
static void sdl_gl_flush_screen (struct vidbuf_description *gfxinfo, struct vidbuffer* vidb, int first_line, int last_line)
{
    render_gl_buffer (&glbuffer, first_line, last_line);
    glFlush ();
}

/* Double-buffered flush-screen method */
static void sdl_gl_flush_screen_dbl (struct vidbuf_description *gfxinfo, struct vidbuffer* vidb, int first_line, int last_line)
{
    render_gl_buffer (&glbuffer, 0, display->h - 1);
    SDL_GL_SwapBuffers ();
}

/* Double-buffered, vsynced flush-screen method */
static void sdl_gl_flush_screen_vsync (struct vidbuf_description *gfxinfo, struct vidbuffer* vidb, int first_line, int last_line)
{
    frame_time_t start_time;
    frame_time_t sleep_time;

    render_gl_buffer (&glbuffer, 0, display->h - 1);

    start_time = read_processor_time ();

    SDL_GL_SwapBuffers ();

    sleep_time = read_processor_time () - start_time;
    idletime += sleep_time;
}

static void sdl_gl_flush_clear_screen (struct vidbuf_description *gfxinfo, struct vidbuffer* vidb)
{
    DEBUG_LOG ("Function: sdl_gl_flush_clear_screen\n");

    if (display) {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapBuffers ();
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapBuffers ();
    }
}


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

    if(bitdepth==24) {
      DEBUG_LOG ("force bitdepth of 32 instead of 24\n");
      bitdepth=32;
    }
    DEBUG_LOG ("Resolution     : %d x %d x %d\n", currentmode->current_width, currentmode->current_height, bitdepth);

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
    DEBUG_LOG ("Resolution     : %d x %d \n", currentmode->current_width, currentmode->current_height);
    screen = SDL_SetVideoMode (currentmode->current_width, currentmode->current_height, 0, uiSDLVidModFlags);

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

    gfxvidinfo.drawbuffer.lockscr = sdl_gl_lock;
    gfxvidinfo.drawbuffer.unlockscr = sdl_gl_unlock;
    gfxvidinfo.drawbuffer.flush_block = sdl_gl_flush_block;
    gfxvidinfo.drawbuffer.flush_clear_screen = sdl_gl_flush_clear_screen;

    gfxvidinfo.drawbuffer.width_allocated =screen->w;
    gfxvidinfo.drawbuffer.height_allocated=screen->h;
    DEBUG_LOG ("Window width   : %d\n", gfxvidinfo.drawbuffer.width_allocated);
    DEBUG_LOG ("Window height  : %d\n", gfxvidinfo.drawbuffer.height_allocated);
    DEBUG_LOG ("use_gl         : %d\n", currprefs.use_gl);


    if (dblbuff) {
        if (vsync) {
            write_log ("SDLGFX: Using double-buffered, vsynced output.\n");
            gfxvidinfo.drawbuffer.flush_screen = sdl_gl_flush_screen_vsync;
        } else {
            write_log ("SDLGFX: Using double-buffered, unsynced output.\n");
            gfxvidinfo.drawbuffer.flush_screen = sdl_gl_flush_screen_dbl;
        }
    } else {
        write_log ("SDLGFX: Using single-buffered output.\n");
        gfxvidinfo.drawbuffer.flush_screen = sdl_gl_flush_screen;
    }

    check_gl_extensions ();

    init_gl_display (currentmode->current_width, currentmode->current_height);
    if (!alloc_gl_buffer (&glbuffer, currentmode->current_width, currentmode->current_height, want_16bit))
        return 0;



#ifdef PICASSO96
    if (!screen_is_picasso) {
#endif
        gfxvidinfo.drawbuffer.bufmem = (uint8_t *) display->pixels;
        gfxvidinfo.drawbuffer.emergmem = 0;
        gfxvidinfo.maxblocklines = MAXBLOCKLINES_MAX;
        gfxvidinfo.drawbuffer.linemem = 0;
        gfxvidinfo.drawbuffer.pixbytes = display->format->BytesPerPixel;
        SDLGD(bug("gfxvidinfo.pixbytes: %d\n", gfxvidinfo.drawbuffer.pixbytes));
        gfxvidinfo.drawbuffer.rowbytes = display->pitch;
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

    SDLGD(bug("gfxvidinfo.drawbuffer.pixbytes: %d\n", gfxvidinfo.drawbuffer.pixbytes));

    gfxvidinfo.drawbuffer.emergmem = scrlinebuf; // memcpy from system-memory to video-memory

    //xfree (gfxvidinfo.realbufmem);
    gfxvidinfo.drawbuffer.realbufmem = NULL;
//  gfxvidinfo.bufmem = NULL;
    gfxvidinfo.drawbuffer.bufmem_allocated = NULL;
    gfxvidinfo.drawbuffer.bufmem_lockable = false;

    /* Set UAE window title and icon name */
    setmaintitle ();

    /* Hide mouse cursor */
    SDL_ShowCursor ((currprefs.input_magic_mouse_cursor!=1) || fullscreen || mousegrab ? SDL_DISABLE : SDL_ENABLE);

    mousehack = !fullscreen && !mousegrab;
    init_colors ();

    DebOut("success!\n");
    return 1;
}

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


#endif /* USE_GL */


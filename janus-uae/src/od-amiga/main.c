/************************************************************************ 
 *
 * Copyright 2004-2006 Richard Drummond
 * Copyright 2009      Oliver Brunner - aros<at>oliver-brunner.de
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
 * Start-up and support functions for Amiga target
 *
 ************************************************************************/


#include "sysconfig.h"
#include "sysdeps.h"

#include <libraries/asl.h>

#include "options.h"
#include "uae.h"
#include "xwin.h"
#include "debug.h"
#include "j.h"

#include "signal.h"

#define  __USE_BASETYPE__
#include <proto/exec.h>
#undef   __USE_BASETYPE__
#include <exec/execbase.h>

#ifdef USE_SDL
# include <SDL.h>
#endif

/* Get compiler/libc to enlarge stack to this size - if possible */
#if defined __PPC__ || defined __ppc__ || defined POWERPC || defined __POWERPC__
# define MIN_STACK_SIZE  (64 * 1024)
#else
# define MIN_STACK_SIZE  (32 * 1024)
#endif

#if defined __libnix__ || defined __ixemul__
/* libnix requires that we link against the swapstack.o module */
unsigned int __stack = MIN_STACK_SIZE;
#else
# if !defined __MORPHOS__ && !defined __AROS__
/* clib2 minimum stack size. Use this on OS3.x and OS4.0. */
unsigned int __stack_size = MIN_STACK_SIZE;
# endif
#endif

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

struct Device *TimerBase;
#ifdef __amigaos4__
struct Library *ExpansionBase;
struct TimerIFace *ITimer;
struct ExpansionIFace *IExpansion;
#endif

extern int os39;

/* close all libraries */
static void free_libs (void) {

#ifdef __amigaos4__
    if (ITimer)
	DropInterface ((struct Interface *)ITimer);
    if (IExpansion)
	DropInterface ((struct Interface *)IExpansion);
    if (ExpansionBase) {
	CloseLibrary (ExpansionBase);
	ExpansionBase=NULL;
    }
#endif

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

static int init_libs (void) {

  if (((struct ExecBase *)SysBase)->LibNode.lib_Version < 36) {
    write_log ("UAE needs OS 2.0+ !\n");
    return 0;
  }
  os39 = (((struct ExecBase *)SysBase)->LibNode.lib_Version >= 39);

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


  TimerBase = (struct Device *) FindName(&SysBase->DeviceList, "timer.device");

#ifdef __amigaos4__
  ITimer = (struct TimerIFace *) GetInterface((struct Library *)TimerBase, "main", 1, 0);

  ExpansionBase = OpenLibrary ("expansion.library", 0);
  if (ExpansionBase) {
    IExpansion = (struct ExpansionIFace *) GetInterface(ExpansionBase, "main", 1, 0);
  }

  if(!ITimer || !IExpansion) {
    return 0;
  }
#endif


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
#endif
  }

  /* success */
  return 1;
}

static int fromWB;
static FILE *logfile;

/************************************************
 *
 * main()
 *
 ************************************************
 *
 * Amiga-specific main entry
 *
 * Open all libraries,
 * Call real_main,
 * Close all libraries again.
 ************************************************/
int main (int argc, char *argv[]) {

  fromWB = argc == 0;

  if (fromWB) {
    set_logfile ("T:J-UAE.log");
  }

  if(!init_libs ()) {
    free_libs();
    write_log("Unable to open required libraries. Sorry.\n");
    exit(1);
  }

#ifdef USE_SDL
  init_sdl ();
#endif

  real_main (argc, argv);

  if (fromWB) {
    set_logfile (0);
  }

  free_libs();

  return 0;
}

/*
 * Handle CTRL-C signals
 */
static RETSIGTYPE sigbrkhandler(int foo)
{
#ifdef DEBUGGER
    activate_debugger ();
#endif
}

void setup_brkhandler (void)
{
#ifdef HAVE_SIGACTION
    struct sigaction sa;
    sa.sa_handler = (void*)sigbrkhandler;
    sa.sa_flags = 0;
    sa.sa_flags = SA_RESTART;
    sigemptyset (&sa.sa_mask);
    sigaction (SIGINT, &sa, NULL);
#else
    signal (SIGINT,sigbrkhandler);
#endif
}


/*
 * Handle target-specific cfgfile options
 */
void target_save_options (FILE *f, const struct uae_prefs *p)
{
}

int target_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    return 0;
}

void target_default_options (struct uae_prefs *p)
{
}

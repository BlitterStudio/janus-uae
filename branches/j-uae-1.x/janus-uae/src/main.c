/************************************************************************ 
 *
 * UAE - The Un*x Amiga Emulator
 *
 * Main program
 *
 * Copyright 1995      Ed Hanway
 * Copyright 1995-1997 Bernd Schmidt
 * Copyright 2006-2007 Richard Drummond
 * Copyright 2009-2010 Oliver Brunner - aros<at>oliver-brunner.de
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
#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "audio.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "serial.h"
#include "newcpu.h"
#include "disk.h"
#include "debug.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "gui.h"
#include "zfile.h"
#include "autoconf.h"
#include "traps.h"
#include "osemu.h"
#include "filesys.h"
#include "p96.h"
#include "bsdsocket.h"
#include "uaeexe.h"
#include "native2amiga.h"
#include "scsidev.h"
#include "akiko.h"
#include "savestate.h"
#include "hrtimer.h"
#include "sleep.h"
#include "version.h"

#ifdef USE_SDL
#include "SDL.h"
#endif

//#define BSDSOCKET

#ifdef WIN32
//FIXME: This shouldn't be necessary
#include "windows.h"
#endif

#ifdef __AROS__
#define GTKMUI
#include "od-amiga/j.h"
#endif

//#define MAIN_DEBUG 1
#ifdef  MAIN_DEBUG
#define DEBUG_LOG(...) do { kprintf("%s:%d %s(): ",__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define DEBUG_LOG(...) do ; while(0)
#endif

extern struct Process *proxy_thread;
struct uae_prefs currprefs, changed_prefs;

static int restart_program;
static char restart_config[256];
char optionsfile[256];
char default_optionsfile[256];

int cloanto_rom = 0;

int log_scsi;

struct gui_info gui_data;

uae_sem_t gui_main_wait_sem;   // For the GUI thread to tell UAE/main that it's ready.

void on_start_clicked (void);
BOOL gui_visible (void);

/*
 * Random prefs-related junk that needs to go elsewhere.
 */

void fixup_prefs_dimensions (struct uae_prefs *prefs)
{
    if (prefs->gfx_width_fs < 320)
	prefs->gfx_width_fs = 320;
    if (prefs->gfx_height_fs < 200)
	prefs->gfx_height_fs = 200;
    if (prefs->gfx_height_fs > 1280)
	prefs->gfx_height_fs = 1280;
    prefs->gfx_width_fs += 7;
    prefs->gfx_width_fs &= ~7;
    if (prefs->gfx_width_win < 320)
	prefs->gfx_width_win = 320;
    if (prefs->gfx_height_win < 200)
	prefs->gfx_height_win = 200;
    if (prefs->gfx_height_win > 1280)
	prefs->gfx_height_win = 1280;
    prefs->gfx_width_win += 7;
    prefs->gfx_width_win &= ~7;
}

static void fixup_prefs_joysticks (struct uae_prefs *prefs)
{
    int joy_count = inputdevice_get_device_total (IDTYPE_JOYSTICK);

    /* If either port is configured to use a non-existent joystick, try
     * to use a sensible alternative.
     */
    if (prefs->jport0 >= JSEM_JOYS && prefs->jport0 < JSEM_MICE) {
	if (prefs->jport0 - JSEM_JOYS >= joy_count)
	    prefs->jport0 = (prefs->jport1 != JSEM_MICE) ? JSEM_MICE : JSEM_NONE;
    }
    if (prefs->jport1 >= JSEM_JOYS && prefs->jport1 < JSEM_MICE) {
	if (prefs->jport1 - JSEM_JOYS >= joy_count)
	    prefs->jport1 = (prefs->jport0 != JSEM_KBDLAYOUT) ? JSEM_KBDLAYOUT : JSEM_NONE;
    }
}

static void fix_options (void)
{
    int err = 0;
		int foo;

    if ((currprefs.chipmem_size & (currprefs.chipmem_size - 1)) != 0
	|| currprefs.chipmem_size < 0x40000
	|| currprefs.chipmem_size > 0x800000)
    {
	currprefs.chipmem_size = 0x200000;
	write_log ("Unsupported chipmem size!\n");
	err = 1;
    }
    if (currprefs.chipmem_size > 0x80000)
	currprefs.chipset_mask |= CSMASK_ECS_AGNUS;

    if ((currprefs.fastmem_size & (currprefs.fastmem_size - 1)) != 0
	|| (currprefs.fastmem_size != 0 && (currprefs.fastmem_size < 0x100000 || currprefs.fastmem_size > 0x800000)))
    {
	currprefs.fastmem_size = 0;
	write_log ("Unsupported fastmem size!\n");
	err = 1;
    }
    if ((currprefs.gfxmem_size & (currprefs.gfxmem_size - 1)) != 0
	|| (currprefs.gfxmem_size != 0 && (currprefs.gfxmem_size < 0x100000 || currprefs.gfxmem_size > 0x10000000)))
    {
	write_log ("Unsupported graphics card memory size %lx!\n", currprefs.gfxmem_size);
	currprefs.gfxmem_size = 0;
	err = 1;
    }
    if ((!currprefs.z3fastmem_size == 0x60000000) && 
          (
	    ( (currprefs.z3fastmem_size & (currprefs.z3fastmem_size - 1)) != 0 ) || 
	      (
	         currprefs.z3fastmem_size != 0 && 
		 (currprefs.z3fastmem_size < 0x100000 || currprefs.z3fastmem_size > 0x60000000)
	      )
	  )
	) {
	  write_log ("Unsupported Zorro III fastmem size 0x%lx!\n", currprefs.z3fastmem_size);
	  currprefs.z3fastmem_size = 0;
	  err = 1;
    }

    if (currprefs.address_space_24 && (currprefs.gfxmem_size != 0 || currprefs.z3fastmem_size != 0)) {
	currprefs.z3fastmem_size = currprefs.gfxmem_size = 0;
	write_log ("Can't use a graphics card or Zorro III fastmem when using a 24 bit\n"
		 "address space - sorry.\n");
    }
    if (currprefs.bogomem_size != 0 && currprefs.bogomem_size != 0x80000 && currprefs.bogomem_size != 0x100000 && currprefs.bogomem_size != 0x1C0000)
    {
	currprefs.bogomem_size = 0;
	write_log ("Unsupported bogomem size!\n");
	err = 1;
    }

    if (currprefs.chipmem_size > 0x200000 && currprefs.fastmem_size != 0) {
	write_log ("You can't use fastmem and more than 2MB chip at the same time!\n");
	currprefs.fastmem_size = 0;
	err = 1;
    }
#if 0
    if (currprefs.m68k_speed < -1 || currprefs.m68k_speed > 20) {
	write_log ("Bad value for -w parameter: must be -1, 0, or within 1..20.\n");
	currprefs.m68k_speed = 4;
	err = 1;
    }
#endif

    if (currprefs.produce_sound < 0 || currprefs.produce_sound > 3) {
	write_log ("Bad value for -S parameter: enable value must be within 0..3\n");
	currprefs.produce_sound = 0;
	err = 1;
    }
#ifdef JIT
    if (currprefs.comptrustbyte < 0 || currprefs.comptrustbyte > 3) {
	write_log ("Bad value for comptrustbyte parameter: value must be within 0..2\n");
	currprefs.comptrustbyte = 1;
	err = 1;
    }
    if (currprefs.comptrustword < 0 || currprefs.comptrustword > 3) {
	write_log ("Bad value for comptrustword parameter: value must be within 0..2\n");
	currprefs.comptrustword = 1;
	err = 1;
    }
    if (currprefs.comptrustlong < 0 || currprefs.comptrustlong > 3) {
	write_log ("Bad value for comptrustlong parameter: value must be within 0..2\n");
	currprefs.comptrustlong = 1;
	err = 1;
    }
    if (currprefs.comptrustnaddr < 0 || currprefs.comptrustnaddr > 3) {
	write_log ("Bad value for comptrustnaddr parameter: value must be within 0..2\n");
	currprefs.comptrustnaddr = 1;
	err = 1;
    }
    if (currprefs.compnf < 0 || currprefs.compnf > 1) {
	write_log ("Bad value for compnf parameter: value must be within 0..1\n");
	currprefs.compnf = 1;
	err = 1;
    }
    if (currprefs.comp_hardflush < 0 || currprefs.comp_hardflush > 1) {
	write_log ("Bad value for comp_hardflush parameter: value must be within 0..1\n");
	currprefs.comp_hardflush = 1;
	err = 1;
    }
    if (currprefs.comp_constjump < 0 || currprefs.comp_constjump > 1) {
	write_log ("Bad value for comp_constjump parameter: value must be within 0..1\n");
	currprefs.comp_constjump = 1;
	err = 1;
    }
    if (currprefs.comp_oldsegv < 0 || currprefs.comp_oldsegv > 1) {
	write_log ("Bad value for comp_oldsegv parameter: value must be within 0..1\n");
	currprefs.comp_oldsegv = 1;
	err = 1;
    }
		/* round it */
		foo=(int) currprefs.cachesize / 1024;
		currprefs.cachesize=foo*1024;
    if (currprefs.cachesize < 0 || currprefs.cachesize > 16384) {
	write_log ("Bad value for cachesize parameter: value must be within 0..16384\n");
	currprefs.cachesize = 0;
	err = 1;
    }
#endif
    if (currprefs.cpu_level < 2 && currprefs.z3fastmem_size > 0) {
	write_log ("Z3 fast memory can't be used with a 68000/68010 emulation. It\n"
		 "requires a 68020 emulation. Turning off Z3 fast memory.\n");
	currprefs.z3fastmem_size = 0;
	err = 1;
    }
    if (currprefs.gfxmem_size > 0 && (currprefs.cpu_level < 2 || currprefs.address_space_24)) {
	write_log ("Picasso96 can't be used with a 68000/68010 or 68EC020 emulation. It\n"
		 "requires a 68020 emulation. Turning off Picasso96.\n");
	currprefs.gfxmem_size = 0;
	err = 1;
    }
#ifndef BSDSOCKET
    if (currprefs.socket_emu) {
	write_log ("Compile-time option of BSDSOCKET was not enabled.  You can't use bsd-socket emulation.\n");
	currprefs.socket_emu = 0;
	err = 1;
    }
#endif

    if (currprefs.nr_floppies < 0 || currprefs.nr_floppies > 4) {
	write_log ("Invalid number of floppies.  Using 4.\n");
	currprefs.nr_floppies = 4;
	currprefs.dfxtype[0] = 0;
	currprefs.dfxtype[1] = 0;
	currprefs.dfxtype[2] = 0;
	currprefs.dfxtype[3] = 0;
	err = 1;
    }

    if (currprefs.floppy_speed > 0 && currprefs.floppy_speed < 10) {
	currprefs.floppy_speed = 100;
    }
    if (currprefs.input_mouse_speed < 1 || currprefs.input_mouse_speed > 1000) {
	currprefs.input_mouse_speed = 100;
    }

    if (currprefs.collision_level < 0 || currprefs.collision_level > 3) {
	write_log ("Invalid collision support level.  Using 1.\n");
	currprefs.collision_level = 1;
	err = 1;
    }
    fixup_prefs_dimensions (&currprefs);

#ifdef CPU_68000_ONLY
    currprefs.cpu_level = 0;
#endif
#ifndef CPUEMU_0
    currprefs.cpu_compatible = 1;
    currprefs.address_space_24 = 1;
#endif
#if !defined(CPUEMU_5) && !defined (CPUEMU_6)
    currprefs.cpu_compatible = 0;
    currprefs.address_space_24 = 0;
#endif
#if !defined (CPUEMU_6)
    currprefs.cpu_cycle_exact = currprefs.blitter_cycle_exact = 0;
#endif
#ifndef AGA
    currprefs.chipset_mask &= ~CSMASK_AGA;
#endif
#ifndef AUTOCONFIG
    currprefs.z3fastmem_size = 0;
    currprefs.fastmem_size = 0;
    currprefs.gfxmem_size = 0;
#endif
#if !defined (BSDSOCKET)
    currprefs.socket_emu = 0;
#endif
#if !defined (SCSIEMU)
    currprefs.scsi = 0;
#ifdef _WIN32
    currprefs.win32_aspi = 0;
#endif
#endif

    if(!currprefs.gfx_afullscreen) {
      /* if we are not fullscreen, we oepn a window on the
       * specified public screen or on wb, if NULL
       * 1 == UAESCREENTYPE_PUBLIC
       *
       * as we ignore picasso full screen option, we
       * handle it analog to amiga_fullscreen
       */
      currprefs.amiga_screen_type = 1;
      currprefs.gfx_pfullscreen = 0;
    }
    else {
      currprefs.gfx_pfullscreen = 1;
    }

    fixup_prefs_joysticks (&currprefs);

    if (err)
	write_log ("Please use \"uae -h\" to get usage information.\n");
}


#ifndef DONT_PARSE_CMDLINE

void usage (void)
{
    cfgfile_show_usage ();
}

static void show_version (void)
{
#ifdef PACKAGE_VERSION
    write_log (PACKAGE_NAME " " PACKAGE_VERSION "\n");
#else
    write_log ("UAE %d.%d.%d\n", UAEMAJOR, UAEMINOR, UAESUBREV);
#endif
    write_log ("Build date: " __DATE__ " " __TIME__ "\n");
}

static void show_version_full (void)
{
    write_log ("\n");
    show_version ();
    write_log ("\nCopyright 2009-2012 Oliver Brunner and contributors.\n");
    write_log ("Based on source code from:\n");
    write_log ("UAE    - copyright 1995-2002 Bernd Schmidt;\n");
    write_log ("WinUAE - copyright 1999-2007 Toni Wilen.\n");
    write_log ("E-UAE  - copyright 2003-2007 Richard Drummond.\n");
    write_log ("See the source code for a full list of contributors.\n");

    write_log ("This is free software; see the file COPYING for copying conditions.  There is NO\n");
    write_log ("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

static void parse_optionfile_parameter (int argc, char **argv) {
  int i;

  default_optionsfile[0]='\0';

  for (i = 1; i < argc; i++) {
    if (strcmp (argv[i], "-f") == 0) {
      if (i + 1 == argc) {
        write_log ("Missing argument for '-f' option.\n");
      } else {
        /* just in case, we need it */
        strcpy (default_optionsfile, optionsfile);
        /* use -f value */
        strncpy(optionsfile, argv[i+1], 255);
        return;
      }
    }
  }
}
 
static void parse_cmdline (int argc, char **argv) {

  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp (argv[i], "-cfgparam") == 0) {
      if (i + 1 < argc) {
        i++;
      }
    } else if (strncmp (argv[i], "-config=", 8) == 0) {
#ifdef FILESYS
      //free_mountinfo (currprefs.mountinfo);
#endif
      write_log("WARNING: -config= is GONE! please use -f!\n");

      //if (cfgfile_load (&currprefs, argv[i] + 8, 0)) {
        //strcpy (optionsfile, argv[i] + 8);
      //}
    }
    /* Check for new-style "-f xxx" argument, where xxx is config-file */
    else if (strcmp (argv[i], "-f") == 0) {
      if (i + 1 != argc) {
        /* if there is a parameter (there should be one..), skip it */
        i++;
      }

      /* we have handled that already! */
#if 0
      if (i + 1 == argc) {
        write_log ("Missing argument for '-f' option.\n");
      } else {
#ifdef FILESYS
        free_mountinfo (currprefs.mountinfo);
#endif
        //uae_save_config_to_file("sys:pre.cfg");
        i++;
        if (cfgfile_load (&currprefs, argv[i], 0)) {
          /* load both config files new! */
          cfgfile_load (&changed_prefs, argv[i], 0);
          strcpy (optionsfile, argv[i]);
        }
        //uae_save_config_to_file("sys:post.cfg");
      }
#endif
    } else if (strcmp (argv[i], "-s") == 0) {
      if (i + 1 == argc) {
        write_log ("Missing argument for '-s' option.\n");
      }
      else {
        cfgfile_parse_line (&currprefs, argv[++i], 0);
      }
    } else if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "-help") == 0) {
      usage ();
      exit (0);
    } else if (strcmp (argv[i], "-version") == 0) {
      show_version_full ();
      exit (0);
    } 
    else if (strcmp (argv[i], "-scsilog") == 0) {
      log_scsi = 1;
    } 
    else if (strcmp (argv[i], "-splash_text") == 0) {
      if (i + 1 == argc) {
        write_log ("Missing argument for '-splash_text' option.\n");
      }
      else {
        kprintf("splash_text: %s\n",argv[i+1]);
        strncpy(currprefs.splash_text, argv[++i], 250);
        if(currprefs.splash_time) {
          do_splash(currprefs.splash_text, currprefs.splash_time);
          show_splash();
        }
      }
    } 
    else if (strcmp (argv[i], "-splash_timeout") == 0) {
      if (i + 1 == argc) {
        write_log ("Missing argument for '-splash_text' option.\n");
      }
      else {
        currprefs.splash_time=atoi(argv[++i]);;
        kprintf("splash_time: %d\n", currprefs.splash_time);
        if(currprefs.splash_text) {
          do_splash(currprefs.splash_text, currprefs.splash_time);
          show_splash();
        }
      }

    } 
    else {
      if (argv[i][0] == '-' && argv[i][1] != '\0') {
        const char *arg = argv[i] + 2;
        int extra_arg = *arg == '\0';
        if (extra_arg) {
          arg = i + 1 < argc ? argv[i + 1] : 0;
        }
        if (parse_cmdline_option (&currprefs, argv[i][1], (char*)arg) && extra_arg) {
          i++;
        }
      }
    }
  }
}
#endif

static void parse_cmdline_and_init_file (int argc, char **argv) {

  strcpy (optionsfile, "");

	strcpy (optionsfile, "PROGDIR:");
  strcat (optionsfile, OPTIONSFILENAME);

  /* if -f is supplied, overwrite optionsfile */
  parse_optionfile_parameter(argc, argv);

  if (! cfgfile_load (&currprefs, optionsfile, 0)) {
    write_log ("failed to load config '%s'\n", optionsfile);
    if(default_optionsfile[0] != '\0') {
      /* -f was supplied but failed, so we load the default config file .. (?) *
       * parse_optionfile_parameter backupped default_optionsfile for us       */
      strcpy(optionsfile, default_optionsfile);
      write_log ("ATTENTION: trying %s instead\n", optionsfile);
      if(! cfgfile_load (&currprefs, optionsfile, 0)) {
        write_log ("failed to load config '%s', too!\n", optionsfile);
      }
    }

#ifdef OPTIONS_IN_HOME
    /* sam: if not found in $HOME then look in current directory */
    char *saved_path = strdup (optionsfile);
    strcpy (optionsfile, OPTIONSFILENAME);
    if (! cfgfile_load (&currprefs, optionsfile, 0) ) {
      /* If not in current dir either, change path back to home
       * directory - so that a GUI can save a new config file there */
      strcpy (optionsfile, saved_path);
    }

    free (saved_path);
#endif
  }

  fix_options ();

  parse_cmdline (argc, argv);

  fix_options ();
}

/*
 * Save the currently loaded configuration.
 */
void uae_save_config (void)
{
    FILE *f;
    char tmp[512];

    /* Back up the old file.  */
    strcpy (tmp, optionsfile);
    strcat (tmp, ".backup");
    write_log ("Backing-up config file '%s' to '%s'\n", optionsfile, tmp);
    rename (optionsfile, tmp);

    write_log ("Writing new config file '%s'\n", optionsfile);
    f = fopen (optionsfile, "w");
    if (f == NULL) {
	gui_message ("Error saving configuration file!\n"); // FIXME - better error msg.
	return;
    }

    // FIXME  - either fix this nonsense, or only allow config to be saved when emulator is stopped.
    if (uae_get_state () == UAE_STATE_STOPPED)
	save_options (f, &changed_prefs, 0);
    else
	save_options (f, &currprefs, 0);

    fclose (f);
}

static void uae_save_config_to_file (char *filename) {

  FILE *f;
  char changed[512];
  char current[512];

  strcpy (changed, filename);
  strcat (changed, ".changed");
  strcpy (current, filename);
  strcat (current, ".current");

  write_log ("Writing new config file '%s'\n", changed);
  f = fopen (changed, "w");
  if (f == NULL) {
    gui_message ("Error saving configuration file!\n"); // FIXME - better error msg.
    return;
  }

  save_options (f, &changed_prefs, 0);
  fclose (f);


  write_log ("Writing new config file '%s'\n", current);
  f = fopen (current, "w");
  if (f == NULL) {
    gui_message ("Error saving configuration file!\n"); // FIXME - better error msg.
    return;
  }
  save_options (f, &currprefs, 0);
  fclose (f);

}

/*
 * (re-)load the current configuration.
 *
 * don't push backup_prefs on the stack, it will exceed the limit .. :(
 */
void uae_load_config (void) {

  struct uae_prefs *backup_prefs;

  backup_prefs=(struct uae_prefs *) AllocVec(sizeof(struct uae_prefs), MEMF_CLEAR);

  write_log ("Load config '%s'\n", optionsfile);

  *backup_prefs=currprefs;

#ifdef FILESYS
  free_mountinfo (currprefs.mountinfo);
#endif
  default_prefs (&currprefs, 0);

  fix_options ();

  if (! cfgfile_load (&currprefs, optionsfile, 0)) {
    write_log ("failed to load config '%s'\n", optionsfile);
    /* reset to last settings */
    currprefs=*backup_prefs;
  }

  fix_options ();

	changed_prefs=currprefs;
  gui_update();

  FreeVec(backup_prefs);
}


/*
 * A first cut at better state management...
 */

static int uae_state;
static int uae_target_state;

int uae_get_state (void)
{
    return uae_state;
}


/***********************************************
 * For j-uae we need to care for resets/crashes
 * of amigaOS, as we possibly have quite
 * some windows/threads hanging around,
 * that have no amigaOS counterparts
 * anymore.
 *
 * But be carefull: Not every reset triggers
 * set_state. Crashes etc are better handled
 * in custom.c/custom_reset
 ***********************************************/
static void set_state (int state) {

  switch(state) {
    case UAE_STATE_RUNNING:
      break;
    case UAE_STATE_PAUSED:
      break;
    case UAE_STATE_STOPPED:
    case UAE_STATE_COLD_START:
    case UAE_STATE_WARM_START:

#ifdef __AROS__
      j_reset();
#endif
      break;
    case UAE_STATE_QUITTING:
      break;
  }
  uae_state = state;
  gui_notify_state (state);
  graphics_notify_state (state);
}

int uae_state_change_pending (void)
{
    return uae_state != uae_target_state;
}

void uae_start (void)
{
    uae_target_state = UAE_STATE_COLD_START;
}

void uae_pause (void)
{
    if (uae_target_state == UAE_STATE_RUNNING)
	uae_target_state = UAE_STATE_PAUSED;
}

void uae_resume (void)
{
    if (uae_target_state == UAE_STATE_PAUSED)
	uae_target_state = UAE_STATE_RUNNING;
}

void uae_quit (void)
{
    if (uae_target_state != UAE_STATE_QUITTING) {
	uae_target_state = UAE_STATE_QUITTING;
    }
}

void uae_stop (void)
{
    if (uae_target_state != UAE_STATE_QUITTING && uae_target_state != UAE_STATE_STOPPED) {
	uae_target_state = UAE_STATE_STOPPED;
	restart_config[0] = 0;
    }
}

void uae_reset (int hard_reset)
{
    switch (uae_target_state) {
	case UAE_STATE_QUITTING:
	case UAE_STATE_STOPPED:
	case UAE_STATE_COLD_START:
	case UAE_STATE_WARM_START:

	    /* Do nothing */
	    break;
	default:
	    uae_target_state = hard_reset ? UAE_STATE_COLD_START : UAE_STATE_WARM_START;
    }
}

/* This needs to be rethought */
void uae_restart (int opengui, char *cfgfile)
{
    uae_stop ();
    restart_program = opengui > 0 ? 1 : (opengui == 0 ? 2 : 3);
    restart_config[0] = 0;
    if (cfgfile)
	strcpy (restart_config, cfgfile);
}


/*
 * Early initialization of emulator, parsing of command-line options,
 * and loading of config files, etc.
 *
 * TODO: Need better cohesion! Break this sucker up!
 */
static int do_preinit_machine (int argc, char **argv)
{
    if (! graphics_setup ()) {
	exit (1);
    }
    if (restart_config[0]) {
#ifdef FILESYS
	free_mountinfo (currprefs.mountinfo);
#endif
	default_prefs (&currprefs, 0);
	fix_options ();
    }

#ifdef NATMEM_OFFSET
    init_shm ();
#endif

#ifdef FILESYS
    rtarea_init ();
    hardfile_install ();
#endif

    if (restart_config[0])
        parse_cmdline_and_init_file (argc, argv);
    else {
	currprefs = changed_prefs;
    }

    uae_inithrtimer ();

    machdep_init ();

    if (! audio_setup ()) {
	write_log ("Sound driver unavailable (1): Sound output disabled\n");
	currprefs.produce_sound = 0;
    }
    inputdevice_init ();

    return 1;
}

/*
 * Initialization of emulator proper
 */
static int do_init_machine (void)
{

#ifdef JIT
    if (!(( currprefs.cpu_level >= 2 ) && ( currprefs.address_space_24 == 0 ) && ( currprefs.cachesize )))
	canbang = 0;
#endif

#ifdef _WIN32
    logging_init(); /* Yes, we call this twice - the first case handles when the user has loaded
		       a config using the cmd-line.  This case handles loads through the GUI. */
#endif

#ifdef SAVESTATE
    savestate_init ();
#endif
#ifdef SCSIEMU
    scsidev_install ();
#endif
#ifdef AUTOCONFIG
    /* Install resident module to get 8MB chipmem, if requested */
    rtarea_setup ();
#endif

    keybuf_init (); /* Must come after init_joystick */

#ifdef AUTOCONFIG
    expansion_init ();
#endif
    memory_init ();
/* alive */
    memory_reset ();
/* dead */

#ifdef FILESYS
    filesys_install ();
#endif
#ifdef AUTOCONFIG
    bsdlib_install ();
    emulib_install ();
    uaeexe_install ();
    native2amiga_install ();
#endif

    if (custom_init ()) { /* Must come after memory_init */
#if defined SERIAL_PORT || defined AROS_SERIAL_HACK
	serial_init ();
#endif
	DISK_init ();

	reset_frame_rate_hack ();
	init_m68k(); /* must come after reset_frame_rate_hack (); */

	//gui_update ();

	if (graphics_init ()) {

#ifdef DEBUGGER
	    setup_brkhandler ();

	    if (currprefs.start_debugger && debuggable ())
		activate_debugger ();
#endif

#ifdef WIN32
#ifdef FILESYS
	    filesys_init (); /* New function, to do 'add_filesys_unit()' calls at start-up */
#endif
#endif
	    if (sound_available && currprefs.produce_sound > 1 && ! audio_init ()) {
		write_log ("Sound driver unavailable (2): Sound output disabled\n");
#if 0
		kprintf ("Sound driver unavailable (2): Sound output disabled\n");
		kprintf("sound_available: %d\n",sound_available);
		kprintf("currprefs.produce_sound: %d\n",currprefs.produce_sound);
#endif

		currprefs.produce_sound = 0;
	    }

	    return 1;
	}
    }
    return 0;
}

/*
 * Helper for reset method
 */
static void reset_all_systems (void)
{
    init_eventtab ();

    memory_reset ();
#ifdef BSDSOCKET
    bsdlib_reset ();
#endif
#ifdef FILESYS
    filesys_reset ();
    filesys_start_threads ();
    hardfile_reset ();
#endif
#ifdef SCSIEMU
    scsidev_reset ();
    scsidev_start_threads ();
#endif
}

/*
 * Reset emulator
 */
static void do_reset_machine (int hardreset)
{
#ifdef SAVESTATE
    if (savestate_state == STATE_RESTORE)
	restore_state (savestate_fname);
    else if (savestate_state == STATE_REWIND)
	savestate_rewind ();
#endif
    /* following three lines must not be reordered or
     * fastram state restore breaks
     */
    reset_all_systems ();
    customreset ();
    m68k_reset ();
    if (hardreset) {
	memset (chipmemory, 0, allocated_chipmem);
	write_log ("chipmem cleared\n");
    }
#ifdef SAVESTATE
    /* We may have been restoring state, but we're done now.  */
    if (savestate_state == STATE_RESTORE || savestate_state == STATE_REWIND)
    {
	map_overlay (1);
	fill_prefetch_slow (&regs); /* compatibility with old state saves */
    }
    savestate_restore_finish ();
#endif

    fill_prefetch_slow (&regs);
    if (currprefs.produce_sound == 0)
	eventtab[ev_audio].active = 0;
    handle_active_events ();

    inputdevice_updateconfig (&currprefs);
}

/*
 * Run emulator
 */
static void do_run_machine (void)
{
#if defined (NATMEM_OFFSET) && defined( _WIN32 ) && !defined( NO_WIN32_EXCEPTION_HANDLER )
    extern int Evaluae_Exception ( LPEXCEPTION_POINTERS blah, int n_except );
    __try
#endif
    {
	m68k_go (1);
    }
#if defined (NATMEM_OFFSET) && defined( _WIN32 ) && !defined( NO_WIN32_EXCEPTION_HANDLER )
    __except( Evaluae_Exception( Getuae_ExceptionInformation(), Getuae_ExceptionCode() ) )
    {
	// Evaluae_Exception does the good stuff...
    }
#endif
}

/*
 * Exit emulator
 *
 * o1i: this is only called, if the uae window is open,
 *      not if only the gui is active.
 */
static void do_exit_machine (void)
{

    graphics_leave ();
    inputdevice_close ();

    j_quit();

#ifdef SCSIEMU
    scsidev_exit ();
#endif
    DISK_free ();
    audio_close ();
    dump_counts ();
#if defined SERIAL_PORT || defined AROS_SERIAL_HACK
    serial_exit ();
#endif
#ifdef CD32
    akiko_free ();
#endif
    /* leave the gui running!? */
    gui_exit (); /* empty function */

#ifdef AUTOCONFIG
    expansion_cleanup ();
#endif
#ifdef FILESYS
    filesys_cleanup ();
    hardfile_cleanup (); 
#endif
#ifdef SAVESTATE
    savestate_free ();
#endif
    memory_cleanup ();
    cfgfile_addcfgparam (0);
}

#if defined GTKMUI
void gui_shutdown (void);
#endif
/*
 * Here's where all the action takes place!
 */

void real_main (int argc, char **argv) {
	ULONG waitcount;
	int err;
	BOOL first_time=TRUE;

   /* j-uae writes to stdout, so if you want to
    * use 'T:uae.log' you better start it
    * with
    * j-uae >T:uae.log
    * ;)
    */
#if 0
    set_logfile("T:uae.log");
#endif

	uae_sem_init (&gui_main_wait_sem, 0, 0);

	gui_init (argc, argv);
	show_version ();

	currprefs.mountinfo = changed_prefs.mountinfo = &options_mountinfo;
	restart_program = 1;
#ifdef _WIN32
	sprintf (restart_config, "%sConfigurations\\", start_path);
#endif
	strcat (restart_config, OPTIONSFILENAME);

	/* Initial state is stopped */
	uae_target_state = UAE_STATE_STOPPED;

		while (uae_target_state != UAE_STATE_QUITTING) {

		set_state (uae_target_state);
		do_preinit_machine (argc, argv);

		changed_prefs = currprefs;

		/* start launchd, if necessary */
		if(currprefs.jlaunch) {
			aros_launch_start_thread();
		}

		/* Handle GUI at start-up */
		err = gui_open ();

		/* we always wait for the GUI! */
		/* if we wait for the GUI here, a manual start with the START button hangs here of course! */
		//DEBUG_LOG("waiting for gui_main_wait_sem ..\n");
		//uae_sem_wait(&gui_main_wait_sem);
		//DEBUG_LOG("got gui_main_wait_sem\n");

		if (err >= 0) {
			/* GUI opened successfully */

		  DEBUG_LOG("currprefs.start_gui: %d gui_visible() %d\n", currprefs.start_gui, gui_visible());

			if(first_time) {
				DEBUG_LOG("first time ..\n");
				/* skip hack for the next time */
				first_time=FALSE;
				/* when we first test for gui_visible it might not be in the correct state (race..) */
				if(!currprefs.start_gui) {
					DEBUG_LOG("no GUI is planned to open => start uae\n");
					uae_start();
				}
			}
			else if(!gui_visible()) {
				DEBUG_LOG("GUI is invisible => start uae\n");
				/* 
				 * GUI will stay hidden, so we start UAE manually here: 
				 */
				uae_start();
			}
			else {
				DEBUG_LOG("GUI is visible => let the user click on the start button\n");
			}

			/* wait for state button to be pressed */
			do {
				gui_handle_events ();

				uae_msleep (10);
			} while (!uae_state_change_pending ());

		} else if (err == - 1) {
			/* will this ever happen !? */

			if (restart_program == 3) {
				restart_program = 0;

				uae_quit ();
			}
		} else {
			/* fragg. No Gui, no fun */
			uae_quit ();
		}

		currprefs = changed_prefs;
		fix_options ();
		inputdevice_init ();

		restart_program = 0;

		if (uae_target_state == UAE_STATE_QUITTING) {
			break;
		}

		uae_target_state = UAE_STATE_COLD_START;

	/* Start emulator proper. */
		if (!do_init_machine ())
				break;

		while (uae_target_state != UAE_STATE_QUITTING && uae_target_state != UAE_STATE_STOPPED) {
	    /* Reset */
	    set_state (uae_target_state);
	    do_reset_machine (uae_state == UAE_STATE_COLD_START);

	    /* Running */
	    uae_target_state = UAE_STATE_RUNNING;

	    /*
 	     * Main Loop
	     */
	    do {
		set_state (uae_target_state);

		/* Run emulator. */
		do_run_machine ();

		if (uae_target_state == UAE_STATE_PAUSED) {
		    /* Paused */
		    set_state (uae_target_state);

		    audio_pause ();

		    /* While UAE is paused we have to handle
		     * input events, etc. ourselves.
		     */
		    do {
			gui_handle_events ();

			handle_events ();

			/* Manually pump input device */
                	inputdevicefunc_keyboard.read ();
			inputdevicefunc_mouse.read ();
			inputdevicefunc_joystick.read ();
			inputdevice_handle_inputcode ();

			/* Don't busy wait. */
			uae_msleep (10);

		    } while (!uae_state_change_pending ());

		    audio_resume ();
		}

	    } while (uae_target_state == UAE_STATE_RUNNING);

	    /*
	     * End of Main Loop
	     *
	     * We're no longer running or paused.
	     */

	    set_inhibit_frame (IHF_QUIT_PROGRAM);

#ifdef FILESYS
	    /* Ensure any cached changes to virtual filesystem are flushed before
	     * resetting or exitting. */
	    filesys_prepare_reset ();
#endif

	} /* while (!QUITTING && !STOPPED) */


	do_exit_machine ();


        /* TODO: This stuff is a hack. What we need to do is
	 * check whether a config GUI is available. If not,
	 * then quit.
	 */
	restart_program = 3;
    }

    /* o1i: aehm, and if there is still a gui? */

#if 0
    uae_stop();
    gui_notify_state(uae_get_state());
#endif

    /* o1i:
     * what would be nice is, is to just shut down the emulator
     * window and tell the GUI, the emulator is stopped.
     * That seems not to be so easy, so I'll just shutdown all.
     *
     * gui_shutdown() at this place was not there in the original sources.
     */

    gui_shutdown (); /* seems as if under UNIX, e-uae let's the gui die
                      * automatically. Not really nice?
		      */
    zfile_exit ();
  //set_logfile(NULL);

    /* if we were never running before, nobody will stop this one.
     * it is safe to stop it, even if it is not running
     */
    aros_launch_kill_thread();
    stop_proxy_thread();

    waitcount=0;
    while(waitcount<20 && (aros_cli_task || aros_launch_task || 
                           janus_windows || janus_screens ||
                           proxy_thread) ) {
      waitcount++;
      sleep(1);
    }

    if(waitcount==20) {
      kprintf("ERROR: could not kill all threads..\n");
      printf("ERROR: could not kill all threads..\n");
      kprintf("aros_launch_task: %lx\n", aros_launch_task);
      kprintf("aros_cli_task: %lx\n", aros_cli_task);
      kprintf("janus_windows: %lx\n", janus_windows);
      kprintf("janus_screens: %lx\n", janus_screens);
    }

}

#ifdef USE_SDL
int init_sdl (void)
{
    int result = (SDL_Init (SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE /*| SDL_INIT_AUDIO*/) == 0);
    if (result)
        atexit (SDL_Quit);

    return result;
}
#else
#define init_sdl()
#endif

#ifndef NO_MAIN_IN_MAIN_C
int main (int argc, char **argv)
{
    init_sdl ();
    gui_init (argc, argv);
    real_main (argc, argv);
    return 0;
}
#endif

#ifdef SINGLEFILE
uae_u8 singlefile_config[50000] = { "_CONFIG_STARTS_HERE" };
uae_u8 singlefile_data[1500000] = { "_DATA_STARTS_HERE" };
#endif

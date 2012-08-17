/************************************************************************ 
 *
 * gtkui.c
 *
 * Yet Another User Interface for the X11 version
 * User Interface for the AROS version
 *
 * X11:
 * The Tk GUI doesn't work.
 * The X Forms Library isn't available as source, and there aren't any
 * binaries compiled against glibc
 *
 * So let's try this...
 *
 * Copyright 1997-1998 Bernd Schmidt
 * Copyright 1998      Michael Krause
 * Copyright 2003-2007 Richard Drummond
 * Copyright 2009-2011 Oliver Brunner - aros<at>oliver-brunner.de
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

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "gui.h"
#include "newcpu.h"
#include "filesys.h"
#include "threaddep/thread.h"
#include "audio.h"
#include "savestate.h"
#include "debug.h"
#include "inputdevice.h"
#include "xwin.h"
#include "p96.h"
#include "version.h"

#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#ifdef __AROS__
#include <dos/dos.h>
#include <proto/arossupport.h>
#endif

#include "gui-gtk/cputypepanel.h"
#include "gui-gtk/cpuspeedpanel.h"
#include "gui-gtk/floppyfileentry.h"
#include "gui-gtk/led.h"
#include "gui-gtk/chipsettypepanel.h"
#include "gui-gtk/chipsetspeedpanel.h"
#include "gui-gtk/util.h"
#include "gui-gtk/display.h"
#include "gui-gtk/integration.h"
#include "gui-gtk/splash.h"

#include "od-amiga/j.h"

#define GUI_DEBUG 1
#ifdef  GUI_DEBUG
#define DEBUG_LOG(...) do { kprintf("%s:%d %s(): ",__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define DEBUG_LOG(...) do ; while(0)
#endif

#if 0
#ifdef __AROS__
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <libraries/cybergraphics.h>
#endif
#endif


//#define P96TRACING_ENABLED 1
#if P96TRACING_ENABLED
#define P96TRACE(x)	do { kprintf x; } while(0)
#else
#define P96TRACE(x)     do { ; } while(0)
#endif


#if defined GTKMUI
//#define MGTK_DEBUG 1
//#include "/home/oli/aros/gtk-mui/debug.h"
#endif

void chipsize_changed (void);
static void did_extchange (GtkWidget *w, gpointer data);

static int gui_active;

static int gui_available;

static GtkWidget *gui_window;

static GtkWidget *start_uae_widget;
static GtkWidget *stop_uae_widget;
static GtkWidget *pause_uae_widget;
static GtkWidget *reset_uae_widget;
static GtkWidget *debug_uae_widget;

static GtkWidget *config_load_widget;
static GtkWidget *config_load_from_widget;
static GtkWidget *config_save_widget;
static GtkWidget *config_save_as_widget;
static GtkWidget *config_path_widget;
static GtkWidget *config_description_widget;

GtkWidget  *chipsize_widget[6];
static const char *chiplabels[] = { "512 KB", "1 MB", "2 MB", "4 MB", "8 MB", NULL };

static GtkWidget  *bogosize_widget[5];
static const char *bogolabels[] = { "None", "512 KB", "1 MB", "1.8 MB", NULL };

extern GtkWidget  *fastsize_widget[5];
static const char *fastlabels[] = { "None", "1 MB", "2 MB", "4 MB", "8 MB", NULL };

static GtkWidget  *z3frame;
static GtkWidget  *z3size_widget[16];
static const char *z3labels[] = { "None", "1 MB", "2 MB", "4 MB", "8 MB",
				  "16 MB", "32 MB", "64 MB", "128 MB", "256 MB", 
				  "384 MB", "512 MB", "768 MB", "1 GB", "1.5 GB", NULL };

#ifdef PICASSO96
static GtkWidget  *p96frame;
static GtkWidget  *p96size_widget[11];
static const char *p96labels[] = { "None", "1 MB", "2 MB", "4 MB", "8 MB", 
                                   "16 MB", "32 MB", "64 MB", "128 MB", "256 MB", NULL };
#endif

static GtkWidget *rom_text_widget, *key_text_widget, *ext_text_widget;
static GtkWidget *rom_change_widget, *key_change_widget, *ext_change_widget;

static GtkWidget *floppy_widget[5]; /* df0, df1, df2 ,df3, NULL */

static char *new_disk_string[4]; /* ? */

static GtkWidget *power_led;

static GtkWidget *ctpanel;
static GtkWidget *cspanel;
extern GtkWidget *chipsettype_panel;
extern GtkWidget *chipsetspeed_panel;

static GtkWidget *hdpanel;
static GtkWidget *memorypanel;

static GtkWidget  *sound_widget[6];
static const char *soundlabels1[] = { "None", "No output", "Normal", "Accurate", NULL };

static GtkWidget  *sound_bits_widget[3];
static const char *soundlabels2[] = { "8 bit", "16 bit", NULL };

static GtkWidget  *sound_ch_widget[4];
static const char *soundlabels3[] = { "Mono", "Stereo", "Mixed", NULL };

//static GtkWidget *sound_freq_widget[3]; /* not used */

#ifdef JIT
static GtkWidget *jit_page;
#ifdef NATMEM_OFFSET
static GtkWidget *compbyte_widget[4], *compword_widget[4], *complong_widget[4];
static GtkWidget *compaddr_widget[4];
#endif
static GtkWidget *compnf_widget[2];
static GtkWidget *compfpu_widget[3], *comp_hardflush_widget[2];
static GtkWidget *comp_constjump_widget[3];
static GtkAdjustment *cachesize_adj;
#endif

#ifdef XARCADE
# define JOY_WIDGET_COUNT 10
#else
# define JOY_WIDGET_COUNT 8
#endif
static GtkWidget *catweasel_joystick_widget;
static GtkWidget *joy_widget[2][JOY_WIDGET_COUNT];
static const char *joylabels[] = {
    "None",
    "Joystick 0",
    "Joystick 1",
    "Mouse",
    "Numeric pad",
    "Cursor keys/Right Ctrl or Alt",
    "T/F/H/B/Left Alt",
#ifdef XARCADE
    "X-Arcade Left",
    "X-Arcade Right",
#endif
    NULL
};



static unsigned int prevledstate;

static GtkWidget *hdlist_widget;
static int selected_hd_row;
static GtkWidget *hdchange_button, *hddel_button;
static GtkWidget *devname_entry, *volname_entry, *path_entry;
static GtkWidget *readonly_widget, *bootpri_widget;
static GtkWidget *dirdlg;
static GtkWidget *dirdlg_ok;
static char dirdlg_devname[256], dirdlg_volname[256], dirdlg_path[256], floppydlg_path[256];


static GtkWidget *jdisp_panel=NULL;
static GtkWidget *jint_panel=NULL;

enum hdlist_cols {
    HDLIST_DEVICE,
    HDLIST_VOLUME,
    HDLIST_PATH,
    HDLIST_READONLY,
    HDLIST_HEADS,
    HDLIST_CYLS,
    HDLIST_SECS,
    HDLIST_RSRVD,
    HDLIST_SIZE,
    HDLIST_BLKSIZE,
    HDLIST_BOOTPRI,
//    HDLIST_FILESYSDIR,
    HDLIST_MAX_COLS
};

static const char *hdlist_col_titles[] = {
     "Device",
     "Volume",
     "File/Directory",
     "R/O",
     "Heads",
     "Cyl.",
     "Sec.",
     "Rsrvd",
     "Size",
     "Blksize",
     "Boot pri",
//    "Filesysdir?"
     NULL
};


static smp_comm_pipe to_gui_pipe;   // For sending messages to the GUI from UAE
static smp_comm_pipe from_gui_pipe; // For sending messages from the GUI to UAE

/*
 * Messages sent to GUI from UAE via to_gui_pipe
 */
enum gui_commands {
    GUICMD_STATE_CHANGE, // Tell GUI about change in emulator state.
    GUICMD_SHOW,         // Show yourself
    GUICMD_UPDATE,       // Refresh your state from changed preferences
    GUICMD_DISKCHANGE,   // Hey! A disk has been changed. Do something!
    GUICMD_MSGBOX,       // Display a message box for me, please
    GUICMD_SPLASH,       // Display a splash screen
    GUICMD_FLOPPYDLG     // Open a floppy insert dialog
};

enum uae_commands {
    UAECMD_START,
    UAECMD_STOP,
    UAECMD_QUIT,
    UAECMD_RESET,
    UAECMD_PAUSE,
    UAECMD_RESUME,
    UAECMD_DEBUG,
    UAECMD_SAVE_CONFIG,
    UAECMD_EJECTDISK,
    UAECMD_INSERTDISK,
    UAECMD_SELECT_ROM,
    UAECMD_SELECT_KEY,
    UAECMD_LOAD_CONFIG
};


static uae_sem_t gui_sem;        // For mutual exclusion on various prefs settings
static uae_sem_t gui_update_sem; // For synchronization between gui_update() and the GUI thread
static uae_sem_t gui_init_sem;   // For the GUI thread to tell UAE that it's ready.
static uae_sem_t gui_quit_sem;   // For the GUI thread to tell UAE that it's quitting.

extern uae_sem_t gui_main_wait_sem;   // For the GUI thread to tell UAE/main that it's ready.

static volatile int quit_gui = 0, quitted_gui = 0;

static void create_guidlg (void);

static void do_message_box (const gchar *title, const gchar *message, gboolean modal, gboolean wait);
static void handle_message_box_request (smp_comm_pipe *msg_pipe);
static GtkWidget *make_message_box (const gchar *title, const gchar *message, int modal, uae_sem_t *sem);
#if 0
static void open_splash_pipe (smp_comm_pipe *msg_pipe);
#endif
void on_message_box_quit (GtkWidget *w, gpointer user_data);

int find_current_toggle (GtkWidget **widgets, int count);

static void set_mem32_widgets_state (void) {

  //D(bug("enable: %d\n",enable));

#ifdef AUTOCONFIG

  int enable = changed_prefs.cpu_level >= 2 && ! changed_prefs.address_space_24;

  gtk_widget_set_sensitive (z3frame, enable);
# ifdef PICASSO96
  gtk_widget_set_sensitive (p96frame, enable);
#endif

#endif

#ifdef JIT
  gtk_widget_set_sensitive (jit_page, changed_prefs.cpu_level >= 2);
#endif
}

/* o1i: HERE !! */
static void set_cpu_state (void)
{
    int i;

    DEBUG_LOG ("set_cpu_state: %d %d %d\n", changed_prefs.cpu_level,
    changed_prefs.address_space_24, changed_prefs.m68k_speed);

    DEBUG_LOG ("set_cpu_state: %d %d %d\n", currprefs.cpu_level,
    currprefs.address_space_24, currprefs.m68k_speed);


    //DebOut ("%d %d %d\n", changed_prefs.cpu_level, changed_prefs.address_space_24, changed_prefs.m68k_speed);

    cputypepanel_set_cpulevel      (CPUTYPEPANEL (ctpanel), currprefs.cpu_level);
    cputypepanel_set_addr24bit     (CPUTYPEPANEL (ctpanel), currprefs.address_space_24);
    cputypepanel_set_accuracy      (CPUTYPEPANEL (ctpanel), currprefs.cpu_compatible, currprefs.cpu_cycle_exact);
    cpuspeedpanel_set_cpuspeed     (CPUSPEEDPANEL (cspanel), currprefs.m68k_speed);
    cpuspeedpanel_set_cpuidle      (CPUSPEEDPANEL (cspanel), currprefs.cpu_idle);

    set_mem32_widgets_state ();
    //DebOut("exit\n");
}

#if 0
static void set_chipset_state (void) {

    chipsettypepanel_set_chipset_mask     (CHIPSETTYPEPANEL  (chipsettype_panel),  currprefs.chipset_mask);
    chipsettypepanel_set_ntscmode         (CHIPSETTYPEPANEL  (chipsettype_panel),  currprefs.ntscmode);
    chipsetspeedpanel_set_framerate       (CHIPSETSPEEDPANEL (chipsetspeed_panel), currprefs.gfx_framerate);
    chipsetspeedpanel_set_collision_level (CHIPSETSPEEDPANEL (chipsetspeed_panel), currprefs.collision_level);
    chipsetspeedpanel_set_immediate_blits (CHIPSETSPEEDPANEL (chipsetspeed_panel), currprefs.immediate_blits);
}
#endif

static void set_sound_state (void)
{
    int stereo = currprefs.sound_stereo + currprefs.sound_mixed_stereo;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sound_widget[currprefs.produce_sound]), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sound_ch_widget[stereo]), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sound_bits_widget[currprefs.sound_bits == 16]), 1);
}

static void set_mem_state (void)
{
    int t, t2;

    t = 0;
    t2 = currprefs.chipmem_size;
    while (t < 4 && t2 > 0x80000)
	t++, t2 >>= 1;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chipsize_widget[t]), 1);

    t = 0;
    t2 = currprefs.bogomem_size;
    while (t < 3 && t2 >= 0x80000)
	t++, t2 >>= 1;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bogosize_widget[t]), 1);

    t = 0;
    t2 = currprefs.fastmem_size;
    while (t < 4 && t2 >= 0x100000)
	t++, t2 >>= 1;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fastsize_widget[t]), 1);

  t=0;
  switch(currprefs.z3fastmem_size) {
    case 384 * 0x100000:
      t=10;
      break;
    case 512 * 0x100000:
      t=11;
      break;
    case 768 * 0x100000:
      t=12;
      break;
    case 1024 * 0x100000:
      t=13;
      break;
    case 0x60000000:
      t=14;
      break;
    default:
      t2 = currprefs.z3fastmem_size;
      while (t < 14 && t2 >= 0x100000)
  	  t++, t2 >>= 1;
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (z3size_widget[t]), 1);

#ifdef PICASSO96
    t = 0;
    t2 = currprefs.gfxmem_size;
    while (t < 10 && t2 >= 0x100000)
	t++, t2 >>= 1;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (p96size_widget[t]), 1);
#endif

    if(currprefs.romfile[0] != '\0') {
      gtk_label_set_text (GTK_LABEL (rom_text_widget), currprefs.romfile);
    }
    else {
      gtk_label_set_text (GTK_LABEL (rom_text_widget), changed_prefs.romfile);
    }

    if(currprefs.romextfile[0] != '\0') {
      gtk_label_set_text (GTK_LABEL (ext_text_widget), currprefs.romextfile);
    }
    else {
      gtk_label_set_text (GTK_LABEL (ext_text_widget), changed_prefs.romextfile);
    }


    if(currprefs.keyfile[0] != '\0') {
      gtk_label_set_text (GTK_LABEL (key_text_widget), currprefs.keyfile);
    }
    else {
      gtk_label_set_text (GTK_LABEL (key_text_widget), changed_prefs.keyfile);
    }
}

#ifdef JIT
static void set_comp_state (void)
{
  gint cache;
#ifdef NATMEM_OFFSET
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compbyte_widget[currprefs.comptrustbyte]), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compword_widget[currprefs.comptrustword]), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (complong_widget[currprefs.comptrustlong]), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compaddr_widget[currprefs.comptrustnaddr]), 1);
#endif
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compnf_widget[currprefs.compnf]), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (comp_hardflush_widget[currprefs.comp_hardflush]), 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (comp_constjump_widget[currprefs.comp_constjump]), 1);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compfpu_widget[currprefs.compfpu]), 1);

    /* round it */
		DEBUG_LOG("currprefs.cachesize: %lx\n", currprefs.cachesize);
		DEBUG_LOG("old cachesize_adj->value: %d\n", (int) cachesize_adj->value);
    cache=currprefs.cachesize / 1024;
    gtk_adjustment_set_value(cachesize_adj, cache);
		DEBUG_LOG("new cachesize_adj->value: %d\n", (int) cachesize_adj->value);
}
#endif


/*
 * Temporary hacks for joystick widgets
 */

/*
 * widget 0 = none
 *        1 = joy 0
 *        2 = joy 1
 *        3 = mouse
 *        4 = numpad
 *        5 = cursor
 *        6 = other
 */
static int map_jsem_to_widget (int jsem)
{
    int widget = 0;

    if (jsem >= JSEM_END)
	widget = 0;
    else if (jsem >= JSEM_MICE)
	widget = 3;
    else if (jsem == JSEM_JOYS || jsem == JSEM_JOYS + 1 )
	widget = jsem - JSEM_JOYS + 1;
    else if (jsem >= JSEM_KBDLAYOUT)
	widget = jsem - JSEM_KBDLAYOUT + 4;

    return widget;
}

static int map_widget_to_jsem (int widget)
{
   int jsem;

   switch (widget) {
	default:
	case 0: jsem = JSEM_NONE;          break;
	case 1: jsem = JSEM_JOYS;          break;
	case 2: jsem = JSEM_JOYS + 1;      break;
	case 3: jsem = JSEM_MICE;          break;
	case 4: jsem = JSEM_KBDLAYOUT;     break;
	case 5: jsem = JSEM_KBDLAYOUT + 1; break;
	case 6: jsem = JSEM_KBDLAYOUT + 2; break;
	case 7: jsem = JSEM_KBDLAYOUT + 3; break;
	case 8: jsem = JSEM_KBDLAYOUT + 4; break;
   }

   return jsem;
}

static void set_joy_state (void)
{
    int j0t = map_jsem_to_widget (changed_prefs.jport0);
    int j1t = map_jsem_to_widget (changed_prefs.jport1);

    int joy_count = inputdevice_get_device_total (IDTYPE_JOYSTICK);
    int i;

    if (j0t != 0 && j0t == j1t) {
	/* Can't happen */
	j0t++;
	j0t %= 7;
    }

    for (i = 0; i < (JOY_WIDGET_COUNT-1); i++) {
	if (i == 1 && joy_count == 0) continue;
	if (i == 2 && joy_count <= 1) continue;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (joy_widget[0][i]), j0t == i);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (joy_widget[1][i]), j1t == i);
	gtk_widget_set_sensitive (joy_widget[0][i], i == 0 || j1t != i);
	gtk_widget_set_sensitive (joy_widget[1][i], i == 0 || j0t != i);
    }
}

#ifdef FILESYS
static void set_hd_state (void)
{
    char  texts[HDLIST_MAX_COLS][256];
    char *tptrs[HDLIST_MAX_COLS];
    int nr = nr_units (currprefs.mountinfo);
    int i;

    DEBUG_LOG ("set_hd_state\n");
    for (i=0; i<HDLIST_MAX_COLS; i++)
	tptrs[i] = texts[i];

    gtk_clist_freeze (GTK_CLIST (hdlist_widget));
    gtk_clist_clear (GTK_CLIST (hdlist_widget));

    for (i = 0; i < nr; i++) {
	int     secspertrack, surfaces, reserved, blocksize, bootpri;
	uae_u64 size;
	int     cylinders, readonly, flags;
	char   *devname, *volname, *rootdir, *filesysdir;
	const char *failure;

	/* We always use currprefs.mountinfo for the GUI.  The filesystem
	   code makes a private copy which is updated every reset.  */
	failure = get_filesys_unit (currprefs.mountinfo, i,
				    &devname, &volname, &rootdir, &readonly,
				    &secspertrack, &surfaces, &reserved,
				    &cylinders, &size, &blocksize, &bootpri,
				    &filesysdir, &flags);

	if (is_hardfile (currprefs.mountinfo, i)) {
	    if (secspertrack == 0)
	        strcpy (texts[HDLIST_DEVICE], "N/A" );
	    else
	        strncpy (texts[HDLIST_DEVICE], devname, 255);
	    sprintf (texts[HDLIST_VOLUME],  "N/A" );
	    sprintf (texts[HDLIST_HEADS],   "%u", surfaces);
	    sprintf (texts[HDLIST_CYLS],    "%u", cylinders);
	    sprintf (texts[HDLIST_SECS],    "%u", secspertrack);
	    sprintf (texts[HDLIST_RSRVD],   "%u", reserved);
	    sprintf (texts[HDLIST_SIZE],    "%u", size);
	    sprintf (texts[HDLIST_BLKSIZE], "%u", blocksize);
	} else {
	    strncpy (texts[HDLIST_DEVICE], devname, 255);
	    strncpy (texts[HDLIST_VOLUME], volname, 255);
	    strcpy (texts[HDLIST_HEADS],   "N/A");
	    strcpy (texts[HDLIST_CYLS],    "N/A");
	    strcpy (texts[HDLIST_SECS],    "N/A");
	    strcpy (texts[HDLIST_RSRVD],   "N/A");
	    strcpy (texts[HDLIST_SIZE],    "N/A");
	    strcpy (texts[HDLIST_BLKSIZE], "N/A");
	}
	strcpy  (texts[HDLIST_PATH],     rootdir);
	strcpy  (texts[HDLIST_READONLY], readonly ? "Y" : "N");
	sprintf (texts[HDLIST_BOOTPRI], "%d", bootpri);
	gtk_clist_append (GTK_CLIST (hdlist_widget), tptrs);
    }
    gtk_clist_thaw (GTK_CLIST (hdlist_widget));
    gtk_widget_set_sensitive (hdchange_button, FALSE);
    gtk_widget_set_sensitive (hddel_button, FALSE);

    DEBUG_LOG ("set_hd_state done\n");
}
#endif

static void set_floppy_state( void )
{
    floppyfileentry_set_filename (FLOPPYFILEENTRY (floppy_widget[0]), currprefs.df[0]);
    floppyfileentry_set_filename (FLOPPYFILEENTRY (floppy_widget[1]), currprefs.df[1]);
    floppyfileentry_set_filename (FLOPPYFILEENTRY (floppy_widget[2]), currprefs.df[2]);
    floppyfileentry_set_filename (FLOPPYFILEENTRY (floppy_widget[3]), currprefs.df[3]);
}

static void update_state (void) {

  set_cpu_state();
  g_signal_emit_by_name(ctpanel, "read-prefs",NULL);
  set_joy_state ();
  set_sound_state ();
#ifdef JIT
  set_comp_state ();
#endif
  set_mem_state ();
  set_floppy_state ();
#ifdef FILESYS
  set_hd_state ();
#endif
  if(jdisp_panel) {
    g_signal_emit_by_name(jdisp_panel,"read-prefs",NULL);
  }
  if(jint_panel) {
    g_signal_emit_by_name(jint_panel, "read-prefs",NULL);
  }

  gtk_label_set_text (GTK_LABEL (config_path_widget),        optionsfile);
  gtk_entry_set_text (GTK_ENTRY (config_description_widget), currprefs.description);
}

static void update_buttons (int state)
{
    if (gui_window) {
	int running = state == UAE_STATE_RUNNING ? 1 : 0;
	int paused  = state == UAE_STATE_PAUSED  ? 1 : 0;

	gtk_widget_set_sensitive (start_uae_widget, !running && !paused);
	gtk_widget_set_sensitive (stop_uae_widget,  running || paused);
	gtk_widget_set_sensitive (pause_uae_widget, running || paused);
//	gtk_widget_set_sensitive (debug_uae_widget, running);
	gtk_widget_set_sensitive (reset_uae_widget, running);

	/* config stuff */
	gtk_widget_set_sensitive (config_load_widget,      !running && !paused);
	gtk_widget_set_sensitive (config_load_from_widget, !running && !paused);

        gtk_widget_set_sensitive (hdpanel,     !running && !paused);
        gtk_widget_set_sensitive (memorypanel, !running && !paused);
        gtk_widget_set_sensitive (rom_change_widget, !running && !paused);
        gtk_widget_set_sensitive (key_change_widget, !running && !paused);

	if(!running && !paused) {
	  g_signal_emit_by_name(jdisp_panel,"unlock-it",NULL);
	}
	else {
	  g_signal_emit_by_name(jdisp_panel,"lock-it",NULL);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pause_uae_widget), paused);
    }
}


#define MY_IDLE_PERIOD        250
#define LEDS_CALLBACK_PERIOD  1000
/*
 * my_idle()
 *
 * This function is added as a callback to the GTK+ mainloop
 * and is run every 250ms. It handles messages sent from UAE.
 */
static int my_idle (void) {

  if (quit_gui) {
    gtk_main_quit ();
    return 0;
  }

    while (comm_pipe_has_data (&to_gui_pipe)) {
	int cmd = read_comm_pipe_int_blocking (&to_gui_pipe);
	int n;
	DEBUG_LOG ("GUI got command:%d\n", cmd);
	switch (cmd) {
	 case GUICMD_STATE_CHANGE: {
			DEBUG_LOG ("GUICMD_STATE_CHANGE: read_comm_pipe_int_blocking ..\n");
	     int state = read_comm_pipe_int_blocking (&to_gui_pipe);
			DEBUG_LOG ("GUICMD_STATE_CHANGE: update_buttons ..\n");
	     update_buttons (state);
			DEBUG_LOG ("GUICMD_STATE_CHANGE: done (break)\n");
	     break;
	 }
	 case GUICMD_SHOW:
	 case GUICMD_FLOPPYDLG:
//	  DebOut("case GUICMD_SHOW\n");
	    if (!gui_window) {
		create_guidlg ();
		update_state ();
		update_buttons (uae_get_state()); //FIXME
	    }
	    if (cmd == GUICMD_SHOW) {

DEBUG_LOG ("GUICMD_SHOW entered\n");
		gtk_widget_show (gui_window);
    close_splash();
DEBUG_LOG ("GUICMD_SHOW 2\n");
#	    if GTK_MAJOR_VERSION >= 2
		gtk_window_present (GTK_WINDOW (gui_window));
	        gui_active = 1;
#	    endif
#if defined GTKMUI
		gui_active = 1;
#endif
DEBUG_LOG ("GUICMD_SHOW 3\n");
	    } else {
		n = read_comm_pipe_int_blocking (&to_gui_pipe);
		floppyfileentry_do_dialog (FLOPPYFILEENTRY (floppy_widget[n]));
	    }
	    //DebOut("case GUICMD_SHOW end\n");
	    break;
	 case GUICMD_DISKCHANGE:
	    n = read_comm_pipe_int_blocking (&to_gui_pipe);
	    if (floppy_widget[n])
		floppyfileentry_set_filename (FLOPPYFILEENTRY (floppy_widget[n]),
			       uae_get_state()!=UAE_STATE_RUNNING ? changed_prefs.df[n] : currprefs.df[n]);
	    break;
	 case GUICMD_UPDATE:
	    update_state ();
	    uae_sem_post (&gui_update_sem);
	    gui_active = 1;
	    DEBUG_LOG ("GUICMD_UPDATE done\n");
	    break;
	 case GUICMD_MSGBOX:
	    handle_message_box_request(&to_gui_pipe);
	    break;
#if 0
	 case GUICMD_SPLASH:
	    open_splash_pipe(&to_gui_pipe);
	    break;
#endif

#if 0
	 case GUICMD_FLOPPYDLG:
	    n = read_comm_pipe_int_blocking (&to_gui_pipe);
	    floppyfileentry_do_dialog (FLOPPYFILEENTRY (floppy_widget[n]));
	    break;
#endif
	}
    }
    return 1;
}

static int leds_callback (void)
{
    unsigned int leds = gui_ledstate;
    unsigned int i;

    if (!quit_gui) {
	for (i = 0; i < 5; i++) {
	    GtkWidget *widget = i ? floppy_widget[i-1] : power_led;
	    unsigned int mask = 1 << i;
	    unsigned int on = leds & mask;

	    if (!widget)
		continue;

	   if (on == (prevledstate & mask))
		continue;

	    if (i > 0)
		floppyfileentry_set_led (FLOPPYFILEENTRY (widget), on);
	    else {
#if !defined GTKMUI
		static GdkColor red   = {0, 0xffff, 0x0000, 0x0000};
		static GdkColor black = {0, 0x0000, 0x0000, 0x0000};
#else
		/* sorry, GTK-MUI uses some hacks here */
		static GdkColor red   = {0, 0xffff, 0x0000, 0x0000, 0, 0};
		static GdkColor black = {0, 0x0000, 0x0000, 0x0000, 0, 0};
#endif

		led_set_color (LED (widget), on ? red : black);
	    }
	}
	prevledstate = leds;
    }
    return 1;
}

int find_current_toggle (GtkWidget **widgets, int count) {
  int i;
  for (i = 0; i < count; i++) {
    if(!*widgets) {
      /* this must not happen! */
      kprintf("ERROR: find_current_toggle hit NULL pointer at element %d!\n",i);
      //exit(1);
      return -1;
    }
    if (GTK_TOGGLE_BUTTON (*widgets)->active) {
      return i;
    }
    *widgets++;
  }
  DEBUG_LOG ("GTKUI: Can't happen!\n");
  return -1;
}

static void joy_changed (void)
{
    if (!gui_active) 
	return;

    changed_prefs.jport0 = map_widget_to_jsem (find_current_toggle (joy_widget[0], 7));
    changed_prefs.jport1 = map_widget_to_jsem (find_current_toggle (joy_widget[1], 7));

    if( changed_prefs.jport0 != currprefs.jport0 || changed_prefs.jport1 != currprefs.jport1 )
	inputdevice_config_change();

    set_joy_state ();
}

static void bogosize_changed (void)
{
    int t = find_current_toggle (bogosize_widget, 4);
    switch (t) {
	case 0: changed_prefs.bogomem_size = 0;        break;
	case 1: changed_prefs.bogomem_size = 0x080000; break;
	case 2: changed_prefs.bogomem_size = 0x100000; break;
	case 3: changed_prefs.bogomem_size = 0x1C0000; break;
    }
}

static void fastsize_changed (void)
{
    int t = find_current_toggle (fastsize_widget, 5);
    changed_prefs.fastmem_size = (0x80000 << t) & ~0x80000;
}

static void z3size_changed (void) {

    int t = find_current_toggle (z3size_widget, 15);
    if(t<10) {
      changed_prefs.z3fastmem_size = (0x80000 << t) & ~0x80000;
      return;
    }

    switch(t) {
      case 10:
	changed_prefs.z3fastmem_size = 384 * 0x100000;
	break;
      case 11:
	changed_prefs.z3fastmem_size = 512 * 0x100000;
	break;
      case 12:
	changed_prefs.z3fastmem_size = 768 * 0x100000;
	break;
      case 13:
	/* 1 GB */
	changed_prefs.z3fastmem_size = 1024 * 0x100000;
	break;
      case 14:
	/* 1.5 GB */
	changed_prefs.z3fastmem_size = 0x60000000;
	break;
      default:
	write_log("ERROR: z3size_changed could not find, which button you clicked !?\n");
    }
}

#ifdef PICASSO96
static void p96size_changed (void)
{
    int t = find_current_toggle (p96size_widget, 10);
    changed_prefs.gfxmem_size = (0x80000 << t) & ~0x80000;
}
#endif

static void sound_changed (void)
{
    changed_prefs.produce_sound = find_current_toggle (sound_widget, 4);
    changed_prefs.sound_stereo = find_current_toggle (sound_ch_widget, 3);
    changed_prefs.sound_mixed_stereo = 0;
    if (changed_prefs.sound_stereo == 2)
	changed_prefs.sound_mixed_stereo = changed_prefs.sound_stereo = 1;
    changed_prefs.sound_bits = (find_current_toggle (sound_bits_widget, 2) + 1) * 8;
}

#ifdef JIT
static void comp_changed (void)
{
		DEBUG_LOG("old changed_prefs.cachesize: %lx\n", changed_prefs.cachesize);
		DEBUG_LOG("old cachesize_adj->value: %d\n", (int) cachesize_adj->value);
		/* WARNING: race condition!
		 * if jit is already running and we resize the cache here, we *will* get
		 * trashed memory ! (or not?)
		 */
    changed_prefs.cachesize=cachesize_adj->value * 1024;
		DEBUG_LOG("new changed_prefs.cachesize: %lx\n", changed_prefs.cachesize);
#ifdef NATMEM_OFFSET
    changed_prefs.comptrustbyte = find_current_toggle (compbyte_widget, 4);
    changed_prefs.comptrustword = find_current_toggle (compword_widget, 4);
    changed_prefs.comptrustlong = find_current_toggle (complong_widget, 4);
    changed_prefs.comptrustnaddr = find_current_toggle (compaddr_widget, 4);
#endif
    changed_prefs.compnf = find_current_toggle (compnf_widget, 2);
    changed_prefs.comp_hardflush = find_current_toggle (comp_hardflush_widget, 2);
    changed_prefs.comp_constjump = find_current_toggle (comp_constjump_widget, 2);
    changed_prefs.compfpu= find_current_toggle (compfpu_widget, 2);
}
#endif

void on_start_clicked (void) {

    DEBUG_LOG ("Start button clicked.\n");

    write_comm_pipe_int (&from_gui_pipe, UAECMD_START, 1);
}

static void on_stop_clicked (void)
{
    DEBUG_LOG ("Stop button clicked.\n");

    write_comm_pipe_int (&from_gui_pipe, UAECMD_STOP, 1);
}

static void on_reset_clicked (void)
{
    DEBUG_LOG ("Reset button clicked.\n");

    if (!quit_gui)
	write_comm_pipe_int (&from_gui_pipe, UAECMD_RESET, 1);
}

#ifdef DEBUGGER
static void on_debug_clicked (void)
{
    DEBUG_LOG ("Called\n");

    if (!quit_gui)
	write_comm_pipe_int (&from_gui_pipe, UAECMD_DEBUG, 1);
}
#endif

static void on_pause_clicked (GtkWidget *widget, gpointer data)
{
    DEBUG_LOG ("Called with %d\n", GTK_TOGGLE_BUTTON (widget)->active == TRUE );

    if (!quit_gui) {
	write_comm_pipe_int (&from_gui_pipe, GTK_TOGGLE_BUTTON (widget)->active ? UAECMD_PAUSE : UAECMD_RESUME, 1);
    }
}


static char *gui_romname, *gui_keyname, *gui_extname;

static void disc_changed (FloppyFileEntry *ffe, gpointer p)
{
    int num = GPOINTER_TO_INT(p);
    char *s = floppyfileentry_get_filename(ffe);
    int len;

    if (quit_gui)
	return;

    if(s == NULL || strlen(s) == 0) {
	write_comm_pipe_int (&from_gui_pipe, UAECMD_EJECTDISK, 0);
	write_comm_pipe_int (&from_gui_pipe, GPOINTER_TO_INT(p), 1);
    } else {
	/* Get the pathname, not including the filename
	 * Set floppydlg_path to this, so that when the requester
	 * dialog pops up again, we don't have to navigate to the same place. */
#if !defined GTKMUI
	len = strrchr(s, '/') - s;
#else
      if(strrchr(s, '/')) {
       	len = strrchr(s, '/') - s;
      }
      else if(strrchr(s, ':')) {
	len = strrchr(s, ':') - s;
      }
      else {
	len = 0;
      }
#endif
	if (len > 254) len = 254;
	strncpy(floppydlg_path, s, len);
	floppydlg_path[len] = '\0';

	uae_sem_wait (&gui_sem);
	if (new_disk_string[num] != 0)
	    free (new_disk_string[num]);
	new_disk_string[num] = strdup (s);
	uae_sem_post (&gui_sem);
	write_comm_pipe_int (&from_gui_pipe, UAECMD_INSERTDISK, 0);
	write_comm_pipe_int (&from_gui_pipe, num, 1);
    }
}

static char fsbuffer[100];

static GtkWidget *make_file_selector (const char *title,
				      void (*insertfunc)(GtkObject *),
				      void (*closefunc)(gpointer))
{
    GtkWidget *p = gtk_file_selection_new (title);
    gtk_signal_connect (GTK_OBJECT (p), "destroy", (GtkSignalFunc) closefunc, p);

    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (p)->ok_button),
			       "clicked", (GtkSignalFunc) insertfunc,
			       GTK_OBJECT (p));
#if !defined GTKMUI
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (p)->cancel_button),
			       "clicked", (GtkSignalFunc) gtk_widget_destroy,
			       GTK_OBJECT (p));

#if 0
    gtk_window_set_title (GTK_WINDOW (p), title);
#endif
    gtk_widget_show (p);

#else
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (p)->cancel_button),
				"clicked", (GtkSignalFunc) closefunc,
				GTK_OBJECT (p));
#endif
    return p;
}

static void filesel_set_path (GtkWidget *p, const char *path)
{
    size_t len = strlen (path);
    if (len > 0 && ! access (path, R_OK)) {
	char *tmp = xmalloc (len + 2);
	strcpy (tmp, path);
	strcat (tmp, "/");
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (p),
					 tmp);
    }
}

static GtkWidget *rom_selector;
static GtkWidget *ext_selector;

static void did_close_rom (gpointer gdata)
{
    gtk_widget_set_sensitive (rom_change_widget, 1);
}

static void did_close_ext (gpointer gdata)
{
    gtk_widget_set_sensitive (ext_change_widget, 1);
}


static void did_rom_select (GtkObject *o)
{
    const char *s = gtk_file_selection_get_filename (GTK_FILE_SELECTION (rom_selector));

    if (quit_gui)
	return;

    gtk_widget_set_sensitive (rom_change_widget, 1);

    uae_sem_wait (&gui_sem);
    gui_romname = strdup (s);
    uae_sem_post (&gui_sem);
    gtk_label_set_text (GTK_LABEL (rom_text_widget), gui_romname);
    write_comm_pipe_int (&from_gui_pipe, UAECMD_SELECT_ROM, 0);
    gtk_widget_destroy (rom_selector);
}


static void did_ext_select (GtkObject *o)
{
    const char *s = gtk_file_selection_get_filename (GTK_FILE_SELECTION (ext_selector));
#ifdef __AROS__
		BPTR lock;
		struct FileInfoBlock *fib;
		BOOL file=FALSE;
#else
		BOOL file=TRUE;
#endif

    if (quit_gui)
	return;

    gtk_widget_set_sensitive (ext_change_widget, 1);

#ifdef __AROS__
		/* 
		 * If you don't select a rom file, only a path/directory will be returned.
		 * There is no way to select "nothing". So check, if it is a valid file.
		 */
		if((lock=Lock((CONST_STRPTR) s, ACCESS_READ))) {

			if((fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL))) {
				if(Examine(lock, fib)) {
					if(fib->fib_EntryType < 0 ) {
						file=TRUE;
					}
				}
				FreeDosObject(DOS_FIB, fib);
			};
			UnLock(lock);
		}
#endif


    uae_sem_wait (&gui_sem);
		if(file) {
			gui_extname = strdup (s);
		}
		else {
			gui_extname = strdup ("");
		}
    uae_sem_post (&gui_sem);
    gtk_label_set_text (GTK_LABEL (ext_text_widget), gui_extname);
    write_comm_pipe_int (&from_gui_pipe, UAECMD_SELECT_ROM, 0);
    gtk_widget_destroy (ext_selector);
}

static void did_romchange (GtkWidget *w, gpointer data)
{
    gtk_widget_set_sensitive (rom_change_widget, 0);

    DEBUG_LOG("rom_selector = %lx\n",rom_selector);
    rom_selector = make_file_selector ("Select a ROM file",
				       did_rom_select, did_close_rom);
#if defined GTKMUI
        gtk_widget_show(rom_selector);
#endif
    
    DEBUG_LOG("rom_selector = %lx\n",rom_selector);
    filesel_set_path (rom_selector, prefs_get_attr ("rom_path"));
}

static void did_extchange (GtkWidget *w, gpointer data)
{
    gtk_widget_set_sensitive (ext_change_widget, 0);

    DEBUG_LOG("ext_selector = %lx\n", ext_selector);
    ext_selector = make_file_selector ("Select a extended ROM file",
				       did_ext_select, did_close_ext);
#if defined GTKMUI
        gtk_widget_show(ext_selector);
#endif
    
    DEBUG_LOG("ext_selector = %lx\n",ext_selector);
    filesel_set_path (ext_selector, prefs_get_attr ("rom_path"));
}



static GtkWidget *key_selector;

static void did_close_key (gpointer gdata)
{
    gtk_widget_set_sensitive (key_change_widget, 1);
}

static void did_key_select (GtkObject *o)
{
    const char *s = gtk_file_selection_get_filename (GTK_FILE_SELECTION (key_selector));
#ifdef __AROS__
		BPTR lock;
		struct FileInfoBlock *fib;
		BOOL file=FALSE;
#else
		BOOL file=TRUE;
#endif

    if (quit_gui)
	return;

    gtk_widget_set_sensitive (key_change_widget, 1);

#ifdef __AROS__
		/* 
		 * If you don't select a rom file, only a path/directory will be returned.
		 * There is no way to select "nothing". So check, if it is a valid file.
		 */
		if((lock=Lock((CONST_STRPTR) s, ACCESS_READ))) {

			if((fib=(struct FileInfoBlock *) AllocDosObject(DOS_FIB, NULL))) {
				if(Examine(lock, fib)) {
					if(fib->fib_EntryType < 0 ) {
						file=TRUE;
					}
				}
				FreeDosObject(DOS_FIB, fib);
			};
			UnLock(lock);
		}
#endif

    uae_sem_wait (&gui_sem);
		if(file) {
			gui_keyname = strdup (s);
		}
		else {
			gui_keyname = strdup ("");
		}
    uae_sem_post (&gui_sem);
		if(file) {
			gtk_label_set_text (GTK_LABEL (key_text_widget), gui_keyname);
		}
		else {
			gtk_label_set_text (GTK_LABEL (key_text_widget), "");
		}
    write_comm_pipe_int (&from_gui_pipe, UAECMD_SELECT_KEY, 0);
    gtk_widget_destroy (key_selector);
}

static void did_keychange (GtkWidget *w, gpointer data)
{
    gtk_widget_set_sensitive (key_change_widget, 0);

    key_selector = make_file_selector ("Select a Kickstart key file",
				       did_key_select, did_close_key);
    gtk_widget_show(key_selector);

    filesel_set_path (key_selector, prefs_get_attr ("rom_path"));
}

static void add_empty_vbox (GtkWidget *tobox)
{
    GtkWidget *thing = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (thing);
    gtk_box_pack_start (GTK_BOX (tobox), thing, TRUE, TRUE, 0);
}

static void add_empty_hbox (GtkWidget *tobox)
{
    GtkWidget *thing = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (thing);
    gtk_box_pack_start (GTK_BOX (tobox), thing, TRUE, TRUE, 0);
}

static void add_centered_to_vbox (GtkWidget *vbox, GtkWidget *w)
{
    GtkWidget *hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
}

static void add_centered_to_hbox (GtkWidget *hbox, GtkWidget *w)
{
    GtkWidget *vbox = gtk_vbox_new (TRUE, 0);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, TRUE, 0);
}



static GtkWidget *make_labelled_widget (const char *str, GtkWidget *thing)
{
    GtkWidget *label = gtk_label_new (str);
    GtkWidget *hbox2 = gtk_hbox_new (FALSE, 4);

    gtk_widget_show (label);
    gtk_widget_show (thing);

    gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), thing, FALSE, TRUE, 0);

    return hbox2;
}

static GtkWidget *add_labelled_widget_centered (const char *str, GtkWidget *thing, GtkWidget *vbox)
{
    GtkWidget *w = make_labelled_widget (str, thing);
    gtk_widget_show (w);
    add_centered_to_vbox (vbox, w);
    return w;
}


GtkWidget *make_radio_group_box_param (const char *title, const char **labels,
					GtkWidget **saveptr, int horiz,
					void (*sigfunc) (void), 
					GtkWidget *parameter)
{
    GtkWidget *frame, *newbox;

		DEBUG_LOG("title: %s\n", title);

    frame = gtk_frame_new (title);
    //kprintf("==> %s\n",title);
    newbox = (horiz ? gtk_hbox_new : gtk_vbox_new) (FALSE, 4);
    gtk_widget_show (newbox);
    gtk_container_set_border_width (GTK_CONTAINER (newbox), 4);
    gtk_container_add (GTK_CONTAINER (frame), newbox);
    make_radio_group_param (labels, newbox, saveptr, horiz, !horiz, sigfunc, -1, NULL,parameter);
    return frame;
}

GtkWidget *make_radio_group_box (const char *title, const char **labels,
					GtkWidget **saveptr, int horiz,
					void (*sigfunc) (void)) {
  return make_radio_group_box_param(title,labels,saveptr,horiz,sigfunc,NULL);
}


static GtkWidget *make_file_container (const char *title, GtkWidget *vbox)
{
    GtkWidget *thing = gtk_frame_new (title);
    GtkWidget *buttonbox = gtk_hbox_new (FALSE, 4);

    gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 4);
    gtk_container_add (GTK_CONTAINER (thing), buttonbox);
    gtk_box_pack_start (GTK_BOX (vbox), thing, FALSE, TRUE, 0);
    gtk_widget_show (buttonbox);
    gtk_widget_show (thing);

    return buttonbox;
}

static GtkWidget *make_file_widget (GtkWidget *buttonbox)
{
    GtkWidget *thing, *subthing;
    GtkWidget *subframe = gtk_frame_new (NULL);

    gtk_frame_set_shadow_type (GTK_FRAME (subframe), GTK_SHADOW_ETCHED_OUT);
    gtk_box_pack_start (GTK_BOX (buttonbox), subframe, TRUE, TRUE, 0);
    gtk_widget_show (subframe);
    subthing = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (subthing);
    gtk_container_add (GTK_CONTAINER (subframe), subthing);
    thing = gtk_label_new ("");
    gtk_widget_show (thing);
    gtk_box_pack_start (GTK_BOX (subthing), thing, TRUE, TRUE, 0);

    return thing;
}

static void make_floppy_disks (GtkWidget *vbox)
{
    char buf[5];
    int i;

    add_empty_vbox (vbox);

    for (i = 0; i < 4; i++) {
	sprintf (buf, "DF%d:", i);

	floppy_widget[i] = floppyfileentry_new ();
        if (currprefs.dfxtype[i] == -1)
	    gtk_widget_set_sensitive(floppy_widget[i], 0);
	floppyfileentry_set_drivename (FLOPPYFILEENTRY (floppy_widget[i]), buf);
	floppyfileentry_set_label (FLOPPYFILEENTRY (floppy_widget[i]), buf);
        floppyfileentry_set_currentdir (FLOPPYFILEENTRY (floppy_widget[i]), prefs_get_attr ("floppy_path"));
	gtk_box_pack_start (GTK_BOX (vbox), floppy_widget[i], FALSE, TRUE, 0);
	gtk_widget_show (floppy_widget[i]);
	gtk_signal_connect (GTK_OBJECT (floppy_widget[i]), "disc-changed", (GtkSignalFunc) disc_changed, GINT_TO_POINTER (i));
    }
    floppy_widget[4]=NULL;

    add_empty_vbox (vbox);
}

/*
 * CPU configuration page
 */
static void on_cputype_changed (void)
{
    int i;

    DEBUG_LOG ("called\n");

    //changed_prefs.cpu_level       = cputypepanel_get_cpulevel (CPUTYPEPANEL (ctpanel));
    changed_prefs.cpu_level       = CPUTYPEPANEL (ctpanel)->cpulevel;
    changed_prefs.address_space_24= CPUTYPEPANEL (ctpanel)->addr24bit;
    changed_prefs.cpu_compatible  = CPUTYPEPANEL (ctpanel)->compatible;
    changed_prefs.cpu_cycle_exact = CPUTYPEPANEL (ctpanel)->cycleexact;

    set_mem32_widgets_state ();

    DEBUG_LOG ("cpu_level=%d address_space24=%d cpu_compatible=%d cpu_cycle_exact=%d\n",
    changed_prefs.cpu_level, changed_prefs.address_space_24,
    changed_prefs.cpu_compatible, changed_prefs.cpu_cycle_exact);
}

static void on_addr24bit_changed (void)
{
    int i;

    DEBUG_LOG ("called\n");
    //D(bug("called\n"));

    changed_prefs.address_space_24 = (cputypepanel_get_addr24bit (CPUTYPEPANEL (ctpanel)) != 0);

    set_mem32_widgets_state ();

    DEBUG_LOG ("address_space_24=%d\n", changed_prefs.address_space_24);
    //D(bug("address_space_24=%d\n", changed_prefs.address_space_24));
}

static void on_cpuspeed_changed (void)
{
    DEBUG_LOG ("called\n");

    changed_prefs.m68k_speed = CPUSPEEDPANEL (cspanel)->cpuspeed;

    DEBUG_LOG ("m68k_speed=%d\n", changed_prefs.m68k_speed);
}

static void on_cpuidle_changed (void)
{
   DEBUG_LOG ("called\n");

   changed_prefs.cpu_idle       = CPUSPEEDPANEL (cspanel)->cpuidle;

   DEBUG_LOG ("cpu_idle=%d\n", changed_prefs.cpu_idle);
}

static void make_cpu_widgets (GtkWidget *vbox)
{
    GtkWidget *table;

    table = make_xtable (5, 7);
    add_table_padding (table, 0, 0);
    add_table_padding (table, 4, 4);
    add_table_padding (table, 1, 2);
    gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

    ctpanel = cputypepanel_new();

    if(!ctpanel) {
      kprintf("ERROR: make_cpu_widgets: cputypepanel_new failed !?\n");
      printf("ERROR: make_cpu_widgets: cputypepanel_new failed !?\n");
    }
    gtk_table_attach (GTK_TABLE (table), ctpanel, 1, 4, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_widget_show (ctpanel);

    cspanel = cpuspeedpanel_new();
    gtk_table_attach (GTK_TABLE (table), cspanel, 1, 4, 3, 4,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_widget_show (cspanel);


    gtk_signal_connect (GTK_OBJECT (ctpanel), "cputype-changed",
                        GTK_SIGNAL_FUNC (on_cputype_changed),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (ctpanel), "addr24bit-changed",
                        GTK_SIGNAL_FUNC (on_addr24bit_changed),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (cspanel), "cpuspeed-changed",
                        GTK_SIGNAL_FUNC (on_cpuspeed_changed),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (cspanel), "cpuidle-changed",
                        GTK_SIGNAL_FUNC (on_cpuidle_changed),
                        NULL);
}

static void on_framerate_changed (void)
{
    changed_prefs.gfx_framerate = CHIPSETSPEEDPANEL (chipsetspeed_panel)->framerate;
    DEBUG_LOG("gfx_framerate = %d\n", changed_prefs.gfx_framerate);
}

static void on_collision_level_changed (void)
{
    changed_prefs.collision_level = CHIPSETSPEEDPANEL (chipsetspeed_panel)->collision_level;
    DEBUG_LOG("collision_level = %d\n", changed_prefs.collision_level);
}

static void on_immediate_blits_changed (void)
{
    changed_prefs.immediate_blits = CHIPSETSPEEDPANEL (chipsetspeed_panel)->immediate_blits;
    DEBUG_LOG("immediate_blits = %d\n", changed_prefs.immediate_blits);
}

extern ULONG aos3_task;
void uae_Signal (uaecptr task, uae_u32 mask);

void show_uae_main_window(void);
void hide_uae_main_window(void);
void close_all_janus_windows(void);
void switch_off_coherence(void);

void switch_off_coherence(void) {

  if((changed_prefs.jcoherence == FALSE) | (!aos3_task)) {
    DEBUG_LOG("switch_off_coherence: nothing to do\n");
    return;
  }

  DEBUG_LOG("switch_off_coherence\n");
  changed_prefs.jcoherence=FALSE;

  close_all_janus_windows_wait();
  close_all_janus_screens_wait();
  show_uae_main_window();

  /* update gui */
  gtk_list_select_item (GTK_LIST (GTK_COMBO (JINTEGRATION (jint_panel)->combo_coherence)->list), FALSE);
}

static void on_coherent_changed (void) {

  if(changed_prefs.jcoherence == JINTEGRATION (jint_panel)->coherence) {
    DEBUG_LOG("jcoherence: nothing to do\n");
    return;
  }

  DEBUG_LOG("old coherence = %d\n", changed_prefs.jcoherence);
  changed_prefs.jcoherence = JINTEGRATION (jint_panel)->coherence;
  DEBUG_LOG("new coherence = %d\n", changed_prefs.jcoherence);
  if(aos3_task && !changed_prefs.jcoherence) {
    DEBUG_LOG("call close_all_janus_windows\n");
    close_all_janus_windows_wait();
    close_all_janus_screens_wait();
    show_uae_main_window();
  }
  else {
    hide_uae_main_window();
  }

  /* if changed to coherence, windows are opened automatically */
}

static void on_mouse_changed (void) {

  if(changed_prefs.jmouse == JINTEGRATION (jint_panel)->mouse) {
    DEBUG_LOG("jmouse: nothing to do\n");
    return;
  }

  DEBUG_LOG("old mouse = %d\n", changed_prefs.jmouse);
  changed_prefs.jmouse = JINTEGRATION (jint_panel)->mouse;
  DEBUG_LOG("new mouse = %d\n", changed_prefs.jmouse);
}

extern BOOL clipboard_aros_changed;
void copy_clipboard_to_amigaos(void);

static void on_clipboard_changed (void) {

  if(changed_prefs.jclipboard == JINTEGRATION (jint_panel)->clipboard) {
    DEBUG_LOG("jcliboard: nothing to do\n");
    return;
  }

  DEBUG_LOG("old clipboard = %d\n", changed_prefs.jclipboard);
  changed_prefs.jclipboard = JINTEGRATION (jint_panel)->clipboard;
  DEBUG_LOG("new clipboard = %d\n", changed_prefs.jclipboard);

  /* manually sync clipboards, AROS clipboard wins */
  if(changed_prefs.jclipboard) {
    clipboard_aros_changed=TRUE;
    copy_clipboard_to_amigaos();
  }
}

static void on_launch_changed (void) {

  if(changed_prefs.jlaunch == JINTEGRATION (jint_panel)->launch) {
    DEBUG_LOG("jcliboard: nothing to do\n");
    return;
  }

  changed_prefs.jlaunch=JINTEGRATION (jint_panel)->launch;

  if(changed_prefs.jlaunch) {
    aros_launch_start_thread();
    aros_cli_start_thread();
  }
  else {
    aros_launch_kill_thread();
    aros_cli_kill_thread();
  }
}

static void make_sound_widgets (GtkWidget *vbox)
{
    GtkWidget *frame, *newbox;
    int i;
    GtkWidget *hbox;

    add_empty_vbox (vbox);

    newbox = make_radio_group_box ("Mode", soundlabels1, sound_widget, 1, sound_changed);
    gtk_widget_set_sensitive (sound_widget[2], sound_available);
    gtk_widget_set_sensitive (sound_widget[3], sound_available);
    gtk_widget_show (newbox);
    add_centered_to_vbox (vbox, newbox);

    hbox = gtk_hbox_new (FALSE, 10);
    gtk_widget_show (hbox);
    add_centered_to_vbox (vbox, hbox);
    newbox = make_radio_group_box ("Channels", soundlabels3, sound_ch_widget, 1, sound_changed);
    gtk_widget_set_sensitive (newbox, sound_available);
    gtk_widget_show (newbox);
    gtk_box_pack_start (GTK_BOX (hbox), newbox, FALSE, TRUE, 0);
    newbox = make_radio_group_box ("Resolution", soundlabels2, sound_bits_widget, 1, sound_changed);
    gtk_widget_set_sensitive (newbox, sound_available);
    gtk_widget_show (newbox);
    gtk_box_pack_start (GTK_BOX (hbox), newbox, FALSE, TRUE, 0);

    add_empty_vbox (vbox);
}

/* TODO: check if this should be disabled..? */
void chipsize_changed (void) {

    int t = find_current_toggle (chipsize_widget, 5);
    changed_prefs.chipmem_size = 0x80000 << t;
    for (t = 0; t < 5; t++)
	gtk_widget_set_sensitive (fastsize_widget[t], changed_prefs.chipmem_size <= 0x200000);
    if (changed_prefs.chipmem_size > 0x200000) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fastsize_widget[0]), 1);
	changed_prefs.fastmem_size = 0;
    }
}

static void make_mem_widgets (GtkWidget *vbox)
{
    GtkWidget *hbox = gtk_hbox_new (FALSE, 10);
    GtkWidget *label, *frame;

    add_empty_vbox (vbox);

    {
	GtkWidget *buttonbox = make_file_container ("Main ROM file:", vbox);
	GtkWidget *thing = gtk_button_new_with_label ("Change");

	/* Current file display */
	rom_text_widget = make_file_widget (buttonbox);

	gtk_box_pack_start (GTK_BOX (buttonbox), thing, FALSE, TRUE, 0);
	gtk_widget_show (thing);
	rom_change_widget = thing;
	gtk_signal_connect (GTK_OBJECT (thing), "clicked", (GtkSignalFunc) did_romchange, 0);
    }

    {
	GtkWidget *buttonbox = make_file_container ("Extended ROM file:", vbox);
	GtkWidget *thing = gtk_button_new_with_label ("Change");

	/* Current file display */
	ext_text_widget = make_file_widget (buttonbox);

	gtk_box_pack_start (GTK_BOX (buttonbox), thing, FALSE, TRUE, 0);
	gtk_widget_show (thing);
	ext_change_widget = thing;
	gtk_signal_connect (GTK_OBJECT (thing), "clicked", (GtkSignalFunc) did_extchange, 0);
    }

    {
	GtkWidget *buttonbox = make_file_container ("ROM key file for Cloanto Amiga Forever:", vbox);
	GtkWidget *thing = gtk_button_new_with_label ("Change");

	/* Current file display */
	key_text_widget = make_file_widget (buttonbox);

	gtk_box_pack_start (GTK_BOX (buttonbox), thing, FALSE, TRUE, 0);
	gtk_widget_show (thing);
	key_change_widget = thing;
	gtk_signal_connect (GTK_OBJECT (thing), "clicked", (GtkSignalFunc) did_keychange, 0);
    }


    gtk_widget_show (hbox);
    add_centered_to_vbox (vbox, hbox);

    add_empty_vbox (vbox);

    frame = make_radio_group_box ("Chip Mem", chiplabels, chipsize_widget, 0, chipsize_changed);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);

    frame = make_radio_group_box ("Slow Mem", bogolabels, bogosize_widget, 0, bogosize_changed);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);

    frame = make_radio_group_box ("Fast Mem", fastlabels, fastsize_widget, 0, fastsize_changed);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);

    z3frame = make_radio_group_box_1 ("Z3 Mem", z3labels, z3size_widget, 0, z3size_changed, 5);
    gtk_widget_show (z3frame);
    gtk_box_pack_start (GTK_BOX (hbox), z3frame, FALSE, TRUE, 0);

#ifdef PICASSO96
    p96frame = make_radio_group_box_1 ("P96 RAM", p96labels, p96size_widget, 0, p96size_changed, 5);
    gtk_widget_show (p96frame);
    gtk_box_pack_start (GTK_BOX (hbox), p96frame, FALSE, TRUE, 0);
#endif

    memorypanel = hbox;
}

#ifdef JIT
static void make_comp_widgets (GtkWidget *vbox)
{
    GtkWidget *newbox;
    GtkWidget *container;
    GtkWidget *hbox;
    GtkWidget *hbox_top;
    GtkWidget *vbox_left;
		gint       cache_mb;

    static const char *complabels1[] = {
	"Direct", "Indirect", "Indirect for KS", "Direct after Picasso",
	NULL
    },*complabels2[] = {
	"Direct", "Indirect", "Indirect for KS", "Direct after Picasso",
	NULL
    },*complabels3[] = {
	"Direct", "Indirect", "Indirect for KS", "Direct after Picasso",
	NULL
    },*complabels3a[] = {
	"Direct", "Indirect", "Indirect for KS", "Direct after Picasso",
	NULL
    }, *complabels4[] = {
      "Always generate", "Only generate when needed",
	NULL
    }, *complabels5[] = {
      "Disable", "Enable",
	NULL
    }, *complabels6[] = {
      "Disable", "Enable",
	NULL
    }, *complabels7[] = {
      "Disable", "Enable",
	NULL
    }, *complabels8[] = {
      "Soft", "Hard",
	NULL
    }, *complabels9[] = {
      "Disable", "Enable",
	NULL
    };
    GtkWidget *thing;

    add_empty_vbox (vbox);

#ifdef NATMEM_OFFSET
    newbox = make_radio_group_box ("Byte access", complabels1, compbyte_widget, 1, comp_changed);
    gtk_widget_show (newbox);
    add_centered_to_vbox (vbox, newbox);
    newbox = make_radio_group_box ("Word access", complabels2, compword_widget, 1, comp_changed);
    gtk_widget_show (newbox);
    add_centered_to_vbox (vbox, newbox);
    newbox = make_radio_group_box ("Long access", complabels3, complong_widget, 1, comp_changed);
    gtk_widget_show (newbox);
    add_centered_to_vbox (vbox, newbox);
    newbox = make_radio_group_box ("Address lookup", complabels3a, compaddr_widget, 1, comp_changed);
    gtk_widget_show (newbox);
    add_centered_to_vbox (vbox, newbox);
#endif

    //newbox = make_radio_group_box ("Flags", complabels4, compnf_widget, 1, comp_changed);

    hbox_top=gtk_hbox_new(0,5);
    gtk_widget_show (hbox_top);
    vbox_left=gtk_vbox_new(0,5);
    gtk_widget_show (vbox_left);

    container=make_file_container("Flags", vbox_left);
    make_radio_group_param (complabels4, container, compnf_widget, 1, 0, comp_changed, -1, NULL,NULL);
    gtk_widget_show (container);

    container=make_file_container("Icache flushes", vbox_left);
    make_radio_group_param (complabels8, container, comp_hardflush_widget, 1, 0, comp_changed, -1, NULL, NULL);
    gtk_widget_show (container);

    container=make_file_container("Compile through uncond branch", vbox_left);
    make_radio_group_param (complabels9, container, comp_constjump_widget, 1, 0, comp_changed, -1, NULL, NULL);
    gtk_widget_show (container);

    container=make_file_container("FPU support", vbox_left);
    make_radio_group_param (complabels7, container, compfpu_widget, 1, 0, comp_changed, -1, NULL, NULL);
    gtk_widget_show (container);

    add_empty_vbox (vbox_left);
    gtk_widget_show (vbox_left);

    /* Translation Buffer */
    container=make_file_container("Cache Size (MB)", vbox_left);

    /* use actual value here*/
    cache_mb=currprefs.cachesize / 1024;
		DEBUG_LOG("currprefs.cachesize %lx => %d MB\n", currprefs.cachesize, cache_mb);
    cachesize_adj = GTK_ADJUSTMENT (gtk_adjustment_new ((gdouble) cache_mb, 0.0, 8.0, 1.0, 1.0, 1.0));
    gtk_adjustment_set_value(cachesize_adj, cache_mb);

    thing = gtk_hscale_new (cachesize_adj);
    gtk_range_set_update_policy (GTK_RANGE (thing), GTK_UPDATE_DELAYED);
    gtk_scale_set_digits (GTK_SCALE (thing), 0);
    gtk_scale_set_value_pos (GTK_SCALE (thing), GTK_POS_RIGHT);
    //gtk_widget_set_usize (thing, 180, -1); // Hack!
    gtk_widget_show (thing);
    gtk_box_pack_start (GTK_BOX (container), thing, TRUE, TRUE, 0);

    add_empty_hbox(hbox_top);
    gtk_box_pack_start (GTK_BOX (hbox_top), vbox_left, FALSE, TRUE, 0);
    add_empty_hbox(hbox_top);

    gtk_box_pack_start (GTK_BOX (vbox), hbox_top, FALSE, TRUE, 0);

    add_empty_vbox (vbox);

    /* Kludge - remember pointer to JIT page, so that we can easily disable it */
    jit_page = vbox;

		/* install hook for changes */

    gtk_signal_connect (GTK_OBJECT (cachesize_adj), "value_changed",
			GTK_SIGNAL_FUNC (comp_changed), NULL);
}
#endif

/*********************************************
 * Joystick
 *********************************************/
static void  on_catweasel_joystick_changed(GtkWidget *w, GtkWidget *cat) {

  currprefs.catweasel_joy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (catweasel_joystick_widget));
  changed_prefs.catweasel_joy = currprefs.catweasel_joy;
  inputdevice_config_change();
}

static GtkWidget *make_joy_frame(int i) {
  GtkWidget *frame;
  char buffer[20];
  int joy_count = inputdevice_get_device_total (IDTYPE_JOYSTICK);

  sprintf (buffer, "Port %d", i);
  frame = make_radio_group_box (buffer, joylabels, joy_widget[i], 0, joy_changed);
  gtk_widget_show (frame);

  if (joy_count < 2) {
      gtk_widget_set_sensitive (joy_widget[i][2], 0);
  }
  if (joy_count == 0) {
      gtk_widget_set_sensitive (joy_widget[i][1], 0);
  }

  return frame;
}

static void make_joy_widgets (GtkWidget *dvbox)
{
    GtkWidget *joy1_frame;
    GtkWidget *joy2_frame;
#ifdef CATWEASEL
    GtkWidget *cat_frame;
#endif

    joy1_frame                =make_joy_frame(0);
    joy2_frame                =make_joy_frame(1);
#ifdef CATWEASEL
    catweasel_joystick_widget =gtk_check_button_new_with_label ("Use Catweasel Joystick");
    cat_frame                 =gtk_frame_new ("Catweasel");

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(catweasel_joystick_widget), currprefs.catweasel_joy);
    gtk_container_add (GTK_CONTAINER (cat_frame), catweasel_joystick_widget);
#endif

    gtkutil_add_table (GTK_WIDGET (dvbox),
        joy1_frame, 1, 1, GTK_FILL,
        joy2_frame, 2, 1, GTK_FILL,
        GTKUTIL_ROW_END,
#ifdef CATWEASEL
	cat_frame,  1, 2, GTK_FILL,
	GTKUTIL_ROW_END,
#endif
	GTKUTIL_TABLE_END
    );

#ifdef CATWEASEL
    gtk_signal_connect (GTK_OBJECT (catweasel_joystick_widget), "toggled",
			GTK_SIGNAL_FUNC (on_catweasel_joystick_changed),
			catweasel_joystick_widget);
#endif
}

#ifdef FILESYS
static int hd_change_mode;

static void newdir_ok (void)
{
    int n;
    int readonly = GTK_TOGGLE_BUTTON (readonly_widget)->active;
    int bootpri  = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (bootpri_widget));
    strcpy (dirdlg_devname, gtk_entry_get_text (GTK_ENTRY (devname_entry)));
    strcpy (dirdlg_volname, gtk_entry_get_text (GTK_ENTRY (volname_entry)));
    strcpy (dirdlg_path, gtk_entry_get_text (GTK_ENTRY (path_entry)));

    n = strlen (dirdlg_volname);
    /* Strip colons from the end.  */
    if (n > 0) {
	if (dirdlg_volname[n - 1] == ':')
	    dirdlg_volname[n - 1] = '\0';
    }
    /* Do device name too */
    n = strlen (dirdlg_devname);
    if (n > 0) {
        if (dirdlg_devname[n - 1] == ':')
	    dirdlg_devname[n - 1] = '\0';
    }
    if (strlen (dirdlg_volname) == 0 || strlen (dirdlg_path) == 0) {
	/* Uh, no messageboxes in gtk?  */
    } else if (hd_change_mode) {
	set_filesys_unit (currprefs.mountinfo, selected_hd_row, dirdlg_devname, dirdlg_volname, dirdlg_path,
			  readonly, 0, 0, 0, 0, bootpri, 0, 0);
	set_hd_state ();
    } else {
	add_filesys_unit (currprefs.mountinfo, dirdlg_devname, dirdlg_volname, dirdlg_path,
			  readonly, 0, 0, 0, 0, bootpri, 0, 0);
	set_hd_state ();
    }
    gtk_widget_destroy (dirdlg);
}


GtkWidget *path_selector;

static void did_dirdlg_done_select (GtkObject *o, gpointer entry )
{
    assert (GTK_IS_ENTRY (entry));

    gtk_entry_set_text (GTK_ENTRY (entry), gtk_file_selection_get_filename (GTK_FILE_SELECTION (path_selector)));
}

static void did_dirdlg_select (GtkObject *o, gpointer entry )
{
    assert( GTK_IS_ENTRY(entry) );
    path_selector = gtk_file_selection_new ("Select a folder to mount");
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (path_selector), gtk_entry_get_text (GTK_ENTRY (entry)));
    gtk_window_set_modal (GTK_WINDOW (path_selector), TRUE);

    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(path_selector)->ok_button),
                                          "clicked", GTK_SIGNAL_FUNC (did_dirdlg_done_select),
                                          (gpointer) entry);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(path_selector)->ok_button),
                                          "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                                          (gpointer) path_selector);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(path_selector)->cancel_button),
                                          "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                                          (gpointer) path_selector);

    /* Gtk1.2 doesn't have a directory chooser widget, so we fake one from the
     * file dialog by hiding the widgets related to file selection */
#ifndef GTKMUI
    gtk_widget_hide ((GTK_FILE_SELECTION(path_selector)->file_list)->parent);
    gtk_widget_hide (GTK_FILE_SELECTION(path_selector)->fileop_del_file);
    gtk_widget_hide (GTK_FILE_SELECTION(path_selector)->fileop_ren_file);
    gtk_widget_hide (GTK_FILE_SELECTION(path_selector)->selection_entry);
    gtk_entry_set_text (GTK_ENTRY (GTK_FILE_SELECTION(path_selector)->selection_entry), "" );
#endif

    gtk_widget_show (path_selector);
}

static void dirdlg_on_change (GtkObject *o, gpointer data)
{
  int can_complete = (strlen (gtk_entry_get_text (GTK_ENTRY(path_entry))) !=0)
                  && (strlen (gtk_entry_get_text (GTK_ENTRY(volname_entry))) != 0)
	          && (strlen (gtk_entry_get_text (GTK_ENTRY(devname_entry))) != 0);

  gtk_widget_set_sensitive (dirdlg_ok, can_complete);
}

static void create_dirdlg (const char *title)
{
    GtkWidget *dialog_vbox, *dialog_hbox, *vbox, *frame, *table, *hbox, *thing, *label, *button;

    dirdlg = gtk_dialog_new ();
#if !defined GTKMUI
    gtk_widget_show (dirdlg);
#endif

    gtk_window_set_title (GTK_WINDOW (dirdlg), title);
    gtk_window_set_position (GTK_WINDOW (dirdlg), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal (GTK_WINDOW (dirdlg), TRUE);

    dialog_vbox = GTK_DIALOG (dirdlg)->vbox;
    gtk_widget_show (dialog_vbox);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox, TRUE, FALSE, 0);

    frame = gtk_frame_new ("Mount host folder");
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 8);

    hbox = gtk_hbox_new (FALSE, 4);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (frame), hbox);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);

    label  = gtk_label_new ("Path");
//    gtk_label_set_pattern (GTK_LABEL (label), "_");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

#ifndef GTKMUI
    thing = gtk_entry_new_with_max_length (255);
#else
    thing = gtk_entry_new();
#endif
    gtk_signal_connect (GTK_OBJECT (thing), "changed", (GtkSignalFunc) dirdlg_on_change, (gpointer) NULL);
    gtk_box_pack_start (GTK_BOX (hbox), thing, TRUE, TRUE, 0);
    gtk_widget_show (thing);
    path_entry = thing;

    button = gtk_button_new_with_label ("Select...");
    gtk_signal_connect (GTK_OBJECT (button), "clicked", (GtkSignalFunc) did_dirdlg_select, (gpointer) path_entry);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    frame = gtk_frame_new ("As Amiga disk");
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 8);

    table = gtk_table_new (3, 4, FALSE);
    gtk_widget_show (table);
    gtk_container_add (GTK_CONTAINER (frame), table);
    gtk_container_set_border_width (GTK_CONTAINER (table), 8);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);

        label = gtk_label_new ("Device name");
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
        gtk_widget_show (label);
#if !defined GTKMUI
        thing = gtk_entry_new_with_max_length (255);
#else
        thing = gtk_entry_new();
#endif
	gtk_signal_connect (GTK_OBJECT (thing), "changed", (GtkSignalFunc) dirdlg_on_change, (gpointer) NULL);
        gtk_widget_show (thing);
        gtk_table_attach (GTK_TABLE (table), thing, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
        gtk_widget_set_usize (thing, 200, -1);
        devname_entry = thing;

        label = gtk_label_new ("Volume name");
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
        gtk_widget_show (label);
#if !defined GTKMUI
        thing = gtk_entry_new_with_max_length (255);
#else
        thing = gtk_entry_new();
#endif
	gtk_signal_connect (GTK_OBJECT (thing), "changed", (GtkSignalFunc) dirdlg_on_change, (gpointer) NULL);
        gtk_table_attach (GTK_TABLE (table), thing, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
        gtk_widget_show (thing);
        gtk_widget_set_usize (thing, 200, -1);
        volname_entry = thing;

        label = gtk_label_new ("Boot priority");
        gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);
        gtk_widget_show (label);
        thing = gtk_spin_button_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, -128, 127, 1, 5, 5)), 1, 0);
        gtk_table_attach (GTK_TABLE (table), thing, 3, 4, 0, 2,
                        (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                        (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
        gtk_widget_show (thing);
        bootpri_widget = thing;

        readonly_widget = gtk_check_button_new_with_label ("Read only");
        gtk_table_attach (GTK_TABLE (table), readonly_widget, 0, 4, 2, 3,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);
        gtk_widget_show (readonly_widget);

    dialog_hbox = GTK_DIALOG (dirdlg)->action_area;

#if defined GTKMUI
    hbox = gtk_hbox_new(FALSE,4);
#else
    hbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
#endif
    gtk_box_pack_start (GTK_BOX (dialog_hbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    button = gtk_button_new_with_label ("OK");
    gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC(newdir_ok), NULL);
//    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
//    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    dirdlg_ok = button;

    button = gtk_button_new_with_label ("Cancel");
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dirdlg));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);

#if defined GTKMUI
    gtk_widget_show (dirdlg);
#endif
}

static void did_newdir (void)
{
    hd_change_mode = 0;
    create_dirdlg ("Add a hard disk");
}
static void did_newhdf (void)
{
    hd_change_mode = 0;
}

static void did_hdchange (void)
{
    int secspertrack, surfaces, reserved, blocksize, bootpri;
    uae_u64 size;
    int cylinders, readonly, flags;
    char *devname, *volname, *rootdir, *filesysdir;
    const char *failure;

    failure = get_filesys_unit (currprefs.mountinfo, selected_hd_row,
				&devname, &volname, &rootdir, &readonly,
				&secspertrack, &surfaces, &reserved,
				&cylinders, &size, &blocksize, &bootpri,
				&filesysdir, &flags);

    hd_change_mode = 1;
    if (is_hardfile (currprefs.mountinfo, selected_hd_row)) {
    } else {
	create_dirdlg ("Hard disk properties");
        gtk_entry_set_text (GTK_ENTRY (devname_entry), devname);
	gtk_entry_set_text (GTK_ENTRY (volname_entry), volname);
	gtk_entry_set_text (GTK_ENTRY (path_entry), rootdir);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (readonly_widget), readonly);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (bootpri_widget), bootpri);
   }
}
static void did_hddel (void)
{
    kill_filesys_unit (currprefs.mountinfo, selected_hd_row);
    set_hd_state ();
}

static void hdselect (GtkWidget *widget, gint row, gint column, GdkEventButton *bevent,
		      gpointer user_data)
{
    selected_hd_row = row;
    gtk_widget_set_sensitive (hdchange_button, TRUE);
    gtk_widget_set_sensitive (hddel_button, TRUE);
}

static void hdunselect (GtkWidget *widget, gint row, gint column, GdkEventButton *bevent,
			gpointer user_data)
{
    gtk_widget_set_sensitive (hdchange_button, FALSE);
    gtk_widget_set_sensitive (hddel_button, FALSE);
}
#endif // FILESYS

static GtkWidget *make_buttons (const char *label, GtkWidget *box, void (*sigfunc) (void), GtkWidget *(*create)(const char *label))
{
    GtkWidget *thing = create (label);

    gtk_widget_show (thing);
    gtk_signal_connect (GTK_OBJECT (thing), "clicked", (GtkSignalFunc) sigfunc, NULL);
    gtk_box_pack_start (GTK_BOX (box), thing, TRUE, TRUE, 0);

    return thing;
}
#define make_button(label, box, sigfunc) make_buttons(label, box, sigfunc, gtk_button_new_with_label)

#ifdef FILESYS
static void make_hd_widgets (GtkWidget *dvbox)
{
    GtkWidget *frame, *vbox, *scrollbox, *thing, *buttonbox, *hbox;
//    char *titles [] = {
//	"Volume", "File/Directory", "R/O", "Heads", "Cyl.", "Sec.", "Rsrvd", "Size", "Blksize"
//    };

    frame = gtk_frame_new (NULL);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (dvbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 8);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    scrollbox = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrollbox);
    gtk_box_pack_start (GTK_BOX (vbox), scrollbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (scrollbox), 8);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollbox), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    thing = gtk_clist_new_with_titles (HDLIST_MAX_COLS, (gchar **)hdlist_col_titles );
    gtk_clist_set_selection_mode (GTK_CLIST (thing), GTK_SELECTION_SINGLE);
    gtk_signal_connect (GTK_OBJECT (thing), "select_row", (GtkSignalFunc) hdselect, NULL);
    gtk_signal_connect (GTK_OBJECT (thing), "unselect_row", (GtkSignalFunc) hdunselect, NULL);
    hdlist_widget = thing;
    gtk_widget_set_usize (thing, -1, 200);
    gtk_widget_show (thing);
    gtk_container_add (GTK_CONTAINER (scrollbox), thing);

    /* The buttons */
#if !defined GTKMUI
    buttonbox = gtk_hbutton_box_new ();
#else
    buttonbox = gtk_hbox_new (FALSE,4);
#endif
    gtk_widget_show (buttonbox);
    gtk_box_pack_start (GTK_BOX (vbox), buttonbox, FALSE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 8);
#if !defined GTKMUI 
    gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_SPREAD);
#endif

    make_button ("Add...", buttonbox, did_newdir);
#if 0 /* later... */
    make_button ("New hardfile...", buttonbox, did_newhdf);
#endif
    hdchange_button = make_button ("Properties...", buttonbox, did_hdchange);
    hddel_button = make_button ("Remove", buttonbox, did_hddel);

    hdpanel = frame;
}
#endif

/*******************************
 * make_config_frame
 *
 * crate a vbox in the supplied
 * box, which has a frame
 * and uses up the complete
 * horizontal space.
 *******************************/
static GtkWidget *make_config_frame (const char *title, GtkWidget *vbox) {

    GtkWidget *thing = gtk_frame_new (title);
    GtkWidget *buttonbox = gtk_vbox_new (TRUE, 4);

    gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 4);
    gtk_container_add (GTK_CONTAINER (thing), buttonbox);
    gtk_box_pack_start (GTK_BOX (vbox), thing, FALSE, TRUE, 0);
    gtk_widget_show (buttonbox);
    gtk_widget_show (thing);

    return buttonbox;
}

static void config_descr_on_change (GtkObject *o, gpointer data) {

  strncpy(changed_prefs.description, gtk_entry_get_text(GTK_ENTRY(config_description_widget)), 250);
}

static void make_about_widgets (GtkWidget *dvbox) {

    GtkWidget   *thing;
    GtkWidget   *vbox;
    GList       *childs;
    GtkWidget   *filename;
    GtkTooltips *tip;

#if GTK_MAJOR_VERSION >= 2
    const char title[] = "<span font_desc=\"Sans 24\">" UAE_VERSION_STRING " </span>";
#else
    const char title[] = UAE_VERSION_STRING;
#endif

    add_empty_vbox (dvbox);

#if 0
#ifdef GTKMUI
    thing = gtk_label_new ("WARNING -- Crashes *will* happen -- WARNING");
    gtk_widget_show (thing);
    add_centered_to_vbox (dvbox, thing);
#endif
#endif


#if GTK_MAJOR_VERSION >= 2 && !defined GTKMUI
    thing = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (thing), title);
#else
    thing = gtk_label_new (title);
#if !defined GTKMUI
    {
	GdkFont *font = gdk_font_load ("-*-helvetica-medium-r-normal--*-240-*-*-*-*-*-*");
	if (font) {
	    GtkStyle *style = gtk_style_copy (GTK_WIDGET (thing)->style);
	    gdk_font_unref (style->font);
	    gdk_font_ref (font);
	    style->font = font;
	    gtk_widget_set_style (thing, style);
	}
    }
#endif
#endif
    gtk_widget_show (thing);
    add_centered_to_vbox (dvbox, thing);

#ifdef PACKAGE_VERSION
    thing = gtk_label_new ("Version " PACKAGE_VERSION );
    gtk_widget_show (thing);
    add_centered_to_vbox (dvbox, thing);
#endif
#ifdef GTKMUI
    thing = gtk_label_new ("GTK UI built with GTK-MUI");
    gtk_widget_show (thing);
    add_centered_to_vbox (dvbox, thing);
    thing = gtk_label_new ("gtk-mui -at- oliver-brunner.de");
    gtk_widget_show (thing);
    add_centered_to_vbox (dvbox, thing);

#endif
/* TEST VERSION */
//#if 0
    thing = gtk_label_new ("*** THIS IS AN INTERNAL BETA TEST VERSION ***");
    gtk_widget_show (thing);
    add_centered_to_vbox (dvbox, thing);
    thing = gtk_label_new ("*** PLEASE DO NOT DISTRIBUTE ***");
    gtk_widget_show (thing);
    add_centered_to_vbox (dvbox, thing);
//#endif

    add_empty_vbox (dvbox);

    /* Current file display */
    vbox = make_config_frame ("Current Configuration ", dvbox);
    gtk_widget_show (vbox);

    config_description_widget = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(config_description_widget), currprefs.description);
    gtk_box_pack_start (GTK_BOX (vbox), config_description_widget, TRUE, TRUE, 0);
    gtk_widget_show (config_description_widget);

    gtk_signal_connect (GTK_OBJECT (config_description_widget), "changed", 
                        (GtkSignalFunc) config_descr_on_change, (gpointer) NULL);

    /* config file path */
    config_path_widget = make_file_widget (vbox);

    tip=gtk_tooltips_new();

    gtk_tooltips_set_tip (GTK_TOOLTIPS (tip), config_description_widget,
				 "Description of the config file.\nRemember to leave this field with RETURN to really change the content!",
				 "Description of the config file.\nRemember to leave this field with RETURN to really change the content!");
}

/******************************
 * Quit hooks
 ******************************/
static void on_quit_clicked (void) {

  DEBUG_LOG ("on_quit_clicked (quit_gui: %d).\n",quit_gui);

  if (!quit_gui) {
    write_comm_pipe_int (&from_gui_pipe, UAECMD_QUIT, 1);
  }
}


static gint did_guidlg_delete (GtkWidget* window, GdkEventAny* e, gpointer data) {

  DEBUG_LOG("did_guidlg_delete(%lx, %lx, %lx)\n", window, e, data);

#if 0
  if (!quit_gui) {
    write_comm_pipe_int (&from_gui_pipe, UAECMD_QUIT, 1);
  }
#endif
  on_quit_clicked();
  gui_window = 0;
  return FALSE;
}


/******************************
 * "Load Config"
 ******************************/
static void on_menu_loadconfig (void) {

  DEBUG_LOG ("Load config ...\n");

  if (!quit_gui) {
    write_comm_pipe_int (&from_gui_pipe, UAECMD_LOAD_CONFIG, 1);
  }
}

/******************************
 * "Load Config From"
 ******************************/
static void did_close_config (gpointer gdata) {
  DEBUG_LOG("cancel save!\n");

  return;
}

static void load_config_select (GtkObject *o) {

  const char *s = gtk_file_selection_get_filename (GTK_FILE_SELECTION (o));

  /* ? */
  if (quit_gui) {
    return;
  }

  DEBUG_LOG("old config name: %s\n", optionsfile);
  DEBUG_LOG("new config name: %s\n", s);

  uae_sem_wait (&gui_sem);

  strncpy (optionsfile, s, 255);

  uae_sem_post (&gui_sem);

  gtk_widget_destroy (GTK_WIDGET(o));

  if (!quit_gui) {
    write_comm_pipe_int (&from_gui_pipe, UAECMD_LOAD_CONFIG, 1);
  }
}

static void on_menu_loadconfigfrom (void) {

  GtkWidget *config_selector;
  
  DEBUG_LOG ("Load config from ...\n");

  config_selector = make_file_selector ("Load configuration file from ..",load_config_select,did_close_config);

  /* set old path and filename */
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(config_selector), optionsfile);

  /* launch it */
  gtk_widget_show(config_selector);

  DEBUG_LOG ("exit\n");
}


/***********************************
 * "Save Config"
 ***********************************/
static void on_menu_saveconfig (void) {

  DEBUG_LOG ("Save config ...\n");
  if (!quit_gui) {
    write_comm_pipe_int (&from_gui_pipe, UAECMD_SAVE_CONFIG, 1);
  }
}

/***********************************
 * "Save Config As"
 ***********************************/
static void did_config_select (GtkObject *o) {

  const char *s = gtk_file_selection_get_filename (GTK_FILE_SELECTION (o));

  /* ? */
  if (quit_gui) {
    return;
  }

  DEBUG_LOG("old config name: %s\n", optionsfile);
  DEBUG_LOG("new config name: %s\n", s);

  uae_sem_wait (&gui_sem);

  strncpy (optionsfile, s, 255);

  uae_sem_post (&gui_sem);

  gtk_widget_destroy (GTK_WIDGET(o));

  if (!quit_gui) {
    write_comm_pipe_int (&from_gui_pipe, UAECMD_SAVE_CONFIG, 1);
    /* need to manually update the widget here, as no gui_update() is called on save */
    gtk_label_set_text (GTK_LABEL (config_path_widget), optionsfile);
  }
}

static void on_menu_saveconfigas (void) {
  GtkWidget *config_selector;
  
  DEBUG_LOG ("Save config as ...\n");

  config_selector = make_file_selector ("Save configuration file as ..", did_config_select, did_close_config);

  /* set old path and filename */
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(config_selector), optionsfile);

  /* launch it */
  gtk_widget_show(config_selector);

  DEBUG_LOG ("exit\n");
}

static void on_screen_changed (jDisplay *j) {

  changed_prefs.gfx_width_fs  = j->gfx_width_fs;
  changed_prefs.gfx_height_fs = j->gfx_height_fs;
}

static void on_window_changed (jDisplay *j) {

  changed_prefs.gfx_width_win  = j->gfx_width_win;
  changed_prefs.gfx_height_win = j->gfx_height_win;
}

static void on_chipset_changed (jDisplay *j) {

  changed_prefs.chipset_mask = j->chipset_mask;
  changed_prefs.ntscmode     = j->ntscmode;
}

static void on_linemode_changed (jDisplay *j) {

  changed_prefs.gfx_linedbl  = j->gfx_linedbl;
}

static void on_settings_changed (jDisplay *j) {

  changed_prefs.gfx_correct_aspect= j->gfx_correct_aspect;
  changed_prefs.gfx_lores         = j->gfx_lores;
  changed_prefs.gfx_afullscreen   = j->gfx_fullscreen_amiga;
  changed_prefs.gfx_pfullscreen   = j->gfx_fullscreen_p96;

  if(changed_prefs.gfx_pfullscreen) {
    currprefs.amiga_screen_type=UAESCREENTYPE_CUSTOM;
    changed_prefs.amiga_screen_type=UAESCREENTYPE_CUSTOM;
  }
  else {
    currprefs.amiga_screen_type=UAESCREENTYPE_PUBLIC;
    changed_prefs.amiga_screen_type=UAESCREENTYPE_PUBLIC;
  }
}

static void on_centering_changed (jDisplay *j) {

  changed_prefs.gfx_xcenter= j->gfx_xcenter;
  changed_prefs.gfx_ycenter= j->gfx_ycenter;
}

static void make_display_widgets (GtkWidget *vbox) {

  jdisp_panel=jdisplay_new();

  gtk_signal_connect (GTK_OBJECT (jdisp_panel), "screen-changed",
		      GTK_SIGNAL_FUNC (on_screen_changed),
     		      NULL);
  gtk_signal_connect (GTK_OBJECT (jdisp_panel), "window-changed",
		      GTK_SIGNAL_FUNC (on_window_changed),
     		      NULL);
  gtk_signal_connect (GTK_OBJECT (jdisp_panel), "chipset-changed",
		      GTK_SIGNAL_FUNC (on_chipset_changed),
     		      NULL);
  gtk_signal_connect (GTK_OBJECT (jdisp_panel), "linemode-changed",
		      GTK_SIGNAL_FUNC (on_linemode_changed),
     		      NULL);
  gtk_signal_connect (GTK_OBJECT (jdisp_panel), "settings-changed",
		      GTK_SIGNAL_FUNC (on_settings_changed),
     		      NULL);
  gtk_signal_connect (GTK_OBJECT (jdisp_panel), "centering-changed",
		      GTK_SIGNAL_FUNC (on_centering_changed),
     		      NULL);

  /* our child, the chipsetspeed_panel */
  gtk_signal_connect (GTK_OBJECT (chipsetspeed_panel), "framerate-changed",
		      GTK_SIGNAL_FUNC (on_framerate_changed),
     		      NULL);
  gtk_signal_connect (GTK_OBJECT (chipsetspeed_panel), 
			"sprite-collisions-changed",
		      GTK_SIGNAL_FUNC (on_collision_level_changed),
     		      NULL);
  gtk_signal_connect (GTK_OBJECT (chipsetspeed_panel), 
                        "immediate-blits-changed",
		      GTK_SIGNAL_FUNC (on_immediate_blits_changed),
     		      NULL);

  gtk_widget_show_all(jdisp_panel);

  gtk_container_add (GTK_CONTAINER (vbox), jdisp_panel);
}

static void make_integration_widgets (GtkWidget *vbox) {

  jint_panel=jintegration_new();

  gtk_signal_connect (GTK_OBJECT (jint_panel), "coherent-changed",
		      GTK_SIGNAL_FUNC (on_coherent_changed),
     		      NULL);

  gtk_signal_connect (GTK_OBJECT (jint_panel), "mouse-changed",
		      GTK_SIGNAL_FUNC (on_mouse_changed),
     		      NULL);

  gtk_signal_connect (GTK_OBJECT (jint_panel), "clipboard-changed",
		      GTK_SIGNAL_FUNC (on_clipboard_changed),
     		      NULL);

  gtk_signal_connect (GTK_OBJECT (jint_panel), "launch-changed",
		      GTK_SIGNAL_FUNC (on_launch_changed),
     		      NULL);


  /* it gets unlocked, as soon as the daemons start */
  g_signal_emit_by_name(jint_panel,"lock-it",NULL);

  gtk_widget_show_all(jint_panel);
  gtk_container_add (GTK_CONTAINER (vbox), jint_panel);
}

void unlock_jgui(void);
/* this is called from j-dispatch 
 * otherwise j-dispatch would need all gtk/glib includes
 */
void unlock_jgui(void) {
    g_signal_emit_by_name(jint_panel,"unlock-it",NULL);
}

static void create_guidlg (void) {

    GtkWidget *window, *notebook;
    GtkWidget *buttonbox, *buttonbox2, *vbox, *hbox;
    GtkWidget *thing;
#if 0
    GtkWidget *menubar, *menuitem, *menuitem_menu;
#endif
    unsigned int i;
    static const struct _pages {
	const char *title;
	void (*createfunc)(GtkWidget *);
    } pages[] = {
	{ "Floppy disks", make_floppy_disks },
	{ "Memory",       make_mem_widgets },
	{ "CPU",          make_cpu_widgets },
//	{ "Graphics",     make_gfx_widgets },
	//{ "Chipset",      make_chipset_widgets },
	{ "Display",      make_display_widgets },
	{ "Integration",  make_integration_widgets },
	{ "Sound",        make_sound_widgets },
#ifdef JIT
 	{ "JIT",          make_comp_widgets },
#endif
	{ "Game ports",   make_joy_widgets },
#ifdef FILESYS
	{ "Hard disks",   make_hd_widgets },
#endif
	{ "About",        make_about_widgets }
    };

    DEBUG_LOG ("Entered\n");
    //DebOut ("Entered\n");

    gui_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (gui_window), PACKAGE_NAME " control: Hot Key = <ctrl alt j>");
    gtk_signal_connect (GTK_OBJECT(gui_window), "delete_event", GTK_SIGNAL_FUNC(did_guidlg_delete), NULL);

    /* keep the GUI hidden, if !currprefs.start_gui 
     * use Exchange to show it
		 *
		 * ATTENTION: according to the MUI autodocs the application remembers this state and
		 *            hides all newly opened windows. This seems not to be the case for Zune,
		 *            so we hide after we opened our window with gtk_window_new..
     */
    DEBUG_LOG("create_guidlg: gtk_mui_application_iconify %d\n", !currprefs.start_gui);
    gtk_mui_application_iconify(!currprefs.start_gui);
    DEBUG_LOG("create_guidlg: gtk_mui_application_is_iconified %d\n", gtk_mui_application_is_iconified());

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_add (GTK_CONTAINER (gui_window), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (gui_window), 10);

#if 0
    /* Quick and dirty menu bar */
    menubar = gtk_menu_bar_new();
    gtk_widget_show (menubar);
    gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

    menuitem = gtk_menu_item_new_with_mnemonic ("File");
    gtk_widget_show (menuitem);
    gtk_container_add (GTK_CONTAINER (menubar), menuitem);

    menuitem_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menuitem_menu);

    thing = gtk_menu_item_new_with_mnemonic ("Load Config");
    gtk_widget_show (thing);
    gtk_container_add (GTK_CONTAINER (menuitem_menu), thing);
    gtk_signal_connect (GTK_OBJECT(thing), "activate", GTK_SIGNAL_FUNC(on_menu_loadconfig), NULL);

    thing = gtk_menu_item_new_with_mnemonic ("Load Config From ...");
    gtk_widget_show (thing);
    gtk_container_add (GTK_CONTAINER (menuitem_menu), thing);
    gtk_signal_connect (GTK_OBJECT(thing), "activate", GTK_SIGNAL_FUNC(on_menu_loadconfigfrom), NULL);

    thing = gtk_menu_item_new_with_mnemonic ("Save Config");
    gtk_widget_show (thing);
    gtk_container_add (GTK_CONTAINER (menuitem_menu), thing);
    gtk_signal_connect (GTK_OBJECT(thing), "activate", GTK_SIGNAL_FUNC(on_menu_saveconfig), NULL);

    thing = gtk_menu_item_new_with_mnemonic ("Save Config As ...");
    gtk_widget_show (thing);
    gtk_container_add (GTK_CONTAINER (menuitem_menu), thing);
    gtk_signal_connect (GTK_OBJECT(thing), "activate", GTK_SIGNAL_FUNC(on_menu_saveconfigas), NULL);


#if !defined GTKMUI
    thing = gtk_separator_menu_item_new ();
    gtk_widget_show (thing);
    gtk_container_add (GTK_CONTAINER (menuitem_menu), thing);
#endif

    thing = gtk_menu_item_new_with_mnemonic ("Quit");
    gtk_widget_show (thing);
    gtk_container_add (GTK_CONTAINER (menuitem_menu), thing);
    gtk_signal_connect (GTK_OBJECT(thing), "activate", GTK_SIGNAL_FUNC(on_quit_clicked), NULL);
#endif

    /* dummy */
    thing = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), thing, FALSE, TRUE, 0);
    gtk_widget_show (thing);

    /* First line - buttons and power LED */
    hbox = gtk_hbox_new (FALSE, 10);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    /* The buttons */
    buttonbox = gtk_hbox_new (TRUE, 6);
    gtk_widget_show (buttonbox);
    gtk_box_pack_start (GTK_BOX (hbox), buttonbox, TRUE, TRUE, 0);
    start_uae_widget = make_button  ("Start", buttonbox, on_start_clicked);
    stop_uae_widget  = make_button  ("Stop",  buttonbox, on_stop_clicked);
    pause_uae_widget = make_buttons ("Pause", buttonbox, (void (*) (void))on_pause_clicked, gtk_toggle_button_new_with_label);
#ifdef DEBUGGER
//FIXME    debug_uae_widget = make_button  ("Debug", buttonbox, on_debug_clicked);
#endif

    reset_uae_widget = make_button  ("Reset", buttonbox, on_reset_clicked);
    make_button ("Quit", buttonbox, on_quit_clicked);

    /* The LED */
    power_led = led_new();
    gtk_widget_show(power_led);
    thing = gtk_vbox_new(FALSE, 4);
    add_empty_hbox(thing);
    gtk_container_add(GTK_CONTAINER(thing), power_led);
    add_empty_hbox(thing);
    thing = make_labelled_widget ("Power:", thing);
    gtk_widget_show (thing);
    gtk_box_pack_start (GTK_BOX (hbox), thing, FALSE, TRUE, 0);

    /* More buttons */
    buttonbox = gtk_hbox_new (TRUE, 4);
    gtk_widget_show (buttonbox);
    gtk_box_pack_start (GTK_BOX (vbox), buttonbox, FALSE, FALSE, 0);

    /* Place a separator below those buttons.  */
    thing = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), thing, FALSE, TRUE, 0);
    gtk_widget_show (thing);

    /* Now the notebook */
    notebook = gtk_notebook_new ();

    for (i = 0; i < sizeof pages / sizeof (struct _pages); i++) {
    //  asm("int3");
	//DebOut ("guiforloop - %s(%d)\n",pages[i].title,i);
	thing = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (thing), 10);
	gtk_widget_show (thing);

	pages[i].createfunc (thing);

	/* gtk_mui_list_child_tree(thing);*/
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), thing, gtk_label_new (pages[i].title));
    }
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show (notebook);
    /* Put "about" screen first.  */
    gtk_notebook_set_page (GTK_NOTEBOOK (notebook), i - 1);

    /* Configuration buttons */
    buttonbox2 = gtk_hbox_new (TRUE, 4);
    gtk_widget_show (buttonbox2);
    gtk_box_pack_start (GTK_BOX (vbox), buttonbox2, TRUE, TRUE, 0);
    config_load_widget      = make_button  ("Load",         buttonbox2, on_menu_loadconfig);
    config_load_from_widget = make_button  ("Load from ..", buttonbox2, on_menu_loadconfigfrom);
    config_save_widget      = make_button  ("Save",         buttonbox2, on_menu_saveconfig);
    config_save_as_widget   = make_button  ("Save As ..",   buttonbox2, on_menu_saveconfigas);

    gtk_widget_show (vbox);

    gtk_timeout_add (1000, (GtkFunction)leds_callback, 0);
    //DebOut("end\n");
    //
#if defined GTKMUI
    DEBUG_LOG("create_guidlg: gtk_mui_application_gui_hotkey: ctrl alt j\n");
    gtk_mui_application_gui_hotkey("ctrl alt j");
#endif
}

/*
 * gtk_gui_thread()
 *
 * This is launched as a separate thread to the main UAE thread
 * to create and handle the GUI. After the GUI has been set up,
 * this calls the standard GTK+ event processing loop.
 *
 */

#if 0
static guint32 timer=0;
static GtkWidget *splash = NULL;
static GtkWidget *splash_label = NULL;

extern struct Task *splash_window_task;

static gint gtk_splash_timer (GtkWidget *splash) {

  DEBUG_LOG("entered!\n");

  if(!splash_window_task) {
    DEBUG_LOG("no splash window\n");
    return FALSE;
  }

  DEBUG_LOG("close window!\n");
  gtk_widget_hide(splash);


  Signal(splash_window_task, SIGBREAKF_CTRL_C);

  if(timer) {
    /* timer is automatically removed.. ? */
//    gtk_timeout_remove (timer);
    timer=0;
  }

  return FALSE;
}

void close_splash(void) {

  DEBUG_LOG("timer %lx, splash %lx\n", timer, splash);

  if(timer) {
    DEBUG_LOG("call gtk_timeout_remove(%lx)\n", timer);
    gtk_timeout_remove (timer);
    timer=0;
  }

  if(splash) {
    gtk_widget_hide(splash);
  }

  DEBUG_LOG("done.\n");
}

void do_splash(char *text, int time) {

  GtkWidget *frame;

  DEBUG_LOG("text %s, time %d\n", text, time);

  if(!time || !text) {
    close_splash();
    return;
  }

  /* reset old timer */
  if(timer) {
    gtk_timeout_remove (timer);
    timer=0;
  }


  if(!splash) {

    splash = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_border_width (GTK_CONTAINER (splash), 10);

    frame=gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (splash), frame);
    gtk_widget_show (frame);
  }

  if(!splash_label) {
    splash_label=gtk_label_new(text);
    gtk_box_pack_end (GTK_BOX (frame), splash_label, 0, 0, 0);
    gtk_widget_show (splash_label);
  }
  else {
    gtk_label_set_text (GTK_LABEL (splash_label), text);
  }

  /* set it again */
  timer = gtk_timeout_add (time*1000, (GtkFunction) gtk_splash_timer, (gpointer) splash);
  gtk_widget_show (splash);

}

static void open_splash_pipe (smp_comm_pipe *msg_pipe) {

  DEBUG_LOG("entered\n");

  const gchar *text = (const gchar *)  read_comm_pipe_pvoid_blocking (msg_pipe);
  const int   *time = (const int *  )  read_comm_pipe_pvoid_blocking (msg_pipe);

  DEBUG_LOG("text %s, time %d\n", text, time);

  do_splash(text, time);
}

void show_splash(char *text, int time) {

  DEBUG_LOG("entered\n");

  DEBUG_LOG("text %s, time %d\n", text, time);

  if (gui_available) {
    write_comm_pipe_int   (&to_gui_pipe, GUICMD_SPLASH, 0);
    write_comm_pipe_pvoid (&to_gui_pipe, (void *) text, 0);
    write_comm_pipe_int   (&to_gui_pipe, time, 0);
  }
  else {
    DEBUG_LOG("gui not yet available!\n");
  }
}
#endif

static void *gtk_gui_thread (void *dummy)
{
    /* fake args for gtk_init() */
    int argc = 2;
    char *a[] = {"UAE", "WE_HAVE_OWN_LIBS", NULL};
    char **argv = a;

    DEBUG_LOG ("Started\n");
    //DebOut ("Started\n");

    gui_active = 0;

    if (gtk_init_check (&argc, &argv)) {
	DEBUG_LOG ("gtk_init() successful\n");
	//DebOut ("gtk_init() successful\n");

#if !defined GTKMUI
	gtk_rc_parse ("uaegtkrc");
#endif
	gui_available = 1;
	//DebOut("gui_available = 1\n");

	/* Add callback to GTK+ mainloop to handle messages from UAE */
	gtk_timeout_add (250, (GtkFunction)my_idle, 0);

	/* We're ready - tell the world */
	uae_sem_post (&gui_init_sem);
	uae_sem_post (&gui_main_wait_sem);

	/* Enter GTK+ main loop */
	DEBUG_LOG ("Entering GTK+ main loop\n");
	//DebOut ("Entering GTK+ main loop\n");

#if 0
  /* try again, if main was too early to display it */
  if(!splash && currprefs.splash_time) {
    show_splash(currprefs.splash_text, currprefs.splash_time);
  }
#endif

#if 0
{
  GtkWidget *splash=jsplash_new();
kprintf("FOOOO=%lx\n", splash);
  gtk_widget_show(splash);
kprintf("FOOOO -------\n");
}
#endif

	gtk_main ();
	//DebOut ("gtk_main exit\n");

	/* Main loop has exited, so the GUI will quit */
	quitted_gui = 1;
	//DebOut("quitted_gui = 1;\n");

	uae_sem_post (&gui_quit_sem);
/*	DEBUG_LOG ("Exiting\n");*/
    } else {
        DEBUG_LOG ("gtk_init() failed\n");
        /* If GTK+ can't display, we still need to say we're done */
        uae_sem_post (&gui_init_sem);
    }
    //DebOut("exit\n");
    /* e-uae does not quit, if the GUI dies.
     * j-uae does quit, as this might be a commodity signal not
     * only for the GUI.
     */
    uae_quit();
    return 0;
}

void gui_fps (int fps, int idle)
{
    gui_data.fps  = fps;
    gui_data.idle = idle;
}

/*
 * gui_led()
 *
 * Called from the main UAE thread to inform the GUI
 * of disk activity so that indicator LEDs may be refreshed.
 *
 * We don't respond to this, since our LEDs are updated
 * periodically by my_idle()
 */
void gui_led (int num, int on)
{
}


/*
 * gui_filename()
 *
 * This is called from the main UAE thread to inform
 * the GUI that a floppy disk has been inserted or ejected.
 */
void gui_filename (int num, const char *name)
{
    DEBUG_LOG ("Entered with drive:%d name:%s\n", num, name);

    if (gui_available) {
        write_comm_pipe_int (&to_gui_pipe, GUICMD_DISKCHANGE, 0);
        write_comm_pipe_int (&to_gui_pipe, num, 1);
    }
    return;
}


/*
 * gui_handle_events()
 *
 * This is called from the main UAE thread to handle the
 * processing of GUI-related events sent from the GUI thread.
 *
 * If the UAE emulation proper is not running yet or is paused,
 * this loops continuously waiting for and responding to events
 * until the emulation is started or resumed, respectively. When
 * the emulation is running, this is called periodically from
 * the main UAE event loop.
 */
void gui_handle_events (void)
{
    if (!gui_available)
	return;

    while (comm_pipe_has_data (&from_gui_pipe)) {
	int cmd = read_comm_pipe_int_blocking (&from_gui_pipe);

	switch (cmd) {
	    case UAECMD_EJECTDISK: {
		int n = read_comm_pipe_int_blocking (&from_gui_pipe);
	        uae_sem_wait (&gui_sem);
		changed_prefs.df[n][0] = '\0';
	        uae_sem_post (&gui_sem);
		if (uae_get_state () != UAE_STATE_RUNNING) {
		    /* When UAE is running it will notify the GUI when a disk has been inserted
		     * or removed itself. When UAE is paused, however, we need to do this ourselves
		     * or the change won't be realized in the GUI until UAE is resumed */
                    write_comm_pipe_int (&to_gui_pipe, GUICMD_DISKCHANGE, 0);
	            write_comm_pipe_int (&to_gui_pipe, n, 1);
		}
		break;
	    }
	    case UAECMD_INSERTDISK: {
		int n = read_comm_pipe_int_blocking (&from_gui_pipe);
		uae_sem_wait (&gui_sem);
		strncpy (changed_prefs.df[n], new_disk_string[n], 255);
		free (new_disk_string[n]);
		new_disk_string[n] = 0;
		changed_prefs.df[n][255] = '\0';
		uae_sem_post (&gui_sem);
		if (uae_get_state () != UAE_STATE_RUNNING) {
		    /* When UAE is running it will notify the GUI when a disk has been inserted
		     * or removed itself. When UAE is paused, however, we need to do this ourselves
		     * or the change won't be realized in the GUI until UAE is resumed */
                    write_comm_pipe_int (&to_gui_pipe, GUICMD_DISKCHANGE, 0);
	            write_comm_pipe_int (&to_gui_pipe, n, 1);
		}
		break;
	    }
	    case UAECMD_RESET:
		uae_reset (0);
		break;
#ifdef DEBUGGER
	    case UAECMD_DEBUG:
		activate_debugger ();
		break;
#endif
	    case UAECMD_QUIT:
		uae_stop (); /* added 30.06.2011 on Paolo's request.. should not be necessary? */
		uae_quit ();
		break;
	    case UAECMD_PAUSE:
		uae_pause ();
		break;
	    case UAECMD_RESUME:
		uae_resume ();
		break;
	    case UAECMD_SAVE_CONFIG:
		DEBUG_LOG("UAECMD_SAVE_CONFIG\n");
		uae_sem_wait (&gui_sem);
		uae_save_config ();
		uae_sem_post (&gui_sem);
		DEBUG_LOG("UAECMD_SAVE_CONFIG done\n");
		break;
	    case UAECMD_LOAD_CONFIG:
		DEBUG_LOG("UAECMD_LOAD_CONFIG\n");
		uae_sem_wait (&gui_sem);
		uae_load_config ();

		uae_sem_post (&gui_sem);
		DEBUG_LOG("UAECMD_LOAD_CONFIG done\n");
		break;
	    case UAECMD_SELECT_ROM:
		uae_sem_wait (&gui_sem);
		if (gui_romname) {
			strncpy (changed_prefs.romfile, gui_romname, 255);
			changed_prefs.romfile[255] = '\0';
			free (gui_romname);
			gui_romname=NULL;
		}
		if (gui_extname) {
			strncpy (changed_prefs.romextfile, gui_extname, 255);
			changed_prefs.romextfile[255] = '\0';
			free (gui_extname);
			gui_extname=NULL;
		}
		uae_sem_post (&gui_sem);
		break;
	    case UAECMD_SELECT_KEY:
		uae_sem_wait (&gui_sem);
		strncpy (changed_prefs.keyfile, gui_keyname, 255);
		changed_prefs.keyfile[255] = '\0';
		free (gui_keyname);
		uae_sem_post (&gui_sem);
		break;
	    case UAECMD_START:
		uae_start ();
		break;
	     case UAECMD_STOP:
		uae_stop ();
		break;
	}
    }
}

/*
 * gui_update()
 *
 * This is called from the main UAE thread to tell the GUI to update itself
 * using the current state of currprefs. This function will block
 * until it receives a message from the GUI telling it that the update
 * is complete.
 */
int gui_update (void) {
  DEBUG_LOG( "Entered\n" );

  if (gui_available) {
    write_comm_pipe_int (&to_gui_pipe, GUICMD_UPDATE, 1);
    uae_sem_wait (&gui_update_sem);
  }
  return 0;
}


/*
 * gui_exit()
 *
 * This called from the main UAE thread to tell the GUI to gracefully
 * quit. We don't need to do anything here for now. Our main() takes
 * care of putting the GUI to bed.
 */
void gui_exit (void)
{
}

/*
 * gui_shutdown()
 *
 * Tell the GUI thread it's time to say goodnight...
 */
#ifndef GTKMUI
static 
#endif
void gui_shutdown (void) {

  DEBUG_LOG( "entered\n" );

  if (gui_available) {
    if (!quit_gui) {
      close_splash();
      quit_gui = 1;

      DEBUG_LOG( "Waiting for GUI thread to quit.\n" );
      uae_sem_wait (&gui_quit_sem);
    }
  }

  DEBUG_LOG("left\n");
}

void gui_hd_led (int led)
{
    static int resetcounter;

    int old = gui_data.hd;
    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.hd = led;
    resetcounter = 6;
    if (old != gui_data.hd)
	gui_led (5, gui_data.hd);
}

void gui_cd_led (int led)
{
    static int resetcounter;

    int old = gui_data.cd;
    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.cd = led;
    resetcounter = 6;
    if (old != gui_data.cd)
	gui_led (6, gui_data.cd);
}

void gui_display (int shortcut)
{
    DEBUG_LOG ("called with shortcut=%d\n", shortcut);

    if (gui_available) {
	/* If running fullscreen, then we must try to switched to windowed
         * mode before activating the GUI */
	if (is_fullscreen ()) {
	    toggle_fullscreen ();
	    if (is_fullscreen ()) {
		write_log ("Cannot activate GUI in full-screen mode\n");
		return;
	    }
	}

	if (shortcut == -1)
	    write_comm_pipe_int (&to_gui_pipe, GUICMD_SHOW, 1);

	if (shortcut >=0 && shortcut <4) {
	    /* In this case, shortcut is the drive number to display
	     * the insert requester for */
	    write_comm_pipe_int (&to_gui_pipe, GUICMD_FLOPPYDLG, 0);
	    write_comm_pipe_int (&to_gui_pipe, shortcut, 1);
	}
    }
}

void gui_message_with_title (const char *title, const char *format,...)
{
    char msg[2048];
    va_list parms;

    va_start (parms,format);
    vsprintf ( msg, format, parms);
    va_end (parms);

    if (gui_available)
	do_message_box (title, msg, TRUE, TRUE);
}

void gui_message (const char *format,...) {
    char msg[2048];
    va_list parms;

    va_start (parms,format);
    vsprintf ( msg, format, parms);
    va_end (parms);

    if (gui_available)
	do_message_box (NULL, msg, TRUE, TRUE);

    write_log (msg);
}

void gui_notify_state (int state)
{
    if (gui_available) {
	write_comm_pipe_int (&to_gui_pipe, GUICMD_STATE_CHANGE, 1);
	write_comm_pipe_int (&to_gui_pipe, state, 1);
    }
}

/*
 * do_message_box()
 *
 * This makes up for GTK's lack of a function for creating simple message dialogs.
 * It can be called from any context. gui_init() must have been called at some point
 * previously.
 *
 * title   - will be displayed in the dialog's titlebar (or NULL for default)
 * message - the message itself
 * modal   - should the dialog block input to the rest of the GUI
 * wait    - should the dialog wait until the user has acknowledged it
 */
static void do_message_box (const gchar *title, const gchar *message, gboolean modal, gboolean wait )
{
    uae_sem_t msg_quit_sem;

    // If we a need reply, then this semaphore which will be used
    // to signal us when the dialog has been exited.
    uae_sem_init (&msg_quit_sem, 0, 0);

    write_comm_pipe_int   (&to_gui_pipe, GUICMD_MSGBOX, 0);
    write_comm_pipe_pvoid (&to_gui_pipe, (void *) title, 0);
    write_comm_pipe_pvoid (&to_gui_pipe, (void *) message, 0);
    write_comm_pipe_int   (&to_gui_pipe, (int) modal, 0);
    write_comm_pipe_pvoid (&to_gui_pipe, wait?&msg_quit_sem:NULL, 1);

    if (wait)
        uae_sem_wait (&msg_quit_sem);

    DEBUG_LOG ("do_message_box() done");
    return;
}

/*
 * handle_message_box_request()
 *
 * This is called from the GUI's context in repsonse to do_message_box()
 * to actually create the dialog box
 */
static void handle_message_box_request (smp_comm_pipe *msg_pipe)
{
    const gchar *title      = (const gchar *)  read_comm_pipe_pvoid_blocking (msg_pipe);
    const gchar *msg        = (const gchar *)  read_comm_pipe_pvoid_blocking (msg_pipe);
    int modal               =                  read_comm_pipe_int_blocking   (msg_pipe);
    uae_sem_t *msg_quit_sem = (uae_sem_t *)    read_comm_pipe_pvoid_blocking (msg_pipe);

    GtkWidget *dialog = make_message_box (title, msg, modal, msg_quit_sem);
}

/*
 * on_message_box_quit()
 *
 * Handler called when message box is exited. Signals anybody that cares
 * via the semaphore it is supplied.
 */
void on_message_box_quit (GtkWidget *w, gpointer user_data)
{
    uae_sem_post ((uae_sem_t *)user_data);
}

/*
 * make_message_box()
 *
 * This does the actual work of constructing the message dialog.
 *
 * title   - displayed in the dialog's titlebar
 * message - the message itself
 * modal   - whether the dialog should block input to the rest of the GUI
 * sem     - semaphore used for signalling that the dialog's finished
 *
 * TODO: Make that semaphore go away. We shouldn't need to know about it here.
 */
static GtkWidget *make_message_box (const gchar *title, const gchar *message, int modal, uae_sem_t *sem )
{
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *hseparator;
    GtkWidget *hbuttonbox;
    GtkWidget *button;
    guint      key;
    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new ();

    dialog = gtk_window_new ( GTK_WINDOW_TOPLEVEL /*GTK_WINDOW_DIALOG*/);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);
    if (title==NULL || (title!=NULL && strlen(title)==0))
        title = PACKAGE_NAME " information";
    gtk_window_set_title (GTK_WINDOW (dialog), title);
    gtk_window_set_modal (GTK_WINDOW (dialog), modal);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (dialog), vbox);

    label = gtk_label_new (message);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

    hseparator = gtk_hseparator_new ();
    gtk_widget_show (hseparator);
    gtk_box_pack_start (GTK_BOX (vbox), hseparator, FALSE, FALSE, 8);

#if !defined GTKMUI
    hbuttonbox = gtk_hbutton_box_new ();
#else
    hbuttonbox = gtk_hbox_new (FALSE,4);
#endif
    gtk_widget_show (hbuttonbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbuttonbox, FALSE, FALSE, 0);
#if !defined GTKMUI
    gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox), 4);
#endif

    button = make_labelled_button ("_Okay", accel_group);
    gtk_widget_show (button);
    gtk_container_add (GTK_CONTAINER (hbuttonbox), button);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    if (sem)
        gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (on_message_box_quit), sem);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			                           GTK_SIGNAL_FUNC (gtk_widget_destroy),
			                           GTK_OBJECT (dialog));

    gtk_widget_grab_default (button);
#if !defined GTKMUI
    gtk_window_add_accel_group (GTK_WINDOW (dialog), accel_group);
#endif
    gtk_widget_show( dialog );

    return dialog;
}

/*
 * gui_open ()
 *
 * Called by the main UAE thread during start up to display the GUI.
 */
int gui_open (void)
{
    int result = 0;

    DEBUG_LOG("Entered\n" );
    //DebOut("entered\n");

    if (!gui_available) {
	DEBUG_LOG( " !gui_available\n" );
	result = -1;
    }
    else {
	/* We have the technology and the will - so tell the GUI to
	 * reveal itself */
	//DebOut("GUICMD_SHOW\n");

	DEBUG_LOG( " write_comm_pipe_int(GUICMD_SHOW)\n" );
	write_comm_pipe_int (&to_gui_pipe, GUICMD_SHOW, 1);
    }
   return result;
}

void gui_init (int argc, char **argv)
{
    uae_thread_id tid;
    DEBUG_LOG ("entered\n");

    /* Check whether we are running with SUID bit set */
    if (getuid() == geteuid()) {
	/*
	 * Only try to start Gtk+ GUI if SUID bit is not set
	 */

	init_comm_pipe (&to_gui_pipe, 20, 1);
	init_comm_pipe (&from_gui_pipe, 20, 1);
	uae_sem_init (&gui_sem, 0, 1);          // Unlock mutex on prefs $
	uae_sem_init (&gui_update_sem, 0, 0);
	uae_sem_init (&gui_init_sem, 0, 0);
	uae_sem_init (&gui_quit_sem, 0, 0);

	/* Start GUI thread to construct GUI */
	uae_start_thread (gtk_gui_thread, NULL, &tid, "j-uae gui");

	/* Wait until GUI thread is ready */
	DEBUG_LOG ("Waiting for GUI thread\n");
	uae_sem_wait (&gui_init_sem);
	DEBUG_LOG ("Okay\n");
    }
}

BOOL gui_visible() {
	DEBUG_LOG("call gtk_mui_application_is_iconified..\n");
	return !gtk_mui_application_is_iconified();
}

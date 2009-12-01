/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-Daemon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-Daemon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************/
#ifndef __J_H__
#define __J_H__

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>

#ifdef __AROS__
#include <proto/arossupport.h>
#endif


#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
/* there are *two* memory.h files :( */
#include "../include/memory.h"
#include "custom.h"
#include "memory.h"
#include "newcpu.h"
#include "xwin.h"
#include "picasso96.h"
#include "traps.h"
#include "uae_endian.h"
#include "inputdevice.h"

#include "threaddep/thread.h"

#include <gtk/gtk.h>
#include "uae.h"

#define JWTRACING_ENABLED 1
#if JWTRACING_ENABLED
#define JWLOG(...)   do { kprintf("%s:%d  %s(): ",__FILE__,__LINE__,__func__);kprintf(__VA_ARGS__); } while(0)
#else
#define JWLOG(...)     do { ; } while(0)
#endif

#define JW_ENTER_ENABLED 0
#if JW_ENTER_ENABLED
#define ENTER  kprintf("%s %s(): entered\n",__FILE__,__func__);
#define LEAVE  kprintf("%s %s(): left at line %d\n",__FILE__,__func__,__LINE__);
#else
#define ENTER
#define LEAVE
#endif



WORD get_lo_word(ULONG *field);
WORD get_hi_word(ULONG *field);

#define TASK_PREFIX_NAME               "AOS3 Window "
#define SCREEN_TASK_PREFIX_NAME        "AOS3 Screen "
#define CUSTOM_SCREEN_TASK_PREFIX_NAME "AOS3 Custom Screen "

#define AD_SHUTDOWN    9

#define AD_SETUP    10
#define AD_GET_JOB  11

#define AD_TEST                        0 
#define AD_GET_JOB_WINDOWS             1
#define AD_GET_JOB_LIST_WINDOWS        2
#define AD_GET_JOB_MESSAGES            3
#define AD_GET_JOB_SYNC_WINDOWS        4
#define AD_GET_JOB_REPORT_UAE_WINDOWS  5
#define AD_GET_JOB_REPORT_HOST_WINDOWS 6
#define AD_GET_JOB_MARK_WINDOW_DEAD    7
#define AD_GET_JOB_GET_MOUSE           8
#define AD_GET_JOB_SWITCH_UAE_WINDOW   9
#define AD_GET_JOB_ACTIVE_WINDOW      10
#define AD_GET_JOB_NEW_WINDOW         11
#define AD_GET_JOB_LIST_SCREENS       12
#define AD_GET_JOB_OPEN_CUSTOM_SCREEN 13 /* not used ATM */
#define AD_GET_JOB_CLOSE_SCREEN       14 /* not used ATM */
#define AD_GET_JOB_TOP_SCREEN         15
#define AD_GET_JOB_DEBUG             999 

#define AD_CLIP_SETUP 15
#define AD_CLIP_JOB 16
#define AD_CLIP_TEST 0
#define JD_AMIGA_CHANGED       1
#define JD_AROS_CHANGED        2
#define JD_CLIP_COPY_TO_AROS   3
#define JD_CLIP_GET_AROS_LEN   4
#define JD_CLIP_COPY_FROM_AROS 5


#define J_MSG_CLOSE                    1

extern GSList *janus_windows;  /* List of JanusWins  */
extern GSList *janus_messages; /* List of JanusMsgs */

/* janusd */
extern ULONG aos3_task;
extern ULONG aos3_task_signal;
extern struct SignalSemaphore aos3_sem;
extern struct SignalSemaphore sem_janus_window_list;
extern struct SignalSemaphore sem_janus_screen_list;
extern struct SignalSemaphore aos3_thread_start;
extern struct SignalSemaphore janus_messages_access;
extern struct SignalSemaphore sem_janus_active_win;
extern struct SignalSemaphore sem_janus_active_custom_screen;

/* clipd */
extern ULONG aos3_clip_task;
extern ULONG aos3_clip_signal;
extern ULONG aos3_clip_to_amigaos_signal;

/* clipboard status */
extern BOOL clipboard_amiga_changed;
extern BOOL clipboard_aros_changed;

void clipboard_hook_install          (void);
void copy_clipboard_to_aros          (void);
void copy_clipboard_to_aros_real     (uaecptr data, uae_u32 len);
void copy_clipboard_to_amigaos       (void);
void copy_clipboard_to_amigaos_real  (uaecptr data, uae_u32 len);
ULONG aros_clipboard_len             (void);

/* test, if a aos3screen is a custom screen */
BOOL aos3screen_is_custom            (uaecptr aos3screen);

/* main uae window active ? */
extern BOOL uae_main_window_closed;

extern WORD menux, menuy;

extern struct Window   *original_W;
extern struct Screen   *original_S;
extern struct ColorMap *original_CM;
extern struct RastPort *original_RP;

#define WIN_DEFAULT_DELAY 50

#define MAXWIDTHHEIGHT 0x7777

typedef struct {
  gpointer             aos3screen;
  struct Screen       *arosscreen;
  char                *title;
  char                *pubname;
  char                *name;    /* name of process */
  struct Task         *task;
  ULONG                depth;
  UWORD                maxwidth;  /* maximal overscan width  */
  UWORD                maxheight; /* maximal overscan height */
  BOOL                 ownscreen;
} JanusScreen;

typedef struct {
  gpointer       aos3win; /* gpointer !? */
  struct Window *aroswin; /* corresponding native aros win */
  JanusScreen   *jscreen; /* screens on which window is running */
  void          *mempool; /* alloc everything here */
  struct Task   *task;
  char          *name;
  struct Menu   *arosmenu;
  ULONG          nr_menu_items;
  void          *newmenu_mem;
  LONG           delay;         /* if > 0, delay-- and leave it untouched */
  WORD           LeftEdge, TopEdge;
  WORD           Width, Height;
  WORD           plusx, plusy;  /* increase visible size */
  ULONG          secs, micros;  /* remember last menupick for DMRequests */
  BOOL           dead;
  BOOL           custom;        /* window is on a custom screen */
} JanusWin;

typedef struct {
  JanusWin      *jwin;
  ULONG          type; /* J_MSG_..*/
  BOOL           old;
} JanusMsg;

/* Values for amiga_screen_type */
enum {
  UAESCREENTYPE_CUSTOM,
  UAESCREENTYPE_PUBLIC,
  UAESCREENTYPE_ASK,
  UAESCREENTYPE_LAST
};

/* one of our custom screens is active */
extern JanusScreen *janus_active_screen;

/* remove me */
extern struct Window *native_window;
extern APTR m68k_win;

/* active janus window or NULL */
extern JanusWin *janus_active_window;
extern GSList *janus_windows;
extern GSList *janus_screens;

extern BOOL j_stop_window_update;

/* borrowed from input.device */
extern struct uae_input_device2 mice2[MAX_INPUT_DEVICES];
extern struct uae_input_device *mice;

/* borrowed from ami-win.c */
extern struct RastPort  *RP;
extern struct Screen    *S;
extern struct Window    *W;
extern struct ColorMap  *CM;
extern int    XOffset,YOffset;

uae_u32 REGPARAM2 aroshack_helper (TrapContext *context);

int aros_daemon_runing(void);

void o1i_clone_windows_task(void);
void o1i_clone_windows(void);

void clone_area(WORD x, WORD y, UWORD width, UWORD height);
BOOL clone_window_area(JanusWin *jwin, 
                       WORD areax, WORD areay, 
		       UWORD areawidth, UWORD areaheight);

UWORD get_LeftEdge(ULONG m68kwin);
UWORD get_TopEdge(ULONG m68kwin);
UWORD get_Width(ULONG m68kwin);
UWORD get_Height(ULONG m68kwin);
UWORD get_Flags(ULONG m68kwin);
UWORD get_BorderLeft(ULONG m68kwin);
UWORD get_BorderTop(ULONG m68kwin);
UWORD get_BorderRight(ULONG m68kwin);
UWORD get_BorderBottom(ULONG m68kwin);

/* ad_jobs */
uae_u32 ad_job_new_window          (ULONG aos3win);
uae_u32 ad_job_mark_window_dead    (ULONG aos3win);
uae_u32 ad_job_active_window       (ULONG *m68k_results);
uae_u32 ad_job_list_screens        (ULONG *m68k_results);
uae_u32 ad_job_report_host_windows (ULONG *m68k_results);
uae_u32 ad_job_report_uae_windows  (ULONG *m68k_results);
uae_u32 ad_job_switch_uae_window   (ULONG *m68k_results);
uae_u32 ad_job_sync_windows        (ULONG *m68k_results);
uae_u32 ad_job_update_janus_windows(ULONG *m68k_results);
uae_u32 ad_job_open_custom_screen  (ULONG aos3screen);
uae_u32 ad_job_get_mouse           (ULONG *m68k_results);
uae_u32 ad_debug                   (ULONG *m68k_results);

/* compare hooks */
gint aos3_process_compare       (gconstpointer aos3win, gconstpointer t);
gint aos3_window_compare        (gconstpointer aos3win, gconstpointer w);
gint aros_window_compare        (gconstpointer aos3win, gconstpointer w);
gint aos3_screen_compare        (gconstpointer jscreen, gconstpointer s);
gint aos3_screen_process_compare(gconstpointer jscreen, gconstpointer t);

/* threads */
int aros_win_start_thread           (JanusWin *win);
int aros_screen_start_thread        (JanusScreen *screen);
int aros_custom_screen_start_thread (JanusScreen *screen);

/* menu */
void clone_menu(JanusWin *jwin);
void click_menu(JanusWin *jwin, WORD menu, WORD item, WORD sub);
void process_menu(JanusWin *jwin, UWORD selection);

/* assert */
struct Window *assert_window (struct Window *search);

/* casting helpers */
void  put_long_p(ULONG *p, ULONG value);
ULONG get_long_p(ULONG *p);

void close_all_janus_windows(void);

/* e-uae stuff */
int match_hotkey_sequence(int key, int state);
void inputdevice_do_keyboard(int code, int state);
void setmousebuttonstate(int mouse, int button, int state);
void inputdevice_acquire(void);
void inputdevice_release_all_keys(void);
void reset_hotkeys(void);
void inputdevice_unacquire(void);
ULONG find_rtg_mode (ULONG *width, ULONG *height, ULONG depth);
void hide_pointer (struct Window *w);
void reset_drawing(void);

STATIC_INLINE uae_u32 get_byte(uaecptr addr);

/* ami-win.c */
void handle_events_W(struct Window *W, BOOL customscreen);

#endif

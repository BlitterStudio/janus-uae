
#ifndef __AROS_WIN_H_
#define __AROS_WIN_H_

#ifdef __AROS__
#include <proto/arossupport.h>
#endif

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "picasso96.h"
#include "traps.h"
#include "uae_endian.h"

#include <gtk/gtk.h>
#include "uae.h"

#define TASK_PREFIX_NAME "AOS3 Window "

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

extern GSList *janus_windows;  /* List of JanusWins  */
extern GSList *janus_messages; /* List of JanusMsgs */

/* we need that, to wake up the aos3 high pri daemon */
extern ULONG aos3_task;
extern ULONG aos3_task_signal;
extern struct SignalSemaphore aos3_sem;
extern struct SignalSemaphore sem_janus_window_list;
extern struct SignalSemaphore sem_janus_screen_list;
extern struct SignalSemaphore aos3_thread_start;
extern struct SignalSemaphore janus_messages_access;

/* main uae window active ? */
extern BOOL uae_main_window_closed;

#define WIN_DEFAULT_DELAY 20

#define MAXWIDTHHEIGHT 0x7777

typedef struct {
  gpointer             aos3screen;
  struct Screen       *arosscreen;
  char                *title;
  char                *pubname;
  ULONG                depth;
  BOOL                 ownscreen;
} JanusScreen;

typedef struct {
  gpointer       aos3win;
  struct Window *aroswin;
  JanusScreen   *jscreen; /* screens on which window is running */
  struct Task   *task;
  char          *name;
  LONG           delay; /* if > 0, delay-- and leave it untouched */
  WORD           LeftEdge, TopEdge;
  WORD           Width, Height;
  WORD           plusx, plusy; /* increase visible size */
  BOOL           dead;
} JanusWin;

typedef struct {
  gpointer             aos3win;
  gpointer             aos3msg;
  struct Window       *aroswin;
  struct IntuiMessage *arosmsg;
} JanusMsg;

/* remove me */
extern struct Window *native_window;
extern APTR m68k_win;

/* active janus window or NULL */
extern JanusWin *janus_active_window;
extern GSList *janus_windows;
extern GSList *janus_screens;

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


/* e-uae stuff */
int match_hotkey_sequence(int key, int state);
void inputdevice_do_keyboard(int code, int state);
void setmousebuttonstate(int mouse, int button, int state);
void inputdevice_acquire(void);
void inputdevice_release_all_keys(void);
void reset_hotkeys(void);
void inputdevice_unacquire(void);
ULONG find_rtg_mode (ULONG *width, ULONG *height, ULONG depth);

#endif

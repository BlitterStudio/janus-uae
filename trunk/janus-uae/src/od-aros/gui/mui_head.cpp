#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include <utility/hooks.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "threaddep/thread.h"
#include "aros.h"
#include "gui_mui.h"
#include "mui_data.h"

Object *app, *win; 
static Object *root, *leftframe, *pages;
static Object *start, *cancel, *help, *errorlog, *reset, *quit;

/* GUI thread */
//static uae_thread_id tid;

static int button_ret;

#if 0
Element IDD_FLOPPY[] = {
  { 0, NULL, GROUPBOX,  1,   0, 393, 163, "bla", 0 },
  { 0, NULL, CONTROL,   7,  14,  34,  15, "DF0:", 0 },
  { 0, NULL, GROUPBOX,  1, 170, 393,  35, "foo", 0 },
  { 0, NULL, 0,         0,   0,   0,   0,  NULL, 0 }
};
#endif

#define IDS_KICKSTART           "ROM"
#define IDS_DISK                "Disk swapper"
#define IDS_DISPLAY             "Display"
#define IDS_HARDDISK            "CD & Hard drives"
#define IDS_FLOPPY              "Floppy drives"
#define IDS_ABOUT               "About"
#define IDS_LOADSAVE            "\33bConfigurations"
#define IDS_LOADSAVE1           "\33bHardware"
#define IDS_LOADSAVE2           "\33bHost"
#define IDS_AVIOUTPUT           "Output"
#define IDS_IOPORTS             "IO ports"
#define IDS_MISC1               "Miscellaneous"
#define IDS_MEMORY              "RAM"
#define IDS_CPU                 "CPU and FPU"
#define IDS_CHIPSET             "Chipset"
#define IDS_INPUT               "Input"
#define IDS_FILTER              "Filter"
#define IDS_MISC2               "Pri. & Extensions"
#define IDS_PATHS               "Paths"
#define IDS_QUICKSTART          "Quickstart"
#define IDS_FRONTEND            "Frontend"
#define IDS_CHIPSET2            "Adv. Chipset"
#define IDS_GAMEPORTS           "Game ports"
#define IDS_EXPANSION           "Expansions"
#define IDS_CDROM               "CD-ROM"
#define IDS_SOUND               "Sound"


static const char *listelements[] = {
  IDS_LOADSAVE,
  IDS_ABOUT,
  IDS_PATHS,
  IDS_QUICKSTART,
  IDS_LOADSAVE1,
  IDS_CPU,
  IDS_CHIPSET,
  IDS_CHIPSET2,
  IDS_KICKSTART,
  IDS_MEMORY,
  IDS_FLOPPY,
  IDS_HARDDISK,
  IDS_EXPANSION,
  IDS_LOADSAVE2,
  IDS_DISPLAY,
  IDS_SOUND,
  IDS_GAMEPORTS,
  IDS_IOPORTS,
  IDS_INPUT,
  IDS_AVIOUTPUT,
  IDS_FILTER,
  IDS_DISK,
  IDS_MISC1,
  IDS_MISC2,
  NULL
};

extern int currentpage;

struct Hook MyMuiHook_list;

AROS_UFH2(void, MUIHook_list, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  ULONG nr;

  nr=XGET((Object *) obj, MUIA_List_Active);

  /* we have to skip those configuration load/save pages here */
  if(nr==0 || nr==4 || nr==13) {
    DebOut("TODO: tree collapse? Try to load config here..?\n");
    currentpage=0;
    nr=0;
  }
  else if(nr<4) {
    currentpage=nr-1; /* skip "Settings" tab */
  }
  else if(nr<13) {
    currentpage=nr-2; /* skip "Settings" and "Hardware" tabs */
  }
  else {
    currentpage=nr-3; /* skip "Settings", "Hardware" and "Host" tabs */
  }

OK:
  DebOut("currentpage is now: %d\n", currentpage);
  DoMethod(pages, MUIM_Set, MUIA_Group_ActivePage, nr);

  AROS_USERFUNC_EXIT
}

struct Hook MyMuiHook_start;

AROS_UFH2(void, MUIHook_start, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  DebOut("START!\n");

  /* 0 = normal, 1 = nogui, -1 = disable nogui */
  aros_hide_gui();
  quit_program=0;
  uae_restart (-1, NULL);

  AROS_USERFUNC_EXIT
}

struct Hook MyMuiHook_reset;

AROS_UFH2(void, MUIHook_reset, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {
  AROS_USERFUNC_INIT

  DebOut("RESET!\n");

  uae_reset (1, 1);

  AROS_USERFUNC_EXIT
}

struct Hook MyMuiHook_quit;

AROS_UFH2(void, MUIHook_quit, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  DebOut("QUIT!\n");

  button_ret=0;
  uae_quit ();
  DebOut("send SIGBREAKF_CTRL_C to ourselves.. is this a good idea?\n");
  Signal(FindTask(NULL), SIGBREAKF_CTRL_C);
 
  AROS_USERFUNC_EXIT
}

int *FloppyDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *KickstartDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *CPUDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *AboutDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *PathsDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

Object* FixedObj(IPTR src)
{
    struct TagItem fo_tags[] =
    {
        { MA_src, src},
        { TAG_DONE, 0}
    };

    return (Object*) NewObjectA(CL_Fixed->mcc_Class, NULL, fo_tags);
}

Object* FixedProcObj(IPTR src, IPTR proc)
{
    struct TagItem fo_tags[] =
    {
        { MA_src, src}, 
        { MA_dlgproc, proc},
        { TAG_DONE, 0}
    };

    return (Object*) NewObjectA(CL_Fixed->mcc_Class, NULL, fo_tags);
}

Object* build_gui(void) {
  int i=0;

  DebOut("src: %lx\n", IDD_FLOPPY);

  if(app) {
    DebOut("build_gui was already called before, do nothing!\n");
    return app;
  }

#if 0
  while(IDD_FLOPPY[i].windows_type) {
    DebOut("i: %d\n", i);
    DebOut("IDD_FLOPPY[%d].windows_type: %d\n", i, IDD_FLOPPY[i].windows_type);
    DebOut("IDD_FLOPPY[%d].x: %d\n", i, IDD_FLOPPY[i].x);
    DebOut("IDD_FLOPPY[%d].text: %s\n", i, IDD_FLOPPY[i].text);
    i++;
  }

  DebOut("send: %lx\n", &IDD_FLOPPY);
#endif

  app = MUI_NewObject(MUIC_Application,
                      MUIA_Application_Author, (IPTR) "Oliver Brunner",
                      MUIA_Application_Base, (IPTR)   "AROSUAE",
                      MUIA_Application_Copyright, (IPTR)"(c) 2014 Oliver Brunner",
                      MUIA_Application_Description, (IPTR)"WinUAE GUI converted to Zune",
                      MUIA_Application_Title, (IPTR)"WinUAE",
                      MUIA_Application_Version, (IPTR)"$VER: WinUAE 0.2 (x.x.2014)",
                      MUIA_Application_Window, (IPTR)(win = MUI_NewObject(MUIC_Window,
                        MUIA_Window_Title, (IPTR)"WinUAE Properties",
                        MUIA_Window_RootObject, root=MUI_NewObject(MUIC_Group,
                          MUIA_Group_Horiz, TRUE,
                          MUIA_Group_Child,
                            /* left group with selector and reset/quit buttons begin */
                            MUI_NewObject(MUIC_Group,
                              MUIA_Group_Horiz, FALSE,
                              MUIA_Group_Child,
                                ListviewObject,
                                          MUIA_Listview_List, leftframe=ListObject,
                                            MUIA_Frame, MUIV_Frame_ReadList,
                                            MUIA_List_SourceArray, (IPTR) listelements,
                                            MUIA_Background, "2:FFFFFFFF,FFFFFFFF,FFFFFFFF",
                                            MUIA_Font, MUIV_Font_List, 
                                          End,
                                        End,
                              /* reset /quit buttons! */
                              MUIA_Group_Child,
                                MUI_NewObject(MUIC_Group,
                                  MUIA_Group_Horiz, TRUE,
                                  MUIA_Group_Child,
                                    reset=MUI_MakeObject(MUIO_Button,"Reset"),
                                  MUIA_Group_Child,
                                    quit=MUI_MakeObject(MUIO_Button,"Quit"),
                                TAG_DONE),
                            TAG_DONE),
                            /* left group with selector and reset/quit buttons end */
                          MUIA_Group_Child, 
                            MUI_NewObject(MUIC_Group,
                              MUIA_Group_Horiz, FALSE,
                              MUIA_Group_Child,
                                pages=PageGroup,
                                  Child, FixedObj((IPTR)IDD_LOADSAVE),
                                  Child, FixedProcObj((IPTR)IDD_ABOUT, (IPTR) &AboutDlgProc),
                                  Child, FixedProcObj((IPTR)IDD_PATHS, (IPTR) &PathsDlgProc),
                                  Child, FixedObj((IPTR)IDD_QUICKSTART),
                                  Child, FixedObj((IPTR)IDD_LOADSAVE),
                                  Child, FixedProcObj((IPTR)IDD_CPU, (IPTR) &CPUDlgProc),
                                  Child, FixedObj((IPTR)IDD_CHIPSET),
                                  Child, FixedObj((IPTR)IDD_CHIPSET2),
                                  Child, FixedProcObj((IPTR)IDD_KICKSTART, (IPTR) &KickstartDlgProc),
                                  Child, FixedObj((IPTR)IDD_MEMORY),
                                  Child, FixedProcObj((IPTR)IDD_FLOPPY, (IPTR) &FloppyDlgProc),
                                  Child, FixedObj((IPTR)IDD_CDDRIVE),
                                  Child, FixedObj((IPTR)IDD_EXPANSION),
                                  Child, FixedObj((IPTR)IDD_LOADSAVE),
                                  Child, FixedObj((IPTR)IDD_DISPLAY),
                                  Child, FixedObj((IPTR)IDD_SOUND),
                                  Child, FixedObj((IPTR)IDD_GAMEPORTS),
                                  Child, FixedObj((IPTR)IDD_IOPORTS),
                                  Child, FixedObj((IPTR)IDD_INPUT),
                                  Child, FixedObj((IPTR)IDD_AVIOUTPUT),
                                  Child, FixedObj((IPTR)IDD_FILTER),
                                  Child, FixedObj((IPTR)IDD_DISK),
                                  Child, FixedObj((IPTR)IDD_MISC1),
                                  Child, FixedObj((IPTR)IDD_MISC2),
                                End,
                              MUIA_Group_Child,
                                MUI_NewObject(MUIC_Group,
                                  MUIA_Group_Horiz, TRUE,
                                  MUIA_Group_Child,
                                    HSpace(0),
                                  MUIA_Group_Child,
                                    HSpace(0),

                                  MUIA_Group_Child,
                                    errorlog=MUI_MakeObject(MUIO_Button,"Error log"),
                                  MUIA_Group_Child,
                                    start=MUI_MakeObject(MUIO_Button,"Start"),
                                  MUIA_Group_Child,
                                    cancel=MUI_MakeObject(MUIO_Button,"Cancel"),
                                  MUIA_Group_Child,
                                    help=MUI_MakeObject(MUIO_Button,"Help"),
                                TAG_DONE),

                              TAG_DONE),
                      TAG_END),
                    TAG_END)),
        TAG_END);


  /* List click events */
  kprintf("leftframe: %lx\n", leftframe);

#ifdef UAE_ABI_v0
  MyMuiHook_list.h_Entry=(HOOKFUNC) MUIHook_list;
#else
  MyMuiHook_list.h_Entry=(APTR) MUIHook_list;
#endif
  MyMuiHook_list.h_Data =NULL;
  DoMethod(leftframe, MUIM_Notify,
                        MUIA_List_Active, MUIV_EveryTime, 
                        (IPTR) leftframe, 2, MUIM_CallHook,(IPTR) &MyMuiHook_list);

  /* button events */
#ifdef UAE_ABI_v0
  MyMuiHook_start.h_Entry=(HOOKFUNC) MUIHook_start;
#else
  MyMuiHook_start.h_Entry=(APTR) MUIHook_start;
#endif
  MyMuiHook_start.h_Data =NULL;
  DoMethod(start, MUIM_Notify, MUIA_Pressed, FALSE, start, 3, MUIM_CallHook, (IPTR) &MyMuiHook_start, TRUE);

#ifdef UAE_ABI_v0
  MyMuiHook_reset.h_Entry=(HOOKFUNC) MUIHook_reset;
#else
  MyMuiHook_reset.h_Entry=(APTR) MUIHook_reset;
#endif
  MyMuiHook_reset.h_Data =NULL;
  DoMethod(reset, MUIM_Notify, MUIA_Pressed, FALSE, reset, 3, MUIM_CallHook, (IPTR) &MyMuiHook_reset, TRUE);

#ifdef UAE_ABI_v0
  MyMuiHook_quit.h_Entry=(HOOKFUNC) MUIHook_quit;
#else
  MyMuiHook_quit.h_Entry=(APTR) MUIHook_quit;
#endif
  MyMuiHook_quit.h_Data =NULL;
  DoMethod(quit,  MUIM_Notify, MUIA_Pressed, FALSE, quit , 3, MUIM_CallHook, (IPTR) &MyMuiHook_quit , TRUE);


  return app;
}

void aros_main_loop(void) {

  ULONG signals = 0;

  DebOut("entered..\n");


  while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
  {
    signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_F);
    if (signals & SIGBREAKF_CTRL_C) break;
    if (signals & SIGBREAKF_CTRL_D) break;
    if (signals & SIGBREAKF_CTRL_F) {
      DebOut("SIGBREAKF_CTRL_F received!\n");
    }
  }

  DebOut("aros_main_loop left\n");
}

/*
 * launch gui without own thread 
 *
 * safe to be called more than once!
 *
 */
#if 0
void launch_gui(void) {

  init_class();
  app = build_gui();

  if(!app) {
    fprintf(stderr, "Unable to initialize GUI!\n");
    exit(1);
  }

  /* We're ready - tell the world */
  DebOut("call aros_main_loop ..\n");
  aros_main_loop();

  DebOut("GUI thread dies!\n");
}

/*
 * gui_thread()
 *
 * This is launched as a separate thread to the main UAE thread
 * to create and handle the GUI. After the GUI has been set up,
 * this calls the standard event processing loop.
 *
 */
static void *gui_thread (void *dummy) {
  DebOut("gui_thread started!\n");


  if(app) {
    DebOut("ERROR: aros_init_gui was already called before!?\n");
    return NULL;
  }
  init_class();
  app = build_gui();

  if(!app) {
    fprintf(stderr, "Unable to initialize GUI!\n");
    exit(1);
  }

  /* We're ready - tell the world */
  DebOut("call aros_main_loop ..\n");
  aros_main_loop();

  DebOut("GUI thread dies!\n");
}
#endif

int aros_init_gui(void) {
  ULONG w;

  DebOut("entered\n");

#if 0
  DebOut("start gui_thread..\n");
  //uae_start_thread (gui_thread, NULL, &tid, "ArosUAE GUI thread");
  uae_start_thread ("ArosUAE GUI thread", gui_thread, NULL, &tid);

  DebOut("waiting for GUI to come up..\n");
  w=15;
  while(w) {
    DebOut("Wait until window is really open .. #%02d\n", w);
    if(win && XGET(win, MUIA_Window_Open)) {
      break;
    }
    sleep(1);
    w--;
  }


  DebOut("GUI is up!\n");
#endif

  if(!app) {
    DebOut("aros_init_gui called for the first time!\n");
    init_class();
    app = build_gui();
  }

  if(!app) {
    fprintf(stderr, "ERROR: Unable to initialize GUI!\n");
    return 0;
  }

  return 1;
}

int aros_show_gui(void) {

  DebOut("entered\n");

  if(!app) {
    DebOut("aros_show_gui called for the first time!\n");
    init_class();
    app = build_gui();
  }

  if(!app) {
    fprintf(stderr, "ERROR: Unable to initialize GUI!\n");
    return -1;
  }

  /* activate current page */
  DebOut("currentpage: %d\n", currentpage);
  DoMethod(leftframe, MUIM_Set, MUIA_List_Active, currentpage);

  DoMethod(win, MUIM_Set, MUIA_Window_Open, TRUE);

  aros_main_loop();

  DoMethod(win, MUIM_Set, MUIA_Window_Open, FALSE);

  DebOut("left\n");
  return 0;
}

void aros_hide_gui(void) {

  DebOut("Send hide signal\n");

  Signal(FindTask(NULL), SIGBREAKF_CTRL_D);
}

void aros_gui_exit(void) {

  DebOut("entered\n");

  if(app) {
    MUI_DisposeObject(app);
    app=NULL;
    delete_class();
  }
}

#ifdef __STANDALONE__
/* not used in AROS */
int main(void) {
  aros_init_gui();
  //aros_main_loop();
  aros_gui_exit();

  return 0;
}
#endif

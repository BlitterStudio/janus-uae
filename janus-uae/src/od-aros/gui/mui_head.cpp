#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "threaddep/thread.h"
#include "aros.h"
#include "gui.h"
#include "mui_data.h"

static Object *app, *win, *root, *leftframe, *pages;
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
#define IDS_LOADSAVE            "Configurations"
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
  IDS_ABOUT,
  IDS_PATHS,
  IDS_QUICKSTART,
  IDS_LOADSAVE,
  IDS_CPU,
  IDS_CHIPSET,
  IDS_CHIPSET2,
  IDS_KICKSTART,
  IDS_MEMORY,
  IDS_FLOPPY,
  IDS_HARDDISK,
  IDS_EXPANSION,
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

HOOKPROTO(MUIHook_list, ULONG, obj, hookpointer)
{
  AROS_USERFUNC_INIT

  ULONG nr;

  nr=xget(obj, MUIA_List_Active);

  DebOut("activate nr %d\n", nr);

  DoMethod(pages, MUIM_Set, MUIA_Group_ActivePage, nr);

  return 0;
  AROS_USERFUNC_EXIT
}
MakeHook(MyMuiHook_list, MUIHook_list);

HOOKPROTO(MUIHook_start, ULONG, obj, hookpointer)
{
  AROS_USERFUNC_INIT

  DebOut("START!\n");

  /* 0 = normal, 1 = nogui, -1 = disable nogui */
  aros_hide_gui();
  quit_program=0;
  uae_restart (-1, NULL);

  return 0;
  AROS_USERFUNC_EXIT
}
MakeHook(MyMuiHook_start, MUIHook_start);

HOOKPROTO(MUIHook_reset, ULONG, obj, hookpointer)
{
  AROS_USERFUNC_INIT

  DebOut("RESET!\n");

  uae_reset (1, 1);

  return 0;
  AROS_USERFUNC_EXIT
}
MakeHook(MyMuiHook_reset, MUIHook_reset);

HOOKPROTO(MUIHook_quit, ULONG, obj, hookpointer)
{
  AROS_USERFUNC_INIT

  DebOut("QUIT!\n");

  button_ret=0;
  uae_quit ();
  DebOut("send SIGBREAKF_CTRL_C to ourselves.. is this a good idea?\n");
  Signal(FindTask(NULL), SIGBREAKF_CTRL_C);

  return 0;
  AROS_USERFUNC_EXIT
}
MakeHook(MyMuiHook_quit, MUIHook_quit);



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
                      MUIA_Application_Author, (ULONG) "Oliver Brunner",
                      MUIA_Application_Base, (ULONG)   "AROSUAE",
                      MUIA_Application_Copyright, (ULONG)"(c) 2014 Oliver Brunner",
                      MUIA_Application_Description, (ULONG)"WinUAE GUI converted to Zune",
                      MUIA_Application_Title, (ULONG)"WinUAE",
                      MUIA_Application_Version, (ULONG)"$VER: WinUAE 0.2 (x.x.2014)",
                      MUIA_Application_Window, (ULONG)(win = MUI_NewObject(MUIC_Window,
                        MUIA_Window_Title, (ULONG)"WinUAE Properties",
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
                                            MUIA_List_SourceArray, (ULONG) listelements,
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
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_ABOUT, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_PATHS, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_QUICKSTART, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_LOADSAVE, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_CPU, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_CHIPSET, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_CHIPSET2, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_KICKSTART, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_MEMORY, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_FLOPPY, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_CDDRIVE, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_EXPANSION, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_DISPLAY, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_SOUND, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_GAMEPORTS, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_IOPORTS, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_INPUT, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_AVIOUTPUT, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_FILTER, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_DISK, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_MISC1, TAG_DONE),
                                  Child, NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_MISC2, TAG_DONE),
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
  DoMethod(leftframe, MUIM_Notify,
                        MUIA_List_Active, MUIV_EveryTime, 
                        (ULONG) leftframe, 2, MUIM_CallHook,(ULONG) &MyMuiHook_list);

  /* button events */
  DoMethod(start, MUIM_Notify, MUIA_Pressed, FALSE, start, 3, MUIM_CallHook, (ULONG) &MyMuiHook_start, TRUE);
  DoMethod(reset, MUIM_Notify, MUIA_Pressed, FALSE, reset, 3, MUIM_CallHook, (ULONG) &MyMuiHook_reset, TRUE);
  DoMethod(quit,  MUIM_Notify, MUIA_Pressed, FALSE, quit , 3, MUIM_CallHook, (ULONG) &MyMuiHook_quit , TRUE);


  return app;
}

void aros_main_loop(void) {

  ULONG signals = 0;

  DebOut("entered..\n");


  while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
  {
    signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
    if (signals & SIGBREAKF_CTRL_C) break;
    if (signals & SIGBREAKF_CTRL_D) break;
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
    if(win && xget(win, MUIA_Window_Open)) {
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


  SetAttrs(win, MUIA_Window_Open, TRUE, TAG_DONE);

  aros_main_loop();

  SetAttrs(win, MUIA_Window_Open, FALSE, TAG_DONE);

  DebOut("left\n");
  return 0;
}

void aros_hide_gui(void) {

  DebOut("Send hide signal\n");

  Signal(FindTask(NULL), SIGBREAKF_CTRL_D);
}

void aros_gui_exit(void) {

  DebOut("entered\n");

  MUI_DisposeObject(app);
  app=NULL;
  delete_class();
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

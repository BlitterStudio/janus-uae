//#define JUAE_DEBUG

#include "sysdeps.h"
#include "sysconfig.h"

#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include <utility/hooks.h>

#include "uae.h"
#include "thread.h"
#include "aros.h"
#include "gui_mui.h"
#include "mui_data.h"

Object *app, *win; 
Object *root, *leftframe, *pages;
static Object *start, *cancel, *help, *errorlog, *quit;
Object *reset;

struct my_list_element {
  ULONG id;
  char *icon_name;
};

#define K "\33I[3:PROGDIR:images/k.png]"  /* |- */
#define L "\33I[3:PROGDIR:images/l.png]"  /* |  */
#define I "\33I[3:PROGDIR:images/i.png]"  /* '- */
#define E "\33I[3:PROGDIR:images/e.png]"  /*    */

/* lazy coded WinUAE lookalike fake tree */
static const struct my_list_element listelements[] = {
  IDS_ABOUT,              K "\33I[3:PROGDIR:images/amigainfo.png]",
  IDS_PATHS,              K "\33I[3:PROGDIR:images/paths.png]",
  IDS_QUICKSTART,         K "\33I[3:PROGDIR:images/quickstart.png]",
  IDS_LOADSAVE,           K "\33I[3:PROGDIR:images/file.png]",
  IDS_TREEVIEW_HARDWARE,  K "\33I[3:PROGDIR:images/root.png]",
  IDS_CPU,                I K "\33I[3:PROGDIR:images/cpu.png]",
  IDS_CHIPSET,            I K "\33I[3:PROGDIR:images/cpu.png]",
  IDS_CHIPSET2,           I K "\33I[3:PROGDIR:images/cpu.png]",
  IDS_KICKSTART,          I K "\33I[3:PROGDIR:images/chip.png]",
  IDS_MEMORY,             I K "\33I[3:PROGDIR:images/chip.png]",
  IDS_FLOPPY,             I K "\33I[3:PROGDIR:images/floppy35.png]",
  IDS_HARDDISK,           I K "\33I[3:PROGDIR:images/drive.png]",
  IDS_EXPANSION2,         I K "\33I[3:PROGDIR:images/expansion.png]",
  IDS_EXPANSION,          I L "\33I[3:PROGDIR:images/expansion.png]",
  IDS_TREEVIEW_HOST,      L "\33I[3:PROGDIR:images/root.png]",
  IDS_DISPLAY,            E K "\33I[3:PROGDIR:images/screen.png]",
  IDS_SOUND,              E K "\33I[3:PROGDIR:images/sound.png]",
  IDS_GAMEPORTS,          E K "\33I[3:PROGDIR:images/joystick.png]",
  IDS_IOPORTS,            E K "\33I[3:PROGDIR:images/port.png]",
  IDS_INPUT,              E K "\33I[3:PROGDIR:images/joystick.png]",
  IDS_AVIOUTPUT,          E K "\33I[3:PROGDIR:images/avioutput.png]",
  IDS_FILTER,             E K "\33I[3:PROGDIR:images/screen.png]",
  IDS_DISK,               E K "\33I[3:PROGDIR:images/floppy35.png]",
  IDS_MISC1,              E K "\33I[3:PROGDIR:images/misc.png]",
  IDS_MISC2,              E L "\33I[3:PROGDIR:images/misc.png]",
  0, NULL
};

static Object *page_obj[50];

extern int currentpage;

struct Hook MyMuiHook_list;

void check_currentpage(int shouldbe) {

  if(currentpage != shouldbe) {
    DebOut("==> ERROR <==   currentpage %d != shouldbe %d\n", currentpage, shouldbe);
  }
}

AROS_UFH2(void, MUIHook_list, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  ULONG nr;
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
  Element *elem;

  nr=XGET((Object *) obj, MUIA_List_Active);

  DebOut("MUIHook_list entered\n");

  {
    struct Task* task=FindTask(NULL);
    DebOut("Task %p, stack lower %p, stack upper %p\n", task, task->tc_SPLower, task->tc_SPUpper);
  }

#if 0
  /* we have to skip those configuration load/save pages here */
  if(nr==0 || nr==4 || nr==13) {
    DebOut("TODO: tree collapse? Try to load config here..?\n");
    currentpage=0;
    nr=0;
  }
  else if(nr<4) {
    currentpage=nr; /* skip "Settings" tab */
  }
  else if(nr<13) {
    currentpage=nr-2; /* skip "Settings" and "Hardware" tabs */
  }
  else {
    currentpage=nr-3; /* skip "Settings", "Hardware" and "Host" tabs */
  }
#endif
  currentpage=nr;

  if(page_obj[nr]) {
    func=(int* (*)(Element*, uint32_t, ULONG, IPTR)) XGET(page_obj[nr], MA_dlgproc);
    elem=(Element *) XGET(page_obj[nr], MA_src);

    if(func && elem) {
      DebOut("call WM_INITDIALOG\n");
      func(elem, WM_INITDIALOG, 0, 0);
    }
  }

  DebOut("set currentpage to %d ..\n", currentpage);
  DoMethod(pages, MUIM_Set, MUIA_Group_ActivePage, nr);
  DebOut("currentpage is now: %d\n", currentpage);
  DebOut("MUIHook_list left\n");

  AROS_USERFUNC_EXIT
}

struct Hook MyMuiHook_start;

void gui_to_prefs (void);

/* "Start" button of WinUAE */
AROS_UFH2(void, MUIHook_start, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  DebOut("START!\n");

  /* 0 = normal, 1 = nogui, -1 = disable nogui */
  gui_to_prefs ();
  aros_hide_gui();
#if 0
  quit_program=0;
  uae_restart (-1, NULL);
#endif
  DebOut("send SIGBREAKF_CTRL_F to ourselves..\n");
  Signal(FindTask(NULL), SIGBREAKF_CTRL_F);

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

  DebOut("send SIGBREAKF_CTRL_C to ourselves..\n");
  Signal(FindTask(NULL), SIGBREAKF_CTRL_C);
 
  AROS_USERFUNC_EXIT
}

int *AboutDlgProc     (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *PathsDlgProc     (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *QuickstartDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *LoadSaveDlgProc  (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int *CPUDlgProc       (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *ChipsetDlgProc   (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *ChipsetDlgProc2  (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *KickstartDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *MemoryDlgProc    (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *FloppyDlgProc    (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *HarddiskDlgProc  (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *ExpansionDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *Expansion2DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int *SwapperDlgProc   (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

//#define SHOW_INCLOMPLETE_PAGES 
Object* FixedObj(IPTR src)
{
 Object *_fObj = NULL;

  DebOut("==========================================================================\n");
#ifdef SHOW_INCLOMPLETE_PAGES
    struct TagItem fo_tags[] =
    {
        { MA_src, src},
        { TAG_DONE, 0}
    };

    if (CL_Fixed)
    {
        _fObj = (Object*) NewObjectA(CL_Fixed->mcc_Class, NULL, fo_tags);
    }
#else
    _fObj = TextObject,
            MUIA_Text_Contents, "\33cThis is an \33bAlpha Version\33n!\n\nThere is still some stuff missing, sorry.",
        End;
#endif
    return _fObj;
}

Object* FixedProcObj(Element *elem, IPTR proc)
{
  elem->obj= NULL;

  DebOut("==========================================================================\n");
  DebOut("elem %p, proc 0x%p\n", elem, proc);
  struct TagItem fo_tags[] =
  {
      { MA_src, (IPTR) elem}, 
      { MA_dlgproc, proc},
      { TAG_DONE, 0}
  };

  if (CL_Fixed)
  {
      elem->obj = (Object*) NewObjectA(CL_Fixed->mcc_Class, NULL, fo_tags);
  }

  DebOut("obj %p\n", elem->obj);
  return elem->obj;

}

Object* FixedProcObj(ULONG idc, IPTR proc) {
  Element *elem;

  DebOut("idc %d, proc 0x%p\n", idc, proc);
  elem=get_control(NULL, idc);
  DebOut("=> elem: %p:\n", elem);
  return FixedProcObj(elem, proc);
}

extern char *STRINGTABLE[];
#if 0
static char *list_source_array[30];
#endif

extern struct MUI_CustomClass *CL_LeftFrame;
struct MUI_CustomClass *LeftFrame_init(void);

Object* build_gui(void) {
#if 0
  ULONG i;
#endif

  DebOut("building gui..\n");

  if(app) {
    DebOut("build_gui was already called before, do nothing!\n");
    return app;
  }

#if 0
  /* build array for left listview, strdup gets freed automatically on exit */
  i=0;
  while(listelements[i].id) {
    list_source_array[i]=strdup(STRINGTABLE[listelements[i].id]); 
    DebOut("list_source_array[%d]=%s\n", i, list_source_array[i]);
    i++;
  }
  list_source_array[i]=(char *)NULL;
#endif

  LeftFrame_init();

  struct TagItem frame_tags[] = {
                  { MUIA_Frame, MUIV_Frame_ReadList },
                  { MUIA_List_SourceArray, (IPTR) listelements },
                  { MUIA_Background, (IPTR) "2:ffffffff,ffffffff,ffffffff" },
                  { MUIA_Font, MUIV_Font_List },
                  { TAG_DONE, 0 }
  };


  app = MUI_NewObject(MUIC_Application,
                      MUIA_Application_Author, (IPTR) "Toni Willen, many others and Oliver Brunner",
                      MUIA_Application_Base, (IPTR)   "AROSUAE",
                      MUIA_Application_Copyright, (IPTR)"(c) 2017 Oliver Brunner",
                      MUIA_Application_Description, (IPTR)"AROS port of WinUAE",
                      MUIA_Application_Title, (IPTR)"Janus-UAE2",
                      MUIA_Application_Version, (IPTR)"$VER: Janus-UAE2 0.4 (27.11.2017)",
                      MUIA_Application_Window, (IPTR)(win = MUI_NewObject(MUIC_Window,
                        MUIA_Window_Title, (IPTR)"Janus-UAE2 Properties",
                        MUIA_Window_RootObject, root=MUI_NewObject(MUIC_Group,
                          MUIA_Group_Horiz, TRUE,
                          MUIA_Group_Child,
                            /* left group with selector and reset/quit buttons begin */
                            MUI_NewObject(MUIC_Group,
                              MUIA_Group_Horiz, FALSE,
                              MUIA_Group_Child,
                                ListviewObject,
                                          MUIA_Frame, MUIV_Frame_Group,
                                          MUIA_Background, "2:FFFFFFFF,FFFFFFFF,FFFFFFFF",
                                          MUIA_Listview_ScrollerPos, MUIV_Listview_ScrollerPos_None,
                                          MUIA_Listview_List, 
                                          leftframe=(Object *) NewObjectA(CL_LeftFrame->mcc_Class,
                                            NULL,
                                            //MUIA_Frame, MUIV_Frame_ReadList,
                                            //MUIA_List_SourceArray, (IPTR) list_source_array,
                                            //MUIA_Font, MUIV_Font_List
                                            frame_tags
                                          ),
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
                                  Child, page_obj[0]=FixedProcObj(IDD_ABOUT,      (IPTR) &AboutDlgProc     ),
                                  Child, page_obj[1]=FixedProcObj(IDD_PATHS,      (IPTR) &PathsDlgProc     ),
                                  Child, page_obj[2]=FixedProcObj(IDD_QUICKSTART, (IPTR) &QuickstartDlgProc),
                                  Child, page_obj[3]=FixedProcObj(IDD_LOADSAVE,   (IPTR) &LoadSaveDlgProc  ),
                                  //Child, FixedObj((IPTR)IDD_LOADSAVE),
                                  Child, TextObject,  MUIA_Text_Contents, "\33c\33bTODO", End,
                                  Child, page_obj[5]=FixedProcObj(IDD_CPU,        (IPTR) &CPUDlgProc       ),
                                  Child, page_obj[6]=FixedProcObj(IDD_CHIPSET,    (IPTR) &ChipsetDlgProc   ),
                                  Child, page_obj[7]=FixedProcObj(IDD_CHIPSET2,   (IPTR) &ChipsetDlgProc2  ),
                                  Child, page_obj[8]=FixedProcObj(IDD_KICKSTART,  (IPTR) &KickstartDlgProc ),
                                  Child, page_obj[9]=FixedProcObj(IDD_MEMORY,     (IPTR) &MemoryDlgProc    ),
                                  Child, page_obj[10]=FixedProcObj(IDD_FLOPPY,    (IPTR) &FloppyDlgProc    ),
                                  Child, page_obj[11]=FixedProcObj(IDD_HARDDISK,  (IPTR) &HarddiskDlgProc  ),
                                  Child, page_obj[12]=FixedProcObj(IDD_EXPANSION2,(IPTR) &Expansion2DlgProc),
                                  Child, page_obj[13]=FixedProcObj(IDD_EXPANSION, (IPTR) &ExpansionDlgProc ),
                                  //Child, FixedObj((IPTR)IDD_LOADSAVE),
                                  Child, TextObject,  MUIA_Text_Contents, "\33c\33bTODO", End,
                                  Child, page_obj[15]=FixedObj(IDD_DISPLAY),
                                  Child, page_obj[16]=FixedObj(IDD_SOUND),
                                  Child, page_obj[17]=FixedObj(IDD_GAMEPORTS),
                                  Child, page_obj[18]=FixedObj(IDD_IOPORTS),
                                  Child, page_obj[19]=FixedObj(IDD_INPUT),
                                  Child, page_obj[20]=FixedObj(IDD_AVIOUTPUT),
                                  Child, page_obj[21]=FixedObj(IDD_FILTER),
                                  Child, page_obj[22]=FixedProcObj(IDD_DISK,      (IPTR) &SwapperDlgProc  ),
                                  Child, page_obj[23]=FixedObj(IDD_MISC1),
                                  Child, page_obj[24]=FixedObj(IDD_MISC2),
                                End,
                              MUIA_Group_Child,
                                MUI_NewObject(MUIC_Group,
                                  MUIA_Group_Horiz, TRUE,
                                  MUIA_Group_Child,
                                    HSpace(0),
                                  MUIA_Group_Child,
                                    HSpace(0),

                                  MUIA_Group_Child,
                                    errorlog=MUI_MakeObject(MUIO_Button,"Error log",
                                    MUIA_Background, (IPTR) MUII_SHINE),
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
  //kprintf("leftframe: 0x%p\n", leftframe);

#if 0
  {
    // foo
    extern Object *image_obj;
    static char blafoo[256];
    sprintf(blafoo, "\33O[%016lx] foo", (APTR) image_obj);
    DebOut("blafoo %s\n", blafoo);
    DoMethod(leftframe, MUIM_List_InsertSingle, (APTR) blafoo, MUIV_List_Insert_Bottom);


  }
#endif

  MyMuiHook_list.h_Entry=(APTR) MUIHook_list;
  MyMuiHook_list.h_Data =NULL;
  DoMethod(leftframe, MUIM_Notify,
                        MUIA_List_Active, MUIV_EveryTime, 
                        (IPTR) leftframe, 2, MUIM_CallHook,(IPTR) &MyMuiHook_list);

  /* button events */
  MyMuiHook_start.h_Entry=(APTR) MUIHook_start;
  MyMuiHook_start.h_Data =NULL;
  DoMethod(start, MUIM_Notify, MUIA_Pressed, FALSE, start, 3, MUIM_CallHook, (IPTR) &MyMuiHook_start, TRUE);

  MyMuiHook_reset.h_Entry=(APTR) MUIHook_reset;
  MyMuiHook_reset.h_Data =NULL;
  DoMethod(reset, MUIM_Notify, MUIA_Pressed, FALSE, reset, 3, MUIM_CallHook, (IPTR) &MyMuiHook_reset, TRUE);

  MyMuiHook_quit.h_Entry=(APTR) MUIHook_quit;
  MyMuiHook_quit.h_Data =NULL;
  DoMethod(quit,  MUIM_Notify, MUIA_Pressed, FALSE, quit , 3, MUIM_CallHook, (IPTR) &MyMuiHook_quit , TRUE);

  /* close button */
  DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
     MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

  return app;
}

/*
 * close/cancel: 0
 * ok: 1
 */
int aros_main_loop(void) {

  ULONG signals = 0;

  DebOut("entered..\n");

  while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
  {
    signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_F);
    if (signals & SIGBREAKF_CTRL_C) {
      uae_quit();
      /* cancel/close window */
      DebOut("SIGBREAKF_CTRL_C: return 0\n");
      return 0;
    }
    if (signals & SIGBREAKF_CTRL_D) {
      DebOut("SIGBREAKF_CTRL_D: ?\n");
    }
    if (signals & SIGBREAKF_CTRL_F) {
      /* "Start" button pressed! */
      DebOut("SIGBREAKF_CTRL_F: return 1\n");
      return 1;
    }
  }

  uae_quit();
  DebOut("MUIV_Application_ReturnID_Quit: return 0\n");
  return 0;
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
#if 0
  ULONG w;
#endif

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
    if (!init_class()) {
        fprintf(stderr, "ERROR: Unable to initalize custom class(es)!\n");
        return 0;
    }
    app = build_gui();
  }

  if(!app) {
    fprintf(stderr, "ERROR: Unable to initialize GUI!\n");
    return 0;
  }
  else {
    DebOut("aros_init_gui succeeded\n");
  }

  return 1;
}

/* 
 * aros_show_gui
 *
 * display the GUI
 *
 * return
 *  0: cancel/close
 *  1: ok
 */

int aros_show_gui(void) {
  int ret;

  DebOut("entered\n");

  if(!app) {
    DebOut("aros_show_gui called for the first time!\n");
    if (!init_class()) {
        fprintf(stderr, "ERROR: Unable to initialize custom class(es)!\n");
        abort();
    }
    app = build_gui();
  }

  if(!app) {
    fprintf(stderr, "ERROR: Unable to initialize GUI!\n");
    //return -1;
    abort(); /* WinUAE does exactly this.. good idea!? */
  }

  /* activate current page */
  DebOut("set currentpage to %d ..\n", currentpage);
  DoMethod(leftframe, MUIM_Set, MUIA_List_Active, currentpage);
  DebOut("currentpage switched to %d\n", currentpage);

  DoMethod(win, MUIM_Set, MUIA_Window_Open, TRUE);
  DebOut("window opened!\n");

  ret = aros_main_loop();

  DoMethod(win, MUIM_Set, MUIA_Window_Open, FALSE);

  DebOut("ret: %d\n", ret);
  return ret;
}

/* not necessary ? */
void aros_hide_gui(void) {

  DebOut("Send hide signal\n");

  Signal(FindTask(NULL), SIGBREAKF_CTRL_D);
}

void gui_exit(void) {

  DebOut("entered\n");

  // ? Signal(FindTask(NULL), SIGBREAKF_CTRL_C);

  if(app) {
    MUI_DisposeObject(app);
    app=NULL;
    delete_class();
  }
}


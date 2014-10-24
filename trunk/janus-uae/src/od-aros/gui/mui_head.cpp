#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "gui.h"

Object *app, *win, *root, *leftframe;

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
  IDS_LOADSAVE,
  IDS_CPU,
  IDS_CHIPSET,
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
  IDS_FILTER,
  IDS_DISK,
  IDS_MISC1,
  IDS_MISC2,
  NULL
};

HOOKPROTO(MUIHook_list, ULONG, obj, hookpointer)
{
  AROS_USERFUNC_INIT
#if 0

  DebOut("MUIHook_list called\n");

  DoMethod(obj,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,(ULONG) &selstr);

#endif
  printf("FOO!\n");
  return 0;
  AROS_USERFUNC_EXIT
}
MakeHook(MyMuiHook_list, MUIHook_list);


Object* build_gui(void) {
  int i=0;

  DebOut("src: %lx\n", IDD_FLOPPY);

  while(IDD_FLOPPY[i].windows_type) {
    DebOut("i: %d\n", i);
    DebOut("IDD_FLOPPY[%d].windows_type: %d\n", i, IDD_FLOPPY[i].windows_type);
    DebOut("IDD_FLOPPY[%d].x: %d\n", i, IDD_FLOPPY[i].x);
    DebOut("IDD_FLOPPY[%d].text: %s\n", i, IDD_FLOPPY[i].text);
    i++;
  }

  DebOut("send: %lx\n", &IDD_FLOPPY);
  app = MUI_NewObject(MUIC_Application,
                      MUIA_Application_Author, (ULONG) "Oliver Brunner",
                      MUIA_Application_Base, (ULONG)   "AROSUAE",
                      MUIA_Application_Copyright, (ULONG)"(c) 2014 Oliver Brunner",
                      MUIA_Application_Description, (ULONG)"WinUAE GUI converted to Zune",
                      MUIA_Application_Title, (ULONG)"WinUAE",
                      MUIA_Application_Version, (ULONG)"$VER: WinUAE 0.2 (x.x.2014)",
                      MUIA_Application_Window, (ULONG)(win = MUI_NewObject(MUIC_Window,
                        MUIA_Window_Title, (ULONG)"WinUAE GUI",
                        MUIA_Window_RootObject, root=MUI_NewObject(MUIC_Group,
                          MUIA_Group_Horiz, TRUE,
                          MUIA_Group_Child,
                            ListviewObject,
                                          MUIA_Listview_List, leftframe=ListObject,
                                            MUIA_Frame, MUIV_Frame_ReadList,
                                            MUIA_List_SourceArray, (ULONG) listelements,
                                            MUIA_Background, MUII_ReadListBack,
                                            MUIA_Font, MUIV_Font_List, 
                                          End,
                                        End,
                          MUIA_Group_Child, 
                            NewObject(CL_Fixed->mcc_Class, NULL, MA_src, IDD_FLOPPY, TAG_DONE),
                      TAG_END),
                    TAG_END)),
        TAG_END);
  /* List click events */

  kprintf("leftframe: %lx\n", leftframe);
  DoMethod(leftframe, MUIM_Notify,
                        MUIA_List_Active, MUIV_EveryTime, 
                        (ULONG) leftframe, 2, MUIM_CallHook,(ULONG) &MyMuiHook_list);

  return app;

}

static void main_loop(void) {

  ULONG signals = 0;

  SetAttrs(win, MUIA_Window_Open, TRUE, TAG_DONE);

  while (DoMethod(app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
  {
    signals = Wait(signals | SIGBREAKF_CTRL_C);
    if (signals & SIGBREAKF_CTRL_C) break;
  }

  SetAttrs(win, MUIA_Window_Open, FALSE, TAG_DONE);
}

int main(void) {
  Object *test;
  init_class();
  app = build_gui();

  //test=(Object *) NewObject(CL_Fixed->mcc_Class, NULL,MA_src,(ULONG) NULL,TAG_DONE);

  //DoMethod(root,OM_ADDMEMBER,(LONG) test);

  if (app)
  {
    //notifications();
    main_loop();
    MUI_DisposeObject(app);
  }

  delete_class();

  return 0;
}

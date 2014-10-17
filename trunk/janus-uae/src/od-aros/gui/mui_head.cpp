#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include "gui.h"

Object *app, *win, *root;

Object* build_gui(void) {
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
                          MUIA_Group_Child, /*MUI_NewObject(MUIC_Text,
                          MUIA_Text_Contents, (ULONG)"Hello world!",
                        TAG_END),*/
                            NewObject(CL_Fixed->mcc_Class, NULL,MA_src,(ULONG) NULL,TAG_DONE),
                      TAG_END),
                    TAG_END)),
        TAG_END);
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

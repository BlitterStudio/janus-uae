/************************************************************************ 
 *
 * Copyright 2009 Oliver Brunner - aros<at>oliver-brunner.de
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

#include <exec/types.h>
#include <libraries/mui.h>
#include <dos/var.h>
#include <exec/libraries.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <aros/debug.h>

#define JWTRACING_ENABLED 1
//#define JW_ENTER_ENABLED 1
#include "j.h"

static void aros_splash_window_thread (void);


static APTR splash_win=NULL;
static APTR splash_app=NULL;
static APTR splash_txt=NULL;

static void aros_splash_window_thread (void) {

  ULONG signals=0;

  splash_app=ApplicationObject,
          SubWindow, splash_win=WindowObject,
            MUIA_Window_Borderless, TRUE,
            MUIA_Window_Width, 200,
            WindowContents, VGroup,
              MUIA_InnerTop,    40,
              MUIA_InnerBottom, 40,
              MUIA_InnerLeft,   40,
              MUIA_InnerRight,  40,
              Child, TextObject,
                MUIA_Text_Contents, "\33b\33cJanus-UAE\n\33n\n\33cVersion " PACKAGE_VERSION "\n",
              End,
              Child, splash_txt=TextObject,
                MUIA_Text_PreParse, "\33c",
                MUIA_Text_Contents, "starting...\n",
              End,
            End,
          End,
        End;

  if(!splash_app || !splash_win) {
    goto EXIT;
  }

  DoMethod(splash_win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            splash_app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
  set(splash_win, MUIA_Window_Open, TRUE);

  while(DoMethod(splash_app, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit) {
    signals = Wait(signals | SIGBREAKF_CTRL_C);
    if (signals & SIGBREAKF_CTRL_C) {
      break;
    }
  }


  /* ... and a time to die. */
  JWLOG("closing splash window ..!\n");

EXIT:

  if(splash_win) {
    JWLOG("hide splash_win %lx\n", splash_win);
    set(splash_win, MUIA_Window_Open, FALSE);
    splash_win=NULL;

    JWLOG("close splash_app %lx\n", splash_app);
    MUI_DisposeObject(splash_app);
  }

  splash_app=NULL;
  splash_txt=NULL;

  JWLOG(" === splash window thread dies ====\n");

  LEAVE
}

static char process_name[]="j-uae splash window";
struct Task *splash_window_task=NULL;

int aros_splash_start_thread (void) {

  ENTER

  JWLOG("starting thread ..\n");

  splash_window_task = (struct Task *)
	  myCreateNewProcTags ( //NP_Output, Output (),
				//NP_Input, Input (),
				NP_Name, (ULONG) process_name,
				//NP_CloseOutput, FALSE,
				//NP_CloseInput, FALSE,
				NP_StackSize, 0x4000,
				NP_Priority, 0,
				NP_Entry, (ULONG) aros_splash_window_thread,
				TAG_DONE);


  if(!splash_window_task) {
    JWLOG("ERROR: could not create splash window thread\n");
    LEAVE
    return FALSE;
  }

  JWLOG("splash window thread created\n");

  LEAVE
  return TRUE;
}


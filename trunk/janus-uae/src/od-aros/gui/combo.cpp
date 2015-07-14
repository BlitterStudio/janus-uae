/************************************************************************
 *
 * combo.cpp
 *
 * Copyright 2015 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE2.
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
 ************************************************************************
 *
 * First try of a combobox compatible Zune Class.
 *
 * Extends string gadgets:
 * - popup window for predefined selections
 * - type ahead selection with TAB key (leave with tab
 *   to accept the highlighted entry in the list
 * - click on list takes content to string box
 * - popup windows are moved in sync with string gadget
 *
 * BUGs: - clicking on a button inside main window
 *         does not close the popup. I don't get any
 *         events/messages whatever from Zune here!?
 *       - some flickering possible, as GoActives are received, even
 *         if object gets *not active*. Zune bug?
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>
#include <exec/types.h>
#include <libraries/mui.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

#include "combo.h"

#if !defined(XGET)
#define XGET(object, attribute)                 \
({                                              \
    IPTR __storage = 0;                         \
    GetAttr((attribute), (object), &__storage); \
    __storage;                                  \
})
#endif /* !XGET */

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(obj, x, y) (_between(_mleft(obj), (x), _mright(obj)) \
                          && _between(_mtop(obj), (y), _mbottom(obj)))

#define GETDATA struct Data *data = (struct Data *)INST_DATA(cl, obj)

#if defined(JUAE_DEBUG)
#define DebOut(...) do { bug("[%lx]: %s:%d %s(): ",FindTask(NULL),__FILE__,__LINE__,__func__);bug(__VA_ARGS__); } while(0)
#define TODO() bug("TODO ==> %s:%d: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)
#else
#define DebOut(...)
#define TODO(...)
#endif

struct MUI_CustomClass *CL_Combo=NULL;

struct Data {
  Object *obj_entry;
  Object *obj_listview;
  Object *obj_popup_win;
  Object *obj_list;
  STRPTR *entries;
  BOOL active;
  BOOL handler_active;
  struct MUI_EventHandlerNode ehn;
  struct Hook MyMuiHook_ack;
  struct Hook MyMuiHook_content;
  struct Hook MyMuiHook_list;
};

static VOID ShowPopupWin(Object *obj, struct Data *data);
static VOID HidePopupWin(Object *obj, struct Data *data);
static BOOL MakePopupWin(Object *obj, struct Data *data);

/*****************************************************
 * MUIHook_list
 *
 * handle popup window list selections
 *****************************************************/
AROS_UFH2(static void, MUIHook_list, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  struct Data *data=(struct Data *) hook->h_Data;
  LONG active;

  DebOut("List!\n");

  if(data->entries) {
    GetAttr(MUIA_List_Active, data->obj_list, (IPTR *) &active);
    DebOut("active: %d\n", active);

    if(active>=0 && data->entries[active]) {
      HidePopupWin((Object *) obj, data);
      DebOut("        %s\n", data->entries[active]);
      SetAttrs(data->obj_entry,       MUIA_String_Contents, data->entries[active],          TAG_DONE);
      SetAttrs(data->obj_entry,       MUIA_String_BufferPos, strlen(data->entries[active]), TAG_DONE);
    }
  }

  AROS_USERFUNC_EXIT
}

static VOID ShowPopupWin(Object *obj, struct Data *data) {

  WORD x, y, winx, winy, winw, winh;
  IPTR intui_win;

  DebOut("ShowPopupWin!\n");

  if(!data->obj_popup_win || !obj) {
    DebOut("not all windows exist!\n");
    return;
  }

  winw = _width(data->obj_entry) - 1;
  winx = _left(data->obj_entry) + 1; /* _parent for the frame */
  winy = _bottom(data->obj_entry) + 1;

  SetAttrs(data->obj_popup_win, MUIA_Window_Width, winw, 
                                MUIA_Window_Screen, (IPTR) _screen(data->obj_entry),
                                MUIA_Window_LeftEdge, winx, 
                                MUIA_Window_TopEdge, winy,
                                MUIA_Window_Open, TRUE, TAG_DONE);
}

static VOID HidePopupWin(Object *obj, struct Data *data) {

  SetAttrs(data->obj_popup_win, MUIA_Window_Open, FALSE, TAG_DONE);
}

static BOOL MakePopupWin(Object *obj, struct Data *data) {

    const struct ZuneFrameGfx *zframe;
    struct RastPort *rp, *saverp;
    Object *child, *cstate;
    WORD i;
    LONG winx, winy;
    LONG winw;
   
    if(data->obj_popup_win) {
      return TRUE;
    }

    data->obj_popup_win = WindowObject,
                            MUIA_Window_NoMenus,      TRUE,
                            MUIA_Window_Borderless,   TRUE,
                            MUIA_Window_CloseGadget,  FALSE,
                            MUIA_Window_DepthGadget,  FALSE,
                            MUIA_Window_DragBar,      FALSE,
                            MUIA_Window_ZoomGadget,   FALSE,
                            MUIA_Window_SizeGadget,   FALSE,
                            MUIA_Window_Title,        NULL,
                            MUIA_Window_Activate,     FALSE,
                            MUIA_Window_Open,         TRUE,
                            MUIA_Window_RefWindow,    _win(data->obj_entry),

                            WindowContents,
                              VGroup,
                                Child, data->obj_listview=ListviewObject,
                                          MUIA_Listview_List, data->obj_list=ListObject,
                                              MUIA_Frame, MUIV_Frame_InputList,
                                              MUIA_Background, MUII_ListBack,
                                              MUIA_List_AutoVisible, TRUE,
                                              MUIA_List_SourceArray, (APTR) data->entries,
                                          End,
                                        End,
                              End,
                          End;

    if (!data->obj_popup_win) {

        return FALSE;
    }

    DoMethod(_app(data->obj_entry), OM_ADDMEMBER, data->obj_popup_win);

    data->MyMuiHook_list.h_Entry=(APTR) MUIHook_list;
    data->MyMuiHook_list.h_Data =data;
    DoMethod(data->obj_list, MUIM_Notify,
                  MUIA_List_Active, MUIV_EveryTime, 
                  (IPTR) data->obj_list, 2,
                  MUIM_CallHook, (IPTR) &data->MyMuiHook_list);

    return TRUE;
}


AROS_UFH2(static void, MUIHook_ack, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  DebOut("MUIHook_ack!\n");


  AROS_USERFUNC_EXIT
}

/*****************************************************
 * MUIHook_content
 *
 * For every typed in character, check it we can
 * match the entry content with one of the strings
 * in the dropdown list.
 *
 * First check, if the beginning of one of the
 * list entries matches the entry content.
 * If not, check for matching substrings.
 *
 * All matches are case-insensitiv.
 *
 * If this ever gets an public Zune class, make
 * this configurable (TODO).
 *****************************************************/
AROS_UFH2(static void, MUIHook_content, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT
  char *content=NULL;
  struct Data *data=(struct Data *) hook->h_Data;
  ULONG i=0;
  size_t l=0;
  BOOL match=FALSE;

  GetAttr(MUIA_String_Contents, (Object *) obj, (IPTR *) &content);
  DebOut("content: %s\n", content);

  if(data->entries && content) {
    i=0;
    l=strlen(content);
    /* try beginning */
    while(data->entries[i]) {
      if(!strncasecmp(data->entries[i], content, l)) {
        DebOut("match: %s\n", data->entries[i]);
        DoMethod(data->obj_listview, MUIM_NoNotifySet, MUIA_List_Active, i);
        match=TRUE;
        break;
      }
      i++;
    }

    /* try substrings */
    if(!match) {
      i=0;
      l=strlen(content);
      while(data->entries[i]) {
        if(strcasestr(data->entries[i], content)) {
          DebOut("match: %s\n", data->entries[i]);
          DoMethod(data->obj_listview, MUIM_NoNotifySet, MUIA_List_Active, i);
          match=TRUE;
          break;
        }
        i++;
      }
    }
  }

  if(!match) {
    DoMethod(data->obj_listview, MUIM_NoNotifySet, MUIA_List_Active, MUIV_List_Active_Off);
  }

  AROS_USERFUNC_EXIT
}


static ULONG mGoActive (struct IClass *cl, APTR obj, Msg msg) {

  GETDATA;

  DebOut("mGoActive!\n");
  MakePopupWin((Object *) obj, data);

  if(!data->handler_active) {
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);
    data->handler_active=TRUE;
  }

  if(XGET(_win(data->obj_entry), MUIA_Window_ActiveObject)) {
    ShowPopupWin((Object *) obj, data);
  }
  data->active=TRUE;

  return 0;
}

static ULONG mGoInactive (struct IClass *cl, APTR obj, Msg msg) {

  GETDATA;
  IPTR pop, act, win;

  DebOut("mGoInactive begin!\n");

  GetAttr(MUIA_Window_Window, (Object *) data->obj_popup_win, &pop);
  /* If popup window is new active one, don't close it! */
  if(pop!=(IPTR) IntuitionBase->ActiveWindow) {
    data->active=FALSE;
    if(data->handler_active) {
      data->handler_active=FALSE;
      DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);
    }
    HidePopupWin((Object *) obj, data);
  }

  DebOut("mGoInactive done!\n");
  return 0;
}

static IPTR mNew(struct IClass *cl, APTR obj, Msg msg) {
  struct TagItem *tstate, *tag;

  obj = (APTR)DoSuperMethodA(cl, obj, msg);

  if(!obj) {
    DebOut("ERROR: DoSuperMethodA failed!\n");
    return 0;
  }

  GETDATA;

  data->obj_entry=(Object *) obj;

  SetAttrs(obj, MUIA_Frame, MUIV_Frame_String, MUIA_CycleChain, 1, TAG_DONE);

  data->MyMuiHook_ack.h_Entry=(APTR) MUIHook_ack;
  data->MyMuiHook_ack.h_Data =NULL;
  DoMethod(obj, MUIM_Notify,
                MUIA_String_Acknowledge, MUIV_EveryTime, 
                (IPTR) obj, 2,
                MUIM_CallHook, (IPTR) &data->MyMuiHook_ack);

  data->MyMuiHook_content.h_Entry=(APTR) MUIHook_content;
  data->MyMuiHook_content.h_Data =data;
  DoMethod(obj, MUIM_Notify,
                MUIA_String_Contents, MUIV_EveryTime, 
                (IPTR) obj, 2,
                MUIM_CallHook, (IPTR) &data->MyMuiHook_content);

  data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | /*IDCMP_RAWKEY |*/ IDCMP_NEWSIZE | IDCMP_CHANGEWINDOW;
  data->ehn.ehn_Priority = 0;
  data->ehn.ehn_Flags = 0;
  data->ehn.ehn_Object = (Object *) obj;
  data->ehn.ehn_Class = cl;

  return (IPTR) obj;
}

static IPTR mSet(struct IClass *cl, APTR obj, struct opSet *msg) {

  struct TagItem *tstate, *tag;
  GETDATA;

  DebOut("mSet(%lx,entry %lx,%lx)\n",data,obj,msg);

  tstate = msg->ops_AttrList;
  while ((tag = (struct TagItem *) NextTagItem(&tstate))) {

    switch (tag->ti_Tag) {
      case MUIA_List_Active:
        SetAttrs(data->obj_list, MUIA_List_Active, tag->ti_Data, TAG_DONE);
        return TRUE;
        break;
      //case MUIA_String_Contents:
        //DoMethod(data->obj_popup_win, OM_DISPOSE);
        //break;
    }
  }
  return DoSuperMethodA(cl, obj, (Msg) msg);
}

static IPTR mGet(struct IClass *cl, Object *obj, struct opGet *msg) {

  struct TagItem *tstate, *tag;
  GETDATA;

  //DebOut("mGet(%lx,entry %lx,%lx)\n",data,obj,msg);

  switch (msg->opg_AttrID) {
      case MUIA_List_Active:
        GetAttr(MUIA_List_Active, data->obj_list, msg->opg_Storage);
        break;
      case MUIA_List_First:
        GetAttr(MUIA_List_First, data->obj_list, msg->opg_Storage);
        break;
      case MUIA_List_AutoVisible:
        GetAttr(MUIA_List_AutoVisible , data->obj_list, msg->opg_Storage);
        break;
      case MUIA_List_InsertPosition:
        GetAttr(MUIA_List_InsertPosition , data->obj_list, msg->opg_Storage);
        break;
      default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
  }

  return TRUE;

}


/* check, if user clicked outside */
static IPTR mHandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg) {

  GETDATA;
  LONG muikey = msg->muikey;

  //DebOut("entered (msg->muikey: %d)\n", msg->muikey);

  /* up/down key? */
  if(muikey != MUIKEY_NONE && data->active) {
    switch (muikey) {
      case MUIKEY_UP:
        SetAttrs(data->obj_list, MUIA_List_Active, MUIV_List_Active_Up, TAG_DONE);
        break;
      case MUIKEY_DOWN:
        SetAttrs(data->obj_list, MUIA_List_Active, MUIV_List_Active_Down, TAG_DONE);
        break;
      case MUIKEY_GADGET_NEXT: {
        LONG active;

        /* gadget left with TAB, so user accepts listview value 
         * WARNING: needs updated window.c in AROS zune sources!! TODO
         */
        GetAttr(MUIA_List_Active, data->obj_list, (IPTR *) &active);
        if(active >= 0) {
          SetAttrs(obj, MUIA_String_Contents, data->entries[active], TAG_DONE);
        }
        break;
      }
    }
  }

  /* mouse pressed outside? */
  if (msg->imsg) {
    UWORD code = msg->imsg->Code;
    WORD x = msg->imsg->MouseX;
    WORD y = msg->imsg->MouseY;
    switch (msg->imsg->Class) {

      case IDCMP_NEWSIZE: 
      case IDCMP_CHANGEWINDOW: {
          ULONG wasopen;
          GetAttr(MUIA_Window_Open, data->obj_popup_win, (IPTR *) &wasopen);
          HidePopupWin(obj,data);
          if(wasopen) {
            ShowPopupWin(obj,data);
          }
        }
        break;
      case IDCMP_MOUSEBUTTONS: {
        if (code == SELECTDOWN) {
          if(!_isinobject(obj, x, y)) {
            DebOut("clicked outside => InActive\n");
            HidePopupWin(obj,data);
          }
        }
        break;
      }
#if 0
      case IDCMP_RAWKEY: {
        unsigned char code;

        if (data->active) {
          break;
        }

        code=ConvertKey(msg->imsg);
        DebOut("code: %d\n", code);
        struct Locale *locale = OpenLocale(NULL);
        if (!(code >= 0x09 && code <= 0x0D) && IsPrint(locale, code)) {
          DebOut("char: %c\n", code);
        }
        CloseLocale(locale);
        break;
      }
#endif
    }
  }

}

/***************************************************
 *
 * MUIM_List_Insert
 *
 * Replace the current list with the new one.
 * WARNING: **  totally different than
 *              MUIM_List_Insert on a list. **
 *
 * TODO: used different name as MUIM_List_Insert
 *
 ****************************************************/
static IPTR mInsert(struct IClass *cl, Object *obj, struct MUIP_List_Insert *msg) {

  GETDATA;

  DebOut("entered!\n");

  data->entries=(char **)msg->entries;

  DoMethod(data->obj_list, MUIM_List_Clear);
  DoMethod(data->obj_list, MUIM_List_Insert, (IPTR) data->entries, -1, MUIV_List_Insert_Top);

  return TRUE;
}

BOOPSI_DISPATCHER(IPTR, mDispatcher, cl, obj, msg) {
  GETDATA;

  //DebOut("(%lx)->msg->MethodID: %lx\n",obj,msg->MethodID);

  switch (msg->MethodID)
  {
    case OM_NEW            : return mNew        (cl, obj, msg);
    case MUIM_GoActive     :        mGoActive   (cl, obj, msg); break;
    case MUIM_GoInactive   :        mGoInactive (cl, obj, msg); break;
    case OM_SET            : return mSet        (cl, obj, (struct opSet *)msg); break;
    case OM_GET            : return mGet        (cl, (Object *) obj, (struct opGet *)msg);
    case MUIM_List_Insert  : return mInsert(cl, (Object *) obj, (struct MUIP_List_Insert *) msg);
            
    case MUIM_HandleEvent  :        mHandleEvent(cl, (Object *) obj, (struct MUIP_HandleEvent *) msg);
                            /* FALL THROUGH! */
  }

  return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

int create_combo_class(void) {
  if(!CL_Combo) {
    CL_Combo = MUI_CreateCustomClass(NULL, MUIC_String, NULL, sizeof(struct Data), (APTR)&mDispatcher);
  }
}

void delete_combo_class(void) {
  if(CL_Combo) {
    MUI_DeleteCustomClass(CL_Combo);
    CL_Combo=NULL;
  }
}

/* simple standalone test case */
#if 0
int main(void) {

  Object *app, *wnd, *but, *combo;
  STRPTR *e;

  if(!create_combo_class()) {
    printf("ERROR: creating class!\n");
    exit(1);
  }

  e=(STRPTR *) malloc(sizeof(void *)*10);
  e[0]=NULL;
  e[0]=strdup("aaa");
  e[1]=strdup("abc");
  e[2]=strdup("abcc");
  e[3]=strdup("abcd");
  e[4]=strdup("abcdxxxxxxx");
  e[5]=strdup("bbb");
  e[6]=strdup("ccc");
  e[7]=NULL;

  combo=(Object *) NewObject(CL_Combo->mcc_Class, NULL,TAG_DONE);

  app = ApplicationObject,
      SubWindow, wnd = WindowObject,
      MUIA_Window_Title, "Hello world!",
      WindowContents, VGroup,
          Child, SimpleButton("foo 0"),
          Child, SimpleButton("foo 1"),
          Child, SimpleButton("foo 2"),
          Child, TextObject,
            MUIA_Text_Contents, "\33cHello world!\nHow are you?",
          End,
          Child, combo,
          Child, SimpleButton("foo 4"),
          Child, SimpleButton("foo 5"),
          Child, SimpleButton("foo 6"),
          Child, but = SimpleButton("_Ok"),
          End,
      End,
      End;

  if (app != NULL) {

    ULONG sigs = 0;

    SetAttrs(but, MUIA_CycleChain, 1, TAG_DONE);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                  (IPTR)app, 2,
                  MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(but, MUIM_Notify, MUIA_Pressed, FALSE,
                  (IPTR)app, 2,
                  MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    SetAttrs(wnd, MUIA_Window_Open, TRUE, TAG_DONE);

    DoMethod(combo, MUIM_List_Insert, e);

    if (XGET(wnd, MUIA_Window_Open)) {
      while((LONG)DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
            != MUIV_Application_ReturnID_Quit) {
        if (sigs) {

          sigs = Wait(sigs | SIGBREAKF_CTRL_C);
          if (sigs & SIGBREAKF_CTRL_C) {
            break;
          }
        }
      }
    }
    MUI_DisposeObject(app);
  }

  delete_combo_class();
  return 0;
}
#endif


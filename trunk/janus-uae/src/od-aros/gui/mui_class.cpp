#define JUAE_DEBUG

#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <clib/alib_protos.h>

#include <graphics/gfxbase.h>
#include <mui/Rawimage_mcc.h>
#include <proto/reqtools.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#ifndef DONT_USE_URLOPEN
#include <mui/Urltext_mcc.h>
#endif

#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"
#include "combo.h"

#include "mui_class.h"

//#define RESIZE 155 / 100
#define RESIZE_X 150 /100
#define RESIZE_Y 140 /100

#define MAX_ENTRIES 255

//#define EMPTY_SELECTION "<-- none -->"
#define EMPTY_SELECTION ""

//static const char *Cycle_Dummy[] = { NULL };

struct ReqToolsBase *ReqToolsBase=NULL;
struct MUI_CustomClass *CL_Fixed=NULL;

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new);

extern Object *win;

/* return the control (Element *) of the item in the hDlg
 * if hDlg is NULL, search global
 */
Element *get_control(HWND hDlg, int item) {
  ULONG i=0;
  Element *elem=(Element *) hDlg;

  DebOut("elem: %p, item: %d\n", elem, item);

  /* skip to hDlg */
  if(hDlg) {
    while(WIN_RES[i].idc && (WIN_RES[i].idc != elem->idc)) {
      //DebOut("WIN_RES[%d].idc: %d, elem->idc %d\n", i, WIN_RES[i].idc, elem->idc);
      i++;
    }
  }

  while(WIN_RES[i].idc) {
    //DebOut("WIN_RES[%d].idc: %d, item %d\n", i, WIN_RES[i].idc, item);
    if(WIN_RES[i].idc==item) {
      return &WIN_RES[i];
    }
    i++;
  }

  DebOut("ERROR (?): can't find control %d in %p (NULL is ok here)\n", item, hDlg);
  return NULL;
}

BOOL flag_editable(ULONG flags) {

  if((flags & CBS_DROPDOWNLIST)==CBS_DROPDOWNLIST) {
    return FALSE;
  }
  return TRUE;
}

/****************************************
 * send_WM_INITDIALOG
 *
 * called from mui_head
 *
 * send WM_INITDIALOG to the now active
 * page
 ****************************************/
#if 0
void send_WM_INITDIALOG(ULONG nr) {

  //SendMessage (IDD_ELEMENT[nr], WM_INITDIALOG, 0, 0);
  //oli

  data->func(data->src, WM_INITDIALOG, 0, 0);
}
#endif
 

AROS_UFH2(void, MUIHook_combo, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  ULONG wParam;
  Element *elem;

  struct Data *data = (struct Data *) hook->h_Data;

  //DebOut("[%lx] entered\n", obj);
  //DebOut("[%lx] hook.h_Data: %lx\n", obj, hook->h_Data);
  //DebOut("[%lx] obj: %lx\n", obj);

  elem=(Element *) data->hwnd;

  elem->value=XGET((Object *) obj, MUIA_Cycle_Active);
  //DebOut("[%lx] MUIA_Cycle_Active: %d (mui obj: %lx)\n", obj, data->src[i].value, obj);

  DebOut("[%lx] state: %d\n", obj, elem->value);

  if(data->func) {
    if(elem->mem[elem->value] && !strcmp(elem->mem[elem->value], EMPTY_SELECTION)) {
      DebOut("[%lx] Empty selection (%s), do nothing\n", obj, EMPTY_SELECTION);
    }
    else {
      if(flag_editable(elem->flags)) {
        elem->value--;
      }
      wParam=MAKELPARAM(elem->idc, CBN_SELCHANGE);
      DebOut("[%lx] call function: %lx(IDC %d, CBN_SELCHANGE)\n", obj, data->func, elem->idc);
      data->func(elem, WM_COMMAND, wParam, 0); /* TODO: was data->src fist parameter.. correct now? */
    }
  }
  else {
    DebOut("[%lx] function is zero: %lx\n", obj, data->func);
  }

  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, MUIHook_entry, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  Element *elem;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;

  //DebOut("[%lx] entered\n", obj);
  //DebOut("[%lx] hook.h_Data: %lx\n", obj, hook->h_Data);
  //DebOut("[%lx] obj: %lx\n", obj);

  elem=(Element *) data->hwnd;

  DebOut("[%lx] elem: %p\n", obj, elem);

  if(data->func) {
    wParam=MAKELPARAM(elem->idc, 0); /* 0? really? */
    data->func(elem, WM_COMMAND, wParam, 0); /* TODO: was data->src fist parameter.. correct now? */
  }
  else {
    DebOut("[%lx] function is zero: %lx\n", obj, data->func);
  }

  AROS_USERFUNC_EXIT
}


AROS_UFH2(void, MUIHook_slide, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  struct Data *data = (struct Data *) hook->h_Data;
  Element *elem;
  ULONG wParam;

  DebOut("Sliding..\n");

  elem=(Element *) data->hwnd;

  if(data->func) {
#if 0
    wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
    DebOut("call function: %lx (IDC %d, wParam: %lx)\n", data->func, data->src[i].idc, wParam);
    data->func(data->src, WM_COMMAND, wParam, 0);
#endif

    /* warning: should we call BN_CLICKED here, too?
     * open console in Paths won't work without .. 
     */
    wParam=MAKELPARAM(elem->idc, BN_CLICKED);
    data->func(elem, WM_COMMAND, wParam, 0); /* TODO: was data->src fist parameter.. correct now? */

    data->func(elem, WM_HSCROLL, 0, 0);

  }
  else {
    DebOut("WARNING: function is zero: %lx\n", data->func);
    /* Solution: add DlgProc in mNew in mui_head.cpp */
  }

  AROS_USERFUNC_EXIT
}

/*
 * Which control sends which default event on click, see
 * https://msdn.microsoft.com/en-us/library/9y0at277.aspx
 */

AROS_UFH2(void, MUIHook_select, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  Element *elem;
  ULONG t;
  ULONG wParam;
  ULONG newstate;
  ULONG i;

  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("entered (hook.h_Data: %lx)\n", hook->h_Data);
  DebOut("data->hwnd: %p\n", data->hwnd );
  i=XGET((Object *) obj, MUIA_UserData);
  DebOut("MUIA_UserData: %d\n", i);

  elem=&WIN_RES[i];

  if(elem->text) {
    DebOut("obj: %lx => elem %p: %d (%s)\n", obj, elem, elem->text);
  }
  else {
    DebOut("obj: %lx => elem %p\n", obj, elem);
  }

  newstate=XGET((Object *) obj, MUIA_Selected);
  DebOut("MUIA_Selected is (now): %d (group: %d)\n", newstate, elem->group);

  if((elem->windows_type==CONTROL) && 
     (elem->flags2==BS_AUTORADIOBUTTON) && 
     (elem->group)) {

    /*** AUTORADIOBUTTON / Radio button logic: ***/

    if(elem->value == 1) {
      /* we were already the active one, we want to stay that, too! */
      DebOut("we were already the active one!\n");
      DoMethod(elem->obj, MUIM_NoNotifySet, MUIA_Selected, TRUE);
      /* no event */
      goto DONE;
    }

    if(newstate == 0) {
      /* this is impossible!*/
      DebOut("ERROR: this is impossible\n");
    }

    DebOut("deselect all my brothers\n");
    /* as we are part of a group, unselect all our active friends */
    t=0;
    while(WIN_RES[t].idc) {
      if(WIN_RES[t].group == elem->group && WIN_RES[t].value && WIN_RES[t].idc!=elem->idc) {
        /* same group */
        DebOut("  => also activated, same group, other guy!\n");
        DoMethod(WIN_RES[t].obj, MUIM_NoNotifySet, MUIA_Selected, FALSE); 
        WIN_RES[t].value=0;
      }
      t++;
    }
  }
  else if(elem->windows_type==COMBOBOX) {

    /*** COMBOBOX ***/

    /* CBN_SELCHANGE: A new combo box list item is selected. */
    if(data->func) {
      wParam=MAKELPARAM(elem->idc, CBN_SELCHANGE);
      DebOut("call function: %lx (IDC %d, wParam: %lx)\n", data->func, elem->idc, wParam);
      data->func(elem, WM_COMMAND, wParam, 0); /* TODO: was data->src fist parameter.. correct now? */
    }
    goto DONE;
  }
  elem->value=newstate;


DONE:
  if(data->func) {
    /* warning: should we call BN_CLICKED here, too?
     * open console in Paths won't work without .. 
     */
    wParam=MAKELPARAM(elem->idc, BN_CLICKED);
    data->func(elem, WM_COMMAND, wParam, 0); /* TODO: was data->src fist parameter.. correct now? */
  }
  else {
    DebOut("WARNING: function is zero: %lx\n", data->func);
    /* Solution: add DlgProc in mNew in mui_head.cpp */
  }

  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, MUIHook_pushbutton, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT
  Element *elem;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("[%lx] entered\n", obj);

  elem=(Element *) data->hwnd;

  DebOut("elem: %p\n", elem);

  if(data->func) {
    DebOut("call function: %lx\n", data->func);
    DebOut("IDC: %d\n", elem->idc);
    DebOut("WM_COMMAND: %d\n", WM_COMMAND);

    wParam=MAKELPARAM(elem->idc, BN_CLICKED);
    data->func(elem, WM_COMMAND, wParam, 0); /* TODO: was data->src fist parameter.. correct now? */
  }
  else {
    DebOut("function is zero: %lx\n", data->func);
  }

  AROS_USERFUNC_EXIT
}



/*****************************************************************************
 * Class
 *****************************************************************************/
AROS_UFH3(static ULONG, LayoutHook, AROS_UFHA(struct Hook *, hook, a0), AROS_UFHA(APTR, obj, a2), AROS_UFHA(struct MUI_LayoutMsg *, lm, a1)) 

  switch (lm->lm_Type) {
    case MUILM_MINMAX: {

      struct Data *data = (struct Data *) hook->h_Data;
      //struct Element *element=data->src;
      //ULONG mincw, minch, defcw, defch, maxcw, maxch;

      //DebOut("MUILM_MINMAX obj %lx\n", obj);
      //DebOut("data:        %lx\n", data);
      //DebOut("data->src:   %lx\n", data->src);

      lm->lm_MinMax.MinWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.MinHeight = data->height * RESIZE_Y;
      lm->lm_MinMax.DefWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.DefHeight = data->height * RESIZE_Y;
      lm->lm_MinMax.MaxWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.MaxHeight = data->height * RESIZE_Y;

      //DebOut("  mincw=%d\n",lm->lm_MinMax.MinWidth);
      //DebOut("  minch=%d\n",lm->lm_MinMax.MinHeight);
      //DebOut("  maxcw=%d\n",lm->lm_MinMax.MaxWidth);
      //DebOut("  maxch=%d\n",lm->lm_MinMax.MaxHeight);

      //DebOut("MUILM_MINMAX done\n");
    }
    return 0;

    case MUILM_LAYOUT: {
      struct Data *data = (struct Data *) hook->h_Data;
      ULONG i=0;
      Element *element=(Element *) data->hwnd;

      /* find our position */
      while(&WIN_RES[i] != element) {
        i++;
      }

      DebOut("MUILM_LAYOUT obj %lx\n", obj);
      DebOut("data:        %lx\n", data);

      /* all our lovely childs */
      i++;
      while(WIN_RES[i].exists && WIN_RES[i].windows_type != DIALOGEX) {
        DebOut("   obj[%d]: %p\n", i, WIN_RES[i].obj);
        if(WIN_RES[i].text) {
          DebOut("          : %s\n", WIN_RES[i].text);
        }
        DebOut("   idc: %d\n", WIN_RES[i].idc);
        DebOut("   x, y: %d, %d  w, h: %d, %d\n", WIN_RES[i].x, WIN_RES[i].y, WIN_RES[i].w, WIN_RES[i].h);
        if(WIN_RES[i].obj != obj) {
          if (!MUI_Layout(WIN_RES[i].obj,
                          WIN_RES[i].x * RESIZE_X,
                          WIN_RES[i].y * RESIZE_Y,
                          WIN_RES[i].w * RESIZE_X,
                          WIN_RES[i].h * RESIZE_Y, 0)
             ) {
            DebOut("MUI_Layout failed!\n");
            return FALSE;
          }
        }
        else {
          DebOut("   RECURSION !?!?!: %lx\n", WIN_RES[i].obj);
        }
        i++;
      }
      //DebOut("MUILM_LAYOUT done\n");
    }
    return TRUE;
  }

  return MUILM_UNKNOWN;
}

#ifdef UAE_ABI_v0
//#define ABI_CONST const
#define ABI_CONST 
#else
#define ABI_CONST 
#endif

static IPTR mNew(struct IClass *cl, APTR obj, Msg msg) {

  struct TagItem *tstate, *tag;
  ULONG i=0;
  struct Element *elem=NULL;
  Object *child       =NULL; /* TODO: remove me */
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
  func=NULL;

  DebOut("mNew(fixed)\n");

  obj = (APTR) DoSuperMethodA(cl, (Object*) obj, msg);

  if (!obj) {
    DebOut("fixed: unable to create object!");
    return (IPTR) NULL;
  }

  tstate=((struct opSet *)msg)->ops_AttrList;

  while ((tag = (struct TagItem *) NextTagItem((ABI_CONST TagItem**) &tstate))) {
    DebOut("tag->ti_Tag: %lx\n", tag->ti_Tag);
    switch (tag->ti_Tag) {
      case MA_src:
        elem = (Element *) tag->ti_Data;
        break;
      case MA_dlgproc:
        DebOut("MA_dlgproc: %lx\n", tag->ti_Data);
        func=(int* (*)(Element*, uint32_t, ULONG, IPTR)) tag->ti_Data;
        break;
    }
  }

  if(!elem) {
    DebOut("mNew: MA_src not supplied!\n");
    return (IPTR) NULL;
  }

  /* place vertically */
  DoMethod((Object *) obj, MUIM_Set, MUIA_Group_Horiz, FALSE);

  DebOut("receive: %p\n", elem);
  {
    GETDATA;
    //struct TextAttr ta;
    //struct TextFont *old;

    data->width =396;
    data->height=320;
    data->hwnd  =elem;
    data->func  =func;

    DebOut("YYYYYYYYYYYYYYYYYY\n");

    struct TextAttr ta = { (STRPTR)"Vera Sans", 12, 0, 0 };
    TextFont *Topaz8Font = OpenDiskFont(&ta);
    DebOut("Topaz8Font: %lx\n", Topaz8Font);
#if 0
    foo=TextObject, MUIA_Text_Contents, (IPTR) "XYZ", End;
    old=(struct TextFont *) XGET(foo, MUIA_Font);
    //GetAttr(MUIA_Font, (Object *)foo, (IPTR *)old);
    DebOut("MUIA_Font: %s\n", old->tf_Message.mn_Node.ln_Name);
    DebOut("MUIA_Font: %d\n", old->tf_YSize);
    DebOut("YYYYYYYYYYYYYYYYYY\n");


    GetAttr(MUIA_Font, (Object *)obj, (IPTR *) &old);
    ta.ta_Name = GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
    ta.ta_YSize = GfxBase->DefaultFont->tf_YSize;
    ta.ta_Style = GfxBase->DefaultFont->tf_Style;
    ta.ta_Flags = GfxBase->DefaultFont->tf_Flags;
    DebOut("OpenFont: %s\n", ta.ta_Name);
    data->font=OpenDiskFont(&ta);
    DebOut("data->font: %lx\n", data->font);
#endif


    DebOut("XXX add childs to our new class XXX\n");

    /* find start of our children */
    i=0;
    while(WIN_RES[i].idc && &WIN_RES[i]!=elem) {
      i++;
    }
    i++;

    while(WIN_RES[i].idc && WIN_RES[i].windows_type!=DIALOGEX) {
      DebOut("========== i=%d idc: %d =========\n", i, WIN_RES[i].idc);
      if(WIN_RES[i].text) {
        DebOut("add \"%s\"\n", WIN_RES[i].text);
      }
      switch(WIN_RES[i].windows_type) {
        case GROUPBOX:
          child=HGroup, MUIA_Frame, MUIV_Frame_Group,
                              MUIA_FrameTitle, WIN_RES[i].text,
                              End;
          WIN_RES[i].exists=TRUE;
          WIN_RES[i].obj=child;
        break;

        case CONTROL:
          if(!strcmp(WIN_RES[i].windows_class, "msctls_trackbar32")) {
            /* Proportional/slider gadget! */
            child=VGroup,
                    MUIA_UserData         , i,
                    Child, VSpace(0),
                    Child, SliderObject,
                      MUIA_UserData         , i,
                      MUIA_Group_Horiz, TRUE,
                      MUIA_Slider_Quiet, TRUE,
#if 0
                      MUIA_Numeric_Min, 0,
                      MUIA_Numeric_Max, 10,
                      MUIA_Numeric_Value, 0,
#endif
                      MUIA_Weight, 0,
                    End,
                    Child, VSpace(0),
                  End;

            if(child) {
              WIN_RES[i].exists=TRUE;
              WIN_RES[i].obj=child;
              data->MyMUIHook_slide.h_Entry=(APTR) MUIHook_slide;
              data->MyMUIHook_slide.h_Data =(APTR) data;

              DebOut("DoMethod(%lx, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime..)\n", WIN_RES[i].obj);
              DoMethod(WIN_RES[i].obj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR) WIN_RES[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_slide, func); 
 
            }
            break;
          }
          if(!strcmp(WIN_RES[i].windows_class, "SysTreeView32")) {
            /* Tree gadget! */
            DebOut("SysTreeView32\n");
            Object *nlisttree=NULL;
            child=new_tree(i, (void *) func, data, &nlisttree); 
            DebOut("child: %lx\n", child);
            if(child) {
              WIN_RES[i].exists=TRUE;
              WIN_RES[i].obj=nlisttree;
            }
            break;
          }

          if(!strcmp(WIN_RES[i].windows_class, "SysListView32")) {
            /* Tree gadget! */
            DebOut("SysListView32\n");
            Object *list=NULL;

            child=new_listview(WIN_RES, i, (void *) func, data, &list); 
            DebOut("child: %lx\n", child);
            if(child) {
              WIN_RES[i].exists=TRUE;
              WIN_RES[i].obj=child;
              WIN_RES[i].action=list;
            }
            break;
          }
          if(WIN_RES[i].windows_class==NULL || !strcmp(WIN_RES[i].windows_class, "Button") || /* TODO: */ !strcmp(WIN_RES[i].windows_class, "Static") ) {
            DebOut("Button found\n");
            if(WIN_RES[i].flags2==BS_AUTORADIOBUTTON) {
              child=HGroup,
                    MUIA_UserData         , i,
                    Child, WIN_RES[i].obj=ImageObject,
                        NoFrame,
                        //MUIA_CycleChain       , 1,
                        MUIA_InputMode        , MUIV_InputMode_Toggle,
                        MUIA_Image_Spec       , MUII_RadioButton,
                        MUIA_ShowSelState     , FALSE,
                        MUIA_Selected         , FALSE,
                        MUIA_UserData         , i,
                      End,
                    Child, TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_Contents, WIN_RES[i].text,
                        MUIA_UserData         , i,
                      End,

                  End;

              data->MyMUIHook_select.h_Entry=(APTR) MUIHook_select;
              data->MyMUIHook_select.h_Data =(APTR) data;

              DoMethod(WIN_RES[i].obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) WIN_RES[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_select, func); 

            }
            else {
              if(WIN_RES[i].text && (WIN_RES[i].text[0]!=(char) 0)) {
                child=HGroup,
                  MUIA_UserData         , i,
                  Child, WIN_RES[i].obj=ImageObject,
                    ImageButtonFrame,
                    MUIA_InputMode   , MUIV_InputMode_Toggle,
                    MUIA_Image_Spec  , MUII_CheckMark,
                    MUIA_Background  , MUII_ButtonBack,
                    MUIA_ShowSelState, FALSE,
                    //MUIA_CycleChain  , TRUE,
                    MUIA_UserData         , i,
                  End,
                  Child, TextObject,
                    MUIA_Font, Topaz8Font,
                    MUIA_Text_Contents, WIN_RES[i].text,
                    MUIA_UserData         , i,
                    End,
                End;
              }
              else {
                /* button without text must be without text, otherwise zune draws "over the border" */
                child=HGroup,
                  MUIA_UserData         , i,
                  Child, WIN_RES[i].obj=ImageObject,
                    ImageButtonFrame,
                    MUIA_InputMode   , MUIV_InputMode_Toggle,
                    MUIA_Image_Spec  , MUII_CheckMark,
                    MUIA_Background  , MUII_ButtonBack,
                    MUIA_ShowSelState, FALSE,
                    //MUIA_CycleChain  , TRUE,
                    MUIA_UserData         , i,
                  End,
                End;
              }
              data->MyMUIHook_select.h_Entry=(APTR) MUIHook_select;
              data->MyMUIHook_select.h_Data =(APTR) data;

              DebOut("DoMethod(%lx, MUIM_Notify, MUIA_Selected, MUIV_EveryTime..)\n", WIN_RES[i].obj);
              DoMethod(WIN_RES[i].obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) WIN_RES[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_select, func); 
            }
          }
          else if(!strcmp(WIN_RES[i].windows_class, "RICHEDIT")) {
            DebOut("RICHEDIT found (text: %s)\n", WIN_RES[i].text);
            child=NULL;
#ifndef DONT_USE_URLOPEN
            child=UrltextObject, 
                    MUIA_Font, Topaz8Font,
                    MUIA_Text_PreParse, "\33c",
                    MUIA_Urltext_Text, WIN_RES[i].text, 
                    MUIA_Urltext_Url, "http://", 
                    MUIA_UserData, i,
                    End;
#endif
            if(!child) {
              DebOut("Urltext.mcc not used/installed, use normal text\n");
              
              child=TextObject,
                    MUIA_Font, Topaz8Font,
                    MUIA_Text_PreParse, "\33c",
                    MUIA_Text_Contents, WIN_RES[i].text,
                    MUIA_UserData         , i,
                  End;
            }
          }
          else {
            fprintf(stderr, "ERROR: Unknown windows class \"%s\" found!\n", WIN_RES[i].windows_class);
            DebOut("ERROR: Unknown windows class \"%s\" found!\n", WIN_RES[i].windows_class);
            DebOut("We *will* crash on this!!\n");
          }
          if(child) {
            WIN_RES[i].exists=TRUE;
            WIN_RES[i].obj=child;
          }
        break;

        case DEFPUSHBUTTON:
        case PUSHBUTTON:
          child=HGroup, MUIA_Background, MUII_ButtonBack,
                              ButtonFrame,
                              MUIA_InputMode , MUIV_InputMode_RelVerify,
                              Child, TextObject,
                                MUIA_UserData         , i,
                                MUIA_Font, Topaz8Font,
                                MUIA_Text_PreParse, "\33c",
                                MUIA_Text_Contents, (IPTR) WIN_RES[i].text,
                              End,
                            End;

          WIN_RES[i].exists=TRUE;
          WIN_RES[i].obj=child;
          /* Add hook */
          data->MyMUIHook_pushbutton.h_Entry=(APTR) MUIHook_pushbutton;
          data->MyMUIHook_pushbutton.h_Data =(APTR) data;
          DoMethod(WIN_RES[i].obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) WIN_RES[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_pushbutton, func); 
        break;

        case RTEXT:
          child=TextObject,
                        //MUIA_Background, MUII_MARKBACKGROUND,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33r",
                        MUIA_Text_Contents, (IPTR) WIN_RES[i].text,
                      End;
          WIN_RES[i].exists=TRUE;
          WIN_RES[i].obj=child;
        break;

        case LTEXT:
          child=TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33l",
                        MUIA_Text_Contents, (IPTR) WIN_RES[i].text,
                      End;
          WIN_RES[i].exists=TRUE;
          WIN_RES[i].obj=child;
        break;


        case EDITTEXT:
          if(WIN_RES[i].flags & ES_READONLY) {
            child=TextObject,
                        TextFrame,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, (IPTR) WIN_RES[i].text,
                      End;
            WIN_RES[i].obj=child;
          }
          else {
            child=StringObject,
                          StringFrame,
                          MUIA_Font, Topaz8Font,
                          MUIA_String_Contents, (IPTR) WIN_RES[i].text,
                        End;
            WIN_RES[i].obj=child;
          }
          WIN_RES[i].exists=TRUE;
          WIN_RES[i].var=(char **) calloc(256, sizeof(IPTR)); // array for cycle texts
          WIN_RES[i].var[0]=NULL;
            data->MyMUIHook_entry.h_Entry=(APTR) MUIHook_entry;
            data->MyMUIHook_entry.h_Data =(APTR) data;
            DoMethod(WIN_RES[i].obj, MUIM_Notify,  MUIA_String_Contents , MUIV_EveryTime, (IPTR) WIN_RES[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_entry, func); 

        break;


        case COMBOBOX:
          WIN_RES[i].mem=(char **) calloc(256, sizeof(IPTR)); // array for cycle texts
          WIN_RES[i].mem[0]=NULL; // always terminate
          if(!flag_editable(WIN_RES[i].flags)) {
            DebOut("not editable => Cycle Gadget\n");
            child=CycleObject,
                         MUIA_Font, Topaz8Font,
                         MUIA_Cycle_Entries, WIN_RES[i].mem,  // start empty
                       End;
            WIN_RES[i].exists=TRUE;
            WIN_RES[i].obj=child;
            data->MyMUIHook_combo.h_Entry=(APTR) MUIHook_combo;
            data->MyMUIHook_combo.h_Data =(APTR) data;
            DoMethod(WIN_RES[i].obj, MUIM_Notify, MUIA_Cycle_Active , MUIV_EveryTime, (IPTR) WIN_RES[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_combo, func); 
          }
          else {
            DebOut("Editable => use own Combo class\n");
            child=(Object *) NewObject(CL_Combo->mcc_Class, NULL,TAG_DONE);
            WIN_RES[i].exists=TRUE;
            WIN_RES[i].obj=child;
            DoMethod(child, MUIM_List_Insert, WIN_RES[i].mem);
          }
        break;


        default:
          DebOut("ERROR: unknown windows_type %d\n", WIN_RES[i].windows_type);
      }
      DebOut("RESULT: child:   %lx\n", child);
      DebOut("RESULT: src.obj: %lx\n", WIN_RES[i].obj);
      if(child) {
        if(WIN_RES[i].help) {
          DebOut("add HOTHELP: %s\n", WIN_RES[i].help);
          DoMethod(child, MUIM_Set, MUIA_ShortHelp, (IPTR) WIN_RES[i].help);
        }
        if(!(WIN_RES[i].flags & WS_VISIBLE)) {
          DoMethod(child, MUIM_Set, MUIA_ShowMe, (IPTR) 0);
        }
        /*
         * bad idea?
        if(WIN_RES[i].flags & WS_DISABLED) {
          DebOut("WS_DISABLED!\n");
          DoMethod(child, MUIM_Set, MUIA_Disabled, TRUE);
        }
        */
        if(WIN_RES[i].flags & WS_TABSTOP) {
          DoMethod(child, MUIM_Set, MUIA_CycleChain, (IPTR) 1);
        }
        child=NULL;
      }
      DebOut("  WIN_RES[%d].obj=%lx\n", i, WIN_RES[i].obj);
      i++;

    }

    //SETUPHOOK(&data->LayoutHook, LayoutHook, data);
    data->LayoutHook.h_Entry=(APTR) LayoutHook;
    data->LayoutHook.h_Data=data;

    /*
     * ATTENTION: Add children *after* setting the LayoutHook,
     *            otherwise we get recursions!
     */
  
    DoMethod((Object *) obj, MUIM_Set, MUIA_Group_LayoutHook, &data->LayoutHook);

    /* find start of our children */
    i=0;
    while(WIN_RES[i].idc && &WIN_RES[i]!=elem) {
      i++;
    }
    i++;
    while(WIN_RES[i].idc && WIN_RES[i].windows_type!=DIALOGEX) {
      DebOut("i: %d (add %lx to %lx)\n", i, WIN_RES[i].obj, obj);
      DoMethod((Object *) obj, OM_ADDMEMBER,(IPTR) WIN_RES[i].obj);
      i++;
    }

    mSet(data, obj, (struct opSet *) msg, 1);
    DoMethod((Object *) obj, MUIM_Set, MUIA_Frame, MUIV_Frame_Group);
  }
  return (IPTR)obj;
}

static IPTR mGet(struct Data *data, APTR obj, struct opGet *msg, struct IClass *cl)
{
  IPTR rc;

  //DebOut("mGet..\n");

  switch (msg->opg_AttrID)
  {
    case MA_Data: 
      DebOut("data: %lx\n", data);
      rc = (IPTR) data; 
      break;
    case MA_dlgproc:
      DebOut("data: %lx\n", data);
      rc = (IPTR) data->func; 
      break;
    case MA_src:
      rc = (IPTR) data->hwnd; 
      break;

    default: return DoSuperMethodA(cl, (Object *) obj, (Msg)msg);
  }

  *msg->opg_Storage = rc;

  return TRUE;
}

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new)
{
  struct TagItem *tstate, *tag;

  tstate = msg->ops_AttrList;

  while ((tag = NextTagItem((ABI_CONST TagItem**)&tstate))) {

    switch (tag->ti_Tag) {
//      case MA_Fixed_Move:
 //       fixed_move(data,obj,(struct MA_Fixed_Move_Data *)tag->ti_Data);
  //      break;
    }
  }
}

static VOID mRemMember(struct Data *data, struct opMember *msg) {
#if 0
  struct FixedNode *node;

  ForeachNode(&data->ChildList, node) {
    if (GtkObj(node->widget) == msg->opam_Object)
    {
      REMOVE(node);
      mgtk_freemem(node, sizeof(*node));
      return;
    }
  }
#endif
}

BEGINMTABLE
GETDATA;

//DebOut("(%lx)->msg->MethodID: %lx\n",obj,msg->MethodID);


  switch (msg->MethodID)
  {
    case OM_NEW         : return mNew           (cl, obj, msg);
  	case OM_SET         :        mSet           (data, obj, (opSet *)msg, FALSE); break;
    case OM_GET         : return mGet           (data, obj, (opGet *)msg, cl);
    case OM_REMMEMBER   :        mRemMember     (data,      (opMember *)msg); break;
  }

ENDMTABLE

int init_class(void) {

  if(!CL_Fixed) {
    CL_Fixed = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct Data), (APTR)&mDispatcher);
  }

  create_combo_class();

  return CL_Fixed ? 1 : 0;
}

void delete_class(void) {

  if (CL_Fixed) {
    MUI_DeleteCustomClass(CL_Fixed);
    CL_Fixed=NULL;
  }
  delete_combo_class();
}



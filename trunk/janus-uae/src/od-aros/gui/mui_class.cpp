
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


#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "aros.h"
#include "gui.h"
#include "winnt.h"
#include "mui_data.h"
#include "combo.h"

//#define RESIZE 155 / 100
#define RESIZE_X 150 /100
#define RESIZE_Y 140 /100

#define MAX_ENTRIES 255

//#define EMPTY_SELECTION "<-- none -->"
#define EMPTY_SELECTION ""

struct Data {
  struct Hook LayoutHook;
  struct Hook MyMUIHook_pushbutton;
  struct Hook MyMUIHook_select;
  struct Hook MyMUIHook_slide;
  struct Hook MyMUIHook_combo;
  struct Hook MyMUIHook_tree_active;
  ULONG width, height;
  struct Element *src;
  struct TextFont *font;
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
};

static const char *Cycle_Dummy[] = { NULL };

struct ReqToolsBase *ReqToolsBase=NULL;
struct MUI_CustomClass *CL_Fixed;

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new);

extern Object *win;

#if (0)
IPTR xget(Object *obj, IPTR attr) {
  IPTR b = 0;
  GetAttr(attr, obj, (IPTR *)&b);
  return b;
}
#endif

/* return index in elem[] for item IDC_* */
LONG get_index(Element *elem, int item) {
  int i=0;

  if(!elem) {
    return -1;
  }

  while(elem[i].exists) {
    if(elem[i].idc == item) {
      return i;
    }
    i++;
  }

  DebOut("item %d (0x%lx) not found in element %lx!!\n", item, item, elem);
  return -1;
}

/* return elem of item IDC_* */
Element *get_elem(int nIDDlgItem) {
  ULONG i=0;
  int res;

  while(IDD_ELEMENT[i]!=NULL) {
    res=get_index(IDD_ELEMENT[i], nIDDlgItem);
    if(res!=-1) {
      DebOut("found %d in Element number %d\n", nIDDlgItem, i);
      return IDD_ELEMENT[i];
    }
    i++;
  }
  
  return NULL;
}

BOOL flag_editable(ULONG flags) {

  if((flags & CBS_DROPDOWNLIST)==CBS_DROPDOWNLIST) {
    return FALSE;
  }
  return TRUE;
}

/*************************************
 * get index of Zune object
 *************************************/
static int get_elem_from_obj(struct Data *data, Object *obj) {

  int i=0;

  //DebOut("obj: %lx, data %lx\n", obj, data);

  /* first try userdata */
  i=XGET(obj, MUIA_UserData);
  if(i>0) {
    //DebOut("MUIA_UserData: %d\n", i);
    return i;
  }

  i=0;
  while(data->src[i].exists) {
    if(data->src[i].obj == obj) {
      //DebOut("found! i: %d\n", i);
      return i;
    }
    i++;
  }

  DebOut("ERROR: obj %lx, data %lx not found!\n", obj, data);
  return -1;
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

  int i;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;


  //DebOut("[%lx] entered\n", obj);
  //DebOut("[%lx] hook.h_Data: %lx\n", obj, hook->h_Data);
  //DebOut("[%lx] obj: %lx\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  //DebOut("[%lx] i: %d\n", obj, i);

  data->src[i].value=XGET((Object *) obj, MUIA_Cycle_Active);
  //DebOut("[%lx] MUIA_Cycle_Active: %d (mui obj: %lx)\n", obj, data->src[i].value, obj);

  DebOut("[%lx] state: %d\n", obj, data->src[i].value);

  if(data->func) {
    if(data->src[i].mem[data->src[i].value] && !strcmp(data->src[i].mem[data->src[i].value], EMPTY_SELECTION)) {
      DebOut("[%lx] Empty selection (%s), do nothing\n", obj, EMPTY_SELECTION);
    }
    else {
      if(flag_editable(data->src[i].flags)) {
        data->src[i].value--;
      }
      wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
      DebOut("[%lx] call function: %lx(IDC %d, CBN_SELCHANGE)\n", obj, data->func, data->src[i].idc);
      data->func(data->src, WM_COMMAND, wParam, NULL);
    }
  }
  else {
    DebOut("[%lx] function is zero: %lx\n", obj, data->func);
  }

  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, MUIHook_slide, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT

  struct Data *data = (struct Data *) hook->h_Data;
  int i;
  ULONG wParam;

  DebOut("Sliding..\n");

  i=get_elem_from_obj(data, (Object *) obj);

  if(data->func) {
#if 0
    wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
    DebOut("call function: %lx (IDC %d, wParam: %lx)\n", data->func, data->src[i].idc, wParam);
    data->func(data->src, WM_COMMAND, wParam, NULL);
#endif

    /* warning: should we call BN_CLICKED here, too?
     * open console in Paths won't work without .. 
     */
    wParam=MAKELPARAM(data->src[i].idc, BN_CLICKED);
    data->func(data->src, WM_COMMAND, wParam, NULL);

    data->func(data->src, WM_HSCROLL, NULL, NULL);

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

  int i;
  ULONG t;
  ULONG wParam;
  ULONG newstate;

  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("entered (hook.h_Data: %lx)\n", hook->h_Data);

  i=get_elem_from_obj(data, (Object *) obj);

  if(data->src[i].text) {
    DebOut("obj: %lx => i: %d (%s)\n", obj, i, data->src[i].text);
  }
  else {
    DebOut("obj: %lx => i: %d\n", obj, i);
  }

  newstate=XGET((Object *) obj, MUIA_Selected);
  DebOut("MUIA_Selected is (now): %d (group: %d)\n", newstate, data->src[i].group);

  if((data->src[i].windows_type==CONTROL) && 
     (data->src[i].flags2==BS_AUTORADIOBUTTON) && 
     (data->src[i].group)) {

    /*** AUTORADIOBUTTON / Radio button logic: ***/

    if(data->src[i].value == 1) {
      /* we were already the active one, we want to stay that, too! */
      DebOut("we were already the active one!\n");
      DoMethod(data->src[i].obj, MUIM_NoNotifySet, MUIA_Selected, TRUE);
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
    while(data->src[t].exists) {
      if(data->src[t].value && data->src[i].group == data->src[t].group && t!=i) {
        /* same group */
        DebOut("  => also activated, same group, other guy!\n");
        DoMethod(data->src[t].obj, MUIM_NoNotifySet, MUIA_Selected, FALSE); 
        data->src[t].value=0;
      }
      t++;
    }
  }
  else if(data->src[i].windows_type==COMBOBOX) {

    /*** COMBOBOX ***/

    /* CBN_SELCHANGE: A new combo box list item is selected. */
    if(data->func) {
      wParam=MAKELPARAM(data->src[i].idc, CBN_SELCHANGE);
      DebOut("call function: %lx (IDC %d, wParam: %lx)\n", data->func, data->src[i].idc, wParam);
      data->func(data->src, WM_COMMAND, wParam, NULL);
    }
    goto DONE;
  }
  data->src[i].value=newstate;


DONE:
  if(data->func) {
    /* warning: should we call BN_CLICKED here, too?
     * open console in Paths won't work without .. 
     */
    wParam=MAKELPARAM(data->src[i].idc, BN_CLICKED);
    data->func(data->src, WM_COMMAND, wParam, NULL);
  }
  else {
    DebOut("WARNING: function is zero: %lx\n", data->func);
    /* Solution: add DlgProc in mNew in mui_head.cpp */
  }

  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, MUIHook_pushbutton, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT
  int i;
  ULONG wParam;

  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("[%lx] entered\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  DebOut("i: %d\n", i);

  if(data->func) {
    DebOut("call function: %lx\n", data->func);
    DebOut("IDC: %d\n", data->src[i].idc);
    DebOut("WM_COMMAND: %d\n", WM_COMMAND);

    wParam=MAKELPARAM(data->src[i].idc, BN_CLICKED);
    data->func(data->src, WM_COMMAND, wParam, NULL);
  }
  else {
    DebOut("function is zero: %lx\n", data->func);
  }

  AROS_USERFUNC_EXIT
}

AROS_UFH2(void, tree_active, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, obj, A2)) {

  AROS_USERFUNC_INIT
  int i;
  ULONG lParam;
  ULONG state;
  LPNMHDR nm=NULL;

  struct Data *data = (struct Data *) hook->h_Data;

  DebOut("[%lx] entered\n", obj);
  DebOut("[%lx] hook.h_Data: %lx\n", obj, hook->h_Data);
  DebOut("[%lx] obj: %lx\n", obj);

  i=get_elem_from_obj(data, (Object *) obj);

  //data->src[i].value=XGET((Object *) obj, MUIA_Cycle_Active);
 
  if(data->func) {
    state=XGET((Object *) obj, MUIA_NList_Active);
    if(state!=MUIV_NList_Active_Off) {
      /* LPNMHDR is sent in wParam */
      nm=(LPNMHDR) malloc(sizeof(NMHDR));
      nm->idFrom=NULL; /* not used by WinUAE! */
      nm->code=TVN_SELCHANGED;
      nm->hwndFrom=data->src[i].idc;

      /* LPNMTREEVIEW is sent in lParam */
      TODO();
      /* lParam needs to hold a pointer to the ConfigStruct of the selected item !? */
      //lParam=(LPARAM) configstruct;
      lParam=NULL;


      DebOut("[%lx] call function: %lx(IDC %d, WM_NOTIFY)\n", obj, data->func, data->src[i].idc);
      data->func(data->src, WM_NOTIFY, NULL, (IPTR) nm);
    }
    else {
      DebOut("MUIV_List_Active_Off? What to do now!?\n");
    }
  }
  else {
    DebOut("[%lx] function is zero: %lx\n", obj, data->func);
  }

  if(nm) free(nm);

  AROS_USERFUNC_EXIT
}
 
/*****************************************************************************
 * Class
 *****************************************************************************/
AROS_UFH3(static ULONG, LayoutHook, AROS_UFHA(struct Hook *, hook, a0), AROS_UFHA(APTR, obj, a2), AROS_UFHA(struct MUI_LayoutMsg *, lm, a1)) 

  switch (lm->lm_Type) {
    case MUILM_MINMAX: {

      struct Data *data = (struct Data *) hook->h_Data;
      ULONG i=0;
      struct Element *element=data->src;
      ULONG mincw, minch, defcw, defch, maxcw, maxch;

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
      struct Element *element=data->src;

      //DebOut("MUILM_LAYOUT obj %lx\n", obj);
      //DebOut("data:        %lx\n", data);
      //DebOut("data->src:   %lx\n", data->src);

      while(element[i].exists) {
        //DebOut("  child i=%d\n", i);
        //DebOut("   obj[%d]: %lx\n", i, element[i].obj);
        //DebOut("   idc: %d (dummy is %d)\n", element[i].idc, IDC_dummy);
        //DebOut("   x, y: %d, %d  w, h: %d, %d\n", element[i].x, element[i].y, element[i].w, element[i].h);
        if(element[i].obj != obj) {
          if (!MUI_Layout(element[i].obj,
                          element[i].x * RESIZE_X,
                          element[i].y * RESIZE_Y,
                          element[i].w * RESIZE_X,
                          element[i].h * RESIZE_Y, 0)
             ) {
            DebOut("MUI_Layout failed!\n");
            return FALSE;
          }
        }
        else {
          DebOut("   RECURSION !?!?!: %lx\n", element[i].obj);
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
#define ABI_CONST const
#else
#define ABI_CONST 
#endif
static IPTR mNew(struct IClass *cl, APTR obj, Msg msg) {

  struct TagItem *tstate, *tag;
  ULONG i=0;
  IPTR s=0;
  struct Element *src=NULL;
  Object *child      =NULL; /* TODO: remove me */
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
        s = tag->ti_Data;
        break;
      case MA_dlgproc:
        DebOut("MA_dlgproc: %lx\n", tag->ti_Data);
        func=(int* (*)(Element*, uint32_t, ULONG, IPTR)) tag->ti_Data;
        break;
    }
  }

  if(!s) {
    DebOut("mNew: MA_src not supplied!\n");
    return (IPTR) NULL;
  }

  /* place vertically */
  DoMethod(obj, MUIM_Set, MUIA_Group_Horiz, FALSE);

  src=(struct Element *) s;

  DebOut("receive: %lx\n", s);
  {
    GETDATA;
    //struct TextAttr ta;
    struct TextFont *old;
    Object *foo;

    data->width =396;
    data->height=320;
    data->src   =src;
    data->func  =func;

    DebOut("YYYYYYYYYYYYYYYYYY\n");

    struct TextAttr ta = { "Vera Sans", 12, 0, 0 };
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


    DebOut("XXXXXXXXXXXXXXXXXXXXXX\n");
    i=0;
    while(src[i].windows_type) {
      DebOut("========== i=%d =========\n", i);
      DebOut("add %s\n", src[i].text);
      switch(src[i].windows_type) {
        case GROUPBOX:
          child=HGroup, MUIA_Frame, MUIV_Frame_Group,
                              MUIA_FrameTitle, src[i].text,
                              End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;

        case CONTROL:
          if(!strcmp(src[i].windows_class, "msctls_trackbar32")) {
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
              src[i].exists=TRUE;
              src[i].obj=child;
#ifdef UAE_ABI_v0
              data->MyMUIHook_slide.h_Entry=(HOOKFUNC) MUIHook_slide;
#else
              data->MyMUIHook_slide.h_Entry=(APTR) MUIHook_slide;
#endif
              data->MyMUIHook_slide.h_Data =(APTR) data;

              DebOut("DoMethod(%lx, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime..)\n", src[i].obj);
              DoMethod(src[i].obj, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_slide, func); 
 
            }
            break;
          }
          if(!strcmp(src[i].windows_class, "SysTreeView32")) {
            /* Tree gadget! */
            DebOut("SysTreeView32\n");
            child=HGroup,
                    MUIA_UserData         , i,
                    //Child, VSpace(0),
                    Child, (NListviewObject,
                      MUIA_UserData         , i,
                      MUIA_NListview_NList, (NListtreeObject,
                        MUIA_UserData         , i,
                        ReadListFrame,
                        MUIA_CycleChain, TRUE,
                        //MUIA_NListtree_ConstructHook, (IPTR)&data->MyMUIHook_tree_construct,
                        //MUIA_NListtree_DestructHook,  (IPTR)&data->MyMUIHook_tree_destruct,
                        //LISTV_EntryArray, bla,
                        MUIA_NListtree_DragDropSort,  FALSE,
                        End),
                        End),
                    End;
                    //Child, VSpace(0),
            DebOut("child: %lx\n", child);
            if(child) {
              src[i].exists=TRUE;
              src[i].obj=child;

#ifdef UAE_ABI_v0
              data->MyMUIHook_tree_active.h_Entry=(HOOKFUNC) tree_active;
#else
              data->MyMUIHook_tree_active.h_Entry=(APTR) tree_active;
#endif
              data->MyMUIHook_tree_active.h_Data =(APTR) data;

              DoMethod(src[i].obj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_tree_active, func); 
 
            }
            break;
          }

          if(src[i].windows_class==NULL || !strcmp(src[i].windows_class, "Button") || /* implement those: !!!  */ !strcmp(src[i].windows_class, "SysListView32") || !strcmp(src[i].windows_class, "Static") || !strcmp(src[i].windows_class, "SysTreeView32")) {
            DebOut("Button found\n");
            if(src[i].flags2==BS_AUTORADIOBUTTON) {
              child=HGroup,
                    MUIA_UserData         , i,
                    Child, src[i].obj=ImageObject,
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
                        MUIA_Text_Contents, src[i].text,
                        MUIA_UserData         , i,
                      End,

                  End;

#ifdef UAE_ABI_v0
              data->MyMUIHook_select.h_Entry=(HOOKFUNC) MUIHook_select;
#else
              data->MyMUIHook_select.h_Entry=(APTR) MUIHook_select;
#endif
              data->MyMUIHook_select.h_Data =(APTR) data;

              DoMethod(src[i].obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_select, func); 

            }
            else {
              if(src[i].text && (src[i].text[0]!=(char) 0)) {
                child=HGroup,
                  MUIA_UserData         , i,
                  Child, src[i].obj=ImageObject,
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
                    MUIA_Text_Contents, src[i].text,
                    MUIA_UserData         , i,
                    End,
                End;
              }
              else {
                /* button without text must be without text, otherwise zune draws "over the border" */
                child=HGroup,
                  MUIA_UserData         , i,
                  Child, src[i].obj=ImageObject,
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
#ifdef UAE_ABI_v0
              data->MyMUIHook_select.h_Entry=(HOOKFUNC) MUIHook_select;
#else
              data->MyMUIHook_select.h_Entry=(APTR) MUIHook_select;
#endif
              data->MyMUIHook_select.h_Data =(APTR) data;

              DebOut("DoMethod(%lx, MUIM_Notify, MUIA_Selected, MUIV_EveryTime..)\n", src[i].obj);
              DoMethod(src[i].obj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_select, func); 
            }
          }
          else if(!strcmp(src[i].windows_class, "RICHEDIT")) {
            DebOut("RICHEDIT found (text: %s\n", src[i].text);
            child=TextObject,
                    MUIA_Font, Topaz8Font,
                     MUIA_Text_PreParse, "\33c",
                    MUIA_Text_Contents, src[i].text,
                    MUIA_UserData         , i,
                  End;
          }
          else {
            fprintf(stderr, "ERROR: Unknown windows class \"%s\" found!\n", src[i].windows_class);
            DebOut("ERROR: Unknown windows class \"%s\" found!\n", src[i].windows_class);
            DebOut("We *will* crash on this!!\n");
          }
          if(child) {
            src[i].exists=TRUE;
            src[i].obj=child;
          }
        break;

        case PUSHBUTTON:
          child=HGroup, MUIA_Background, MUII_ButtonBack,
                              ButtonFrame,
                              MUIA_InputMode , MUIV_InputMode_RelVerify,
                              Child, TextObject,
                                MUIA_UserData         , i,
                                MUIA_Font, Topaz8Font,
                                MUIA_Text_PreParse, "\33c",
                                MUIA_Text_Contents, (IPTR) src[i].text,
                              End,
                            End;

          src[i].exists=TRUE;
          src[i].obj=child;
          /* Add hook */
#ifdef UAE_ABI_v0
          data->MyMUIHook_pushbutton.h_Entry=(HOOKFUNC) MUIHook_pushbutton;
#else
          data->MyMUIHook_pushbutton.h_Entry=(APTR) MUIHook_pushbutton;
#endif
          data->MyMUIHook_pushbutton.h_Data =(APTR) data;
          DoMethod(src[i].obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_pushbutton, func); 
        break;

        case RTEXT:
          child=TextObject,
                        //MUIA_Background, MUII_MARKBACKGROUND,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33r",
                        MUIA_Text_Contents, (IPTR) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;

        case LTEXT:
          child=TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33l",
                        MUIA_Text_Contents, (IPTR) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;


        case EDITTEXT:
          if(src[i].flags & ES_READONLY) {
            child=TextObject,
                        TextFrame,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, (IPTR) src[i].text,
                      End;
            src[i].obj=child;
          }
          else {
            child=StringObject,
                          StringFrame,
                          MUIA_Font, Topaz8Font,
                          MUIA_String_Contents, (IPTR) src[i].text,
                        End;
            src[i].obj=child;
          }
          src[i].exists=TRUE;
          src[i].var=(char **) malloc(256 * sizeof(IPTR)); // array for cycle texts
          src[i].var[0]=NULL;
        break;


        case COMBOBOX:
          src[i].mem=(char **) malloc(256 * sizeof(IPTR)); // array for cycle texts
          src[i].mem[0]=NULL; // always terminate
          if(!flag_editable(src[i].flags)) {
            DebOut("not editable => Cycle Gadget\n");
            child=CycleObject,
                         MUIA_Font, Topaz8Font,
                         MUIA_Cycle_Entries, src[i].mem,  // start empty
                       End;
            src[i].exists=TRUE;
            src[i].obj=child;
#ifdef UAE_ABI_v0
            data->MyMUIHook_combo.h_Entry=(HOOKFUNC) MUIHook_combo;
#else
            data->MyMUIHook_combo.h_Entry=(APTR) MUIHook_combo;
#endif
            data->MyMUIHook_combo.h_Data =(APTR) data;
            DoMethod(src[i].obj, MUIM_Notify, MUIA_Cycle_Active , MUIV_EveryTime, (IPTR) src[i].obj, 2, MUIM_CallHook,(IPTR) &data->MyMUIHook_combo, func); 
          }
          else {
            DebOut("Editable => use own Combo class\n");
            child=(Object *) NewObject(CL_Combo->mcc_Class, NULL,TAG_DONE);
            src[i].exists=TRUE;
            src[i].obj=child;
            DoMethod(child, MUIM_List_Insert, src[i].mem);
          }
        break;


        default:
          DebOut("ERROR: unknown windows_type %d\n", src[i].windows_type);
      }
      DebOut("RESULT: child:   %lx\n", child);
      DebOut("RESULT: src.obj: %lx\n", src[i].obj);
      if(child) {
        if(src[i].help) {
          DebOut("add HOTHELP: %s\n", src[i].help);
          DoMethod(child, MUIM_Set, MUIA_ShortHelp, (IPTR) src[i].help);
        }
        if(!(src[i].flags & WS_VISIBLE)) {
          DoMethod(child, MUIM_Set, MUIA_ShowMe, (IPTR) 0);
        }
        /*
         * bad idea?
        if(src[i].flags & WS_DISABLED) {
          DebOut("WS_DISABLED!\n");
          DoMethod(child, MUIM_Set, MUIA_Disabled, TRUE);
        }
        */
        if(src[i].flags & WS_TABSTOP) {
          DoMethod(child, MUIM_Set, MUIA_CycleChain, (IPTR) 1);
        }
        child=NULL;
      }
      DebOut("  src[%d].obj=%lx\n", i, src[i].obj);
      i++;

    }

    //SETUPHOOK(&data->LayoutHook, LayoutHook, data);
#ifdef UAE_ABI_v0
    data->LayoutHook.h_Entry=(HOOKFUNC) LayoutHook;
#else
    data->LayoutHook.h_Entry=(APTR) LayoutHook;
#endif
    data->LayoutHook.h_Data=data;

    /*
     * ATTENTION: Add children *after* setting the LayoutHook,
     *            otherwise we get recursions!
     */
  
    DoMethod(obj, MUIM_Set, MUIA_Group_LayoutHook, &data->LayoutHook);
    i=0;
    while(src[i].exists) {
      DebOut("i: %d (add %lx to %lx)\n", i, src[i].obj, obj);
      DoMethod((Object *) obj, OM_ADDMEMBER,(IPTR) src[i].obj);
      i++;
    }

    mSet(data, obj, (struct opSet *) msg, 1);
    DoMethod(obj, MUIM_Set, MUIA_Frame, MUIV_Frame_Group);
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
      rc = (IPTR) data->src; 
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



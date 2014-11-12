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

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "gui.h"
#include "mui_data.h"

//#define RESIZE 155 / 100
#define RESIZE_X 150 /100
#define RESIZE_Y 140 /100

#define MAX_ENTRIES 255

#define MA_Data 0xfece0000

struct Data {
  struct Hook LayoutHook;
  ULONG width, height;
  struct Element *src;
  struct TextFont *font;
};

static const char *Cycle_Dummy[] = { "empty 1", "empty 2", NULL };

struct MUI_CustomClass *CL_Fixed;

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new);

ULONG xget(Object *obj, ULONG attr) {
  ULONG b = 0;
  GetAttr(attr, obj, (IPTR *)&b);
  return b;
}

/* return index in elem[] for item IDC_* */
static LONG get_index(Element *elem, int item) {
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

  DebOut("item %d not found in element %lx!!\n", item, elem);
  return -1;
}

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


/*****************************************************************************
 * WinAPI
 *****************************************************************************/
BOOL SetDlgItemText(Element *elem, int item, TCHAR *lpString) {
  Object *obj;
  LONG i;

  DebOut("elem: %lx\n", elem);
  DebOut("item: %d\n", item);
  DebOut("string: %s\n", lpString);

  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n");
    return FALSE;
  }

  if(elem[i].windows_type==EDITTEXT) {
    SetAttrs(elem[i].obj, MUIA_String_Contents, lpString, TAG_DONE);
  }
  else {
    SetAttrs(elem[i].obj, MUIA_Text_Contents,   lpString, TAG_DONE);
  }

  return TRUE;
}

BOOL CheckDlgButton(Element *elem, int button, UINT uCheck) {
  LONG i;

  DebOut("elem: %lx\n", elem);
  DebOut("button: %d\n", button);
  DebOut("uCheck: %d\n", uCheck);

  i=get_index(elem, button);
  if(i<0) return FALSE;

  SetAttrs(elem[i].obj, MUIA_Selected, uCheck, TAG_DONE);

  return TRUE;

}

LONG SendDlgItemMessage(struct Element *elem, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) {
  Object *obj;
  ULONG i;
  ULONG l;

  DebOut("elem: %lx\n", elem);
  DebOut("nIDDlgItem: %d\n", nIDDlgItem);

  i=get_index(elem, nIDDlgItem);
  /* might be in a different element..*/
  if(i<0) {
    elem=get_elem(nIDDlgItem);
    i=get_index(elem, nIDDlgItem);
  }

  /* still not found !? */
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", nIDDlgItem);
    return FALSE;
  }
  DebOut("index: %d\n", i);
  DebOut("elem[i].var: %lx\n", elem[i].var);

  switch(Msg) {

    case CB_ADDSTRING:
      /* add string to popup window */
      DebOut("new string: %s\n", (TCHAR *) lParam);
      l=0;
      while(elem[i].var[l] != NULL) {
        DebOut("l: %d\n", l);
        l++;
      }
      /* found next free string */
      DebOut("  next free line: %d\n", l);
#warning TODO: free strings again!
      elem[i].var[l]=strdup((TCHAR *) lParam);
      elem[i].var[l+1]=NULL;
      DebOut("  obj: %lx\n", elem[i].obj);
      SetAttrs(elem[i].obj, MUIA_Cycle_Entries, (ULONG) elem[i].var, TAG_DONE);
      break;

    case CB_RESETCONTENT:
      /* delete all strings */
      DebOut("reset strings\n");
      l=0;
      /* free old strings */
      while(elem[i].var[l] != NULL) {
        free(elem[i].var[l]);
        elem[i].var[l]=NULL;
        l++;
      }
      SetAttrs(elem[i].obj, MUIA_Cycle_Entries, (ULONG) elem[i].var, TAG_DONE);
      break;
    case CB_FINDSTRING:
      /* return string index */
      DebOut("search for string >%s<\n", (TCHAR *) lParam);
      l=0;
      while(elem[i].var[l] != NULL) {
        DebOut("  compare with >%s<\n", elem[i].var[l]);
        if(!strcmp(elem[i].var[l], (TCHAR *) lParam)) {
          return l;
        }
        l++;
      }
      DebOut("return -1\n");
      return -1;

    default:
      DebOut("unkown Windows Message-ID: %d\n", Msg);
      return FALSE;
  }

  return TRUE;
}

UINT GetDlgItemText(HWND elem, int nIDDlgItem, TCHAR *lpString, int nMaxCount) {
  int i;
  UINT ret;
  char buffer[256]; // ATTENTION: this might crash badly..

  i=get_index(elem, nIDDlgItem);
  /* might be in a different element..*/
  if(i<0) {
    elem=get_elem(nIDDlgItem);
    i=get_index(elem, nIDDlgItem);
  }

  /* still not found !? */
  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", nIDDlgItem);
    return 0;
  }
  DebOut("index: %d\n", i);

  GetAttr(MUIA_String_Contents, elem[i].obj, (IPTR *) &buffer);

  strncpy(lpString, buffer, nMaxCount);

  return strlen(lpString);
}

/*
 * Sets the text of a control in a dialog box to the string representation of a specified integer value.
 */
BOOL SetDlgItemInt(HWND elem, int item, UINT uValue, BOOL bSigned) {
  Object *obj;
  LONG i;
  TCHAR tmp[64];

  DebOut("elem: %lx\n", elem);
  DebOut("item: %d\n", item);
  DebOut("value: %d\n", uValue);
  DebOut("bSigned: %d\n", bSigned);

  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n");
    return FALSE;
  }

  if(bSigned) {
    snprintf(tmp, 64, "-%d", uValue);
  }
  else {
    snprintf(tmp, 64, "%d", uValue);
  }

  DebOut("set to: %s\n", tmp);

  if(!elem[i].var) {
    /* array for text (only one line, but respect data structure here */
    elem[i].var=(char **) malloc(16 * sizeof(ULONG *)); 
    elem[i].var[0]=NULL;
  }

  if(elem[i].var[0]) {
    /* free old content */
    free(elem[i].var[0]);
  }

  elem[i].var[0]=strdup(tmp);
  elem[i].var[1]=NULL;

  if(elem[i].windows_type==EDITTEXT) {
    DebOut("elem[i].windows_type==EDITTEXT\n");
    SetAttrs(elem[i].obj, MUIA_String_Contents, elem[i].var[0], TAG_DONE);
  }
  else {
    DebOut("elem[i].windows_type!=EDITTEXT\n");
    SetAttrs(elem[i].obj, MUIA_Text_Contents, elem[i].var[0], TAG_DONE);
  }

  return TRUE;
}

/* Adds a check mark to (checks) a specified radio button in a group and removes 
 * a check mark from (clears) all other radio buttons in the group. 
 */
BOOL CheckRadioButton(HWND elem, int nIDFirstButton, int nIDLastButton, int nIDCheckButton) {
  int i;
  int e;
  i=nIDFirstButton;
  while(i<nIDLastButton) {
    e=get_index(elem, i);
    /* might be in a different element..*/
    if(e<0) {
      elem=get_elem(i);
      e=get_index(elem, i);
    }

    /* still not found !? */
    if(e<0) {
      DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", i);
    }
    else {
      DebOut("index: %d\n", e);
      SetAttrs(elem[e].obj, MUIA_Selected, FALSE, TAG_DONE);
    }
    i++;
  }

  e=get_index(elem, nIDCheckButton);
  /* might be in a different element..*/
  if(e<0) {
    elem=get_elem(i);
    e=get_index(elem, i);
  }

  /* still not found !? */
  if(e<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n", i);
  }
  else {
    DebOut("index: %d\n", e);
    SetAttrs(elem[e].obj, MUIA_Selected, TRUE, TAG_DONE);
  }
}

UINT IsDlgButtonChecked(HWND elem, int item) {
  int i;
  int res;

  DebOut("elem: %lx\n", elem);
  DebOut("item: %d\n", item);

  i=get_index(elem, item);
  if(i<0) {
    elem=get_elem(item);
    i=get_index(elem, item);
  }

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d found nowhere!?\n");
    return FALSE;
  }

  res=xget(elem[i].obj, MUIA_Selected);

  DebOut("res: %d\n", res);

  return res;
}

int MessageBox(HWND hWnd, TCHAR *lpText, TCHAR *lpCaption, UINT uType) {

  struct EasyStruct req;

  DebOut("MessageBox(): %s \n", lpText);

  if(!uType & MB_OK) {
    DebOut("WARNING: unsupported type: %lx\n", uType);
  }

  req.es_StructSize=sizeof(struct EasyStruct);
  req.es_TextFormat=lpText;
  if(lpCaption) {
    req.es_Title=lpCaption;
  }
  else {
    req.es_Title="Error";
  }
  req.es_GadgetFormat="Ok";

  EasyRequestArgs(NULL, &req, NULL, NULL );
}

/*
 * Retrieves a handle to a control in the specified dialog box.
 */
#if 0
HWND GetDlgItem(HWND hDlg, int nIDDlgItem) {
  int i;
  UINT ret;
  char buffer[256]; // ATTENTION: this might crash badly..

  i=get_index(hDlg, nIDDlgItem);

  if(i<0) {
    DebOut("ERROR: nIDDlgItem %d not found in %lx!?\n", nIDDlgItem, hDlg);
    return NULL;
  }
  DebOut("index: %d\n", i);

  return hDlg[i];
}


/* Changes the text of the specified window's title bar (if it has one). 
 * If the specified window is a control, the text of the control is changed.
 */
BOOL SetWindowText(HWND hDlg, TCHAR *lpString) {

  DebOut("lpString: %s\n", lpString);
  DebOut("hWnd: %lx\n", hWnd);
}
#endif
BOOL ShowWindow(HWND hWnd, int nCmdShow) {
  TODO();
}; 


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

      DebOut("MUILM_MINMAX obj %lx\n", obj);
      //DebOut("data:        %lx\n", data);
      //DebOut("data->src:   %lx\n", data->src);

      lm->lm_MinMax.MinWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.MinHeight = data->height * RESIZE_Y;
      lm->lm_MinMax.DefWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.DefHeight = data->height * RESIZE_Y;
      lm->lm_MinMax.MaxWidth  = data->width * RESIZE_X;
      lm->lm_MinMax.MaxHeight = data->height * RESIZE_Y;

      DebOut("  mincw=%d\n",lm->lm_MinMax.MinWidth);
      DebOut("  minch=%d\n",lm->lm_MinMax.MinHeight);
      DebOut("  maxcw=%d\n",lm->lm_MinMax.MaxWidth);
      DebOut("  maxch=%d\n",lm->lm_MinMax.MaxHeight);

      DebOut("MUILM_MINMAX done\n");
    }
    return 0;

    case MUILM_LAYOUT: {
      struct Data *data = (struct Data *) hook->h_Data;
      ULONG i=0;
      struct Element *element=data->src;

      DebOut("MUILM_LAYOUT obj %lx\n", obj);
      //DebOut("data:        %lx\n", data);
      //DebOut("data->src:   %lx\n", data->src);

      while(element[i].exists) {
        DebOut("  child i=%d\n", i);
        DebOut("   obj[%d]: %lx\n", i, element[i].obj);
        DebOut("   idc: %d (dummy is %d)\n", element[i].idc, IDC_dummy);
        DebOut("   x, y: %d, %d  w, h: %d, %d\n", element[i].x, element[i].y, element[i].w, element[i].h);
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
      DebOut("MUILM_LAYOUT done\n");
    }
    return TRUE;
  }

  return MUILM_UNKNOWN;
}

static ULONG mNew(struct IClass *cl, APTR obj, Msg msg) {

  struct TagItem *tstate, *tag;
  ULONG i;
  ULONG s;
  struct Element *src;
  Object *child; /* TODO: remove me */

  DebOut("mNew(fixed)\n");

  obj = (APTR) DoSuperMethodA(cl, (Object*) obj, msg);

  if (!obj) {
    DebOut("fixed: unable to create object!");
    return (ULONG) NULL;
  }

  tstate=((struct opSet *)msg)->ops_AttrList;

  while ((tag = (struct TagItem *) NextTagItem((const TagItem**) &tstate))) {
    switch (tag->ti_Tag) {
      case MA_src:
        s = tag->ti_Data;
        break;
    }
  }

  if(!s) {
    DebOut("mNew: MA_src not supplied!\n");
    return (ULONG) NULL;
  }

  /* place vertically */
  SetAttrs(obj, MUIA_Group_Horiz, FALSE, TAG_DONE);

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

    DebOut("YYYYYYYYYYYYYYYYYY\n");

    struct TextAttr ta = { "Vera Sans", 12, 0, 0 };
    TextFont *Topaz8Font = OpenDiskFont(&ta);
    DebOut("Topaz8Font: %lx\n", Topaz8Font);
#if 0
    foo=TextObject, MUIA_Text_Contents, (ULONG) "XYZ", End;
    old=(struct TextFont *) xget(foo, MUIA_Font);
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
          if(src[i].windows_class==NULL || !strcmp(src[i].windows_class, "Button") || /* implement those: !!!  */ !strcmp(src[i].windows_class, "SysListView32") || !strcmp(src[i].windows_class, "msctls_trackbar32") || !strcmp(src[i].windows_class, "Static") || !strcmp(src[i].windows_class, "SysTreeView32")) {
            DebOut("Button found\n");
            if(src[i].flags2==BS_AUTORADIOBUTTON) {
              child=HGroup,
                    Child, src[i].obj=ImageObject,
                        NoFrame,
                        MUIA_CycleChain       , 1,
                        MUIA_InputMode        , MUIV_InputMode_Toggle,
                        MUIA_Image_Spec       , MUII_RadioButton,
                        MUIA_ShowSelState     , FALSE,
                        MUIA_Selected         , FALSE,
                      End,
                    Child, TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_Contents, src[i].text,
                      End,

                  End;
            }
            else {
              child=HGroup,
                Child, src[i].obj=ImageObject,
                  ImageButtonFrame,
                  MUIA_InputMode   , MUIV_InputMode_Toggle,
                  MUIA_Image_Spec  , MUII_CheckMark,
                  MUIA_Background  , MUII_ButtonBack,
                  MUIA_ShowSelState, FALSE,
                  MUIA_CycleChain  , TRUE,
                End,
                Child, TextObject,
                  MUIA_Font, Topaz8Font,
                  MUIA_Text_Contents, src[i].text,
                  End,
              End;

            }
          }
          else if(!strcmp(src[i].windows_class, "RICHEDIT")) {
            DebOut("RICHEDIT found (text: %s\n", src[i].text);
            child=TextObject,
                    MUIA_Font, Topaz8Font,
                     MUIA_Text_PreParse, "\33c",
                    MUIA_Text_Contents, src[i].text,
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
          //src[i].obj=MUI_MakeObject(MUIO_Button, (ULONG) src[i].text);
          child=HGroup, MUIA_Background, MUII_ButtonBack,
                              ButtonFrame,
                              MUIA_InputMode , MUIV_InputMode_RelVerify,
                              Child, TextObject,
                                MUIA_Font, Topaz8Font,
                                MUIA_Text_PreParse, "\33c",
                                MUIA_Text_Contents, (ULONG) src[i].text,
                              End,
                            End;

          src[i].exists=TRUE;
          src[i].obj=child;
        break;

        case RTEXT:
          child=TextObject,
                        //MUIA_Background, MUII_MARKBACKGROUND,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33r",
                        MUIA_Text_Contents, (ULONG) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;

        case LTEXT:
          child=TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33l",
                        MUIA_Text_Contents, (ULONG) src[i].text,
                      End;
          src[i].exists=TRUE;
          src[i].obj=child;
        break;


        case EDITTEXT:
          if(src[i].flags & ES_READONLY) {
            child=TextObject,
                        MUIA_Font, Topaz8Font,
                        MUIA_Text_PreParse, "\33c",
                        MUIA_Text_Contents, (ULONG) src[i].text,
                      End;
            src[i].obj=child;
          }
          else {
            child=StringObject,
                          StringFrame,
                          MUIA_Font, Topaz8Font,
                          MUIA_String_Contents, (ULONG) src[i].text,
                        End;
            src[i].obj=child;
          }
          src[i].exists=TRUE;
        break;


        case COMBOBOX:
          child=CycleObject,
                       MUIA_Font, Topaz8Font,
                       MUIA_Cycle_Entries, Cycle_Dummy,
                     End;
          src[i].var=(char **) malloc(256 * sizeof(ULONG *)); // array for cycle texts
          src[i].var[0]=NULL;
          src[i].obj=child;
          src[i].exists=TRUE;
        break;


        default:
          DebOut("ERROR: unknown windows_type %d\n", src[i].windows_type);
      }
      DebOut("RESULT: child:   %lx\n", child);
      DebOut("RESULT: src.obj: %lx\n", src[i].obj);
      if(child) {
        if(src[i].help) {
          DebOut("add HOTHELP: %s\n", src[i].help);
          SetAttrs(child, MUIA_ShortHelp, (ULONG) src[i].help, TAG_DONE);
        }
        if(!(src[i].flags & WS_VISIBLE)) {
          SetAttrs(child, MUIA_ShowMe, (ULONG) 0, TAG_DONE);
        }
        //DebOut("=> add %lx to %lx\n", child, obj);
        //DoMethod((Object *) obj, OM_ADDMEMBER,(LONG) child);
        child=NULL;
      }
      DebOut("  src[%d].obj=%lx\n", i, src[i].obj);
      i++;

    }

    SETUPHOOK(&data->LayoutHook, LayoutHook, data);

    /*
     * ATTENTION: Add children *after* seting the LayoutHook,
     *            otherwise we get recursions!
     */
  
    SetAttrs(obj, MUIA_Group_LayoutHook, &data->LayoutHook, TAG_DONE);
    i=0;
    while(src[i].exists) {
      DebOut("i: %d (add %lx to %lx\n", i, src[i].obj, obj);
      DoMethod((Object *) obj, OM_ADDMEMBER,(LONG) src[i].obj);
      i++;
    }

    mSet(data, obj, (struct opSet *) msg, 1);
    SetAttrs(obj, MUIA_Frame, MUIV_Frame_Group, TAG_DONE);
  }
  return (ULONG)obj;
}

static ULONG mGet(struct Data *data, APTR obj, struct opGet *msg, struct IClass *cl)
{
  ULONG rc;

  DebOut("mGet..\n");

  switch (msg->opg_AttrID)
  {
    case MA_Data: 
      DebOut("data: %lx\n", data);
      rc = (ULONG) data; 
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

  while ((tag = NextTagItem((const TagItem**)&tstate))) {

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

DebOut("(%lx)->msg->MethodID: %lx\n",obj,msg->MethodID);


  switch (msg->MethodID)
  {
    case OM_NEW         : return mNew           (cl, obj, msg);
  	case OM_SET         :        mSet           (data, obj, (opSet *)msg, FALSE); break;
    case OM_GET         : return mGet           (data, obj, (opGet *)msg, cl);
    case OM_REMMEMBER   :        mRemMember     (data,      (opMember *)msg); break;
  }

ENDMTABLE

int init_class(void) {

  CL_Fixed = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct Data), (APTR)&mDispatcher);
  return CL_Fixed ? 1 : 0;
}

void delete_class(void) {

  if (CL_Fixed) {
    MUI_DeleteCustomClass(CL_Fixed);
    CL_Fixed=NULL;
  }
}



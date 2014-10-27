#ifndef __GUI_H__
#define __GUI_H__


enum {
  NJET,
  GROUPBOX,
  CONTROL,
  PUSHBUTTON,
  COMBOBOX,
  EDITTEXT,
  RTEXT,
  LTEXT,
};

typedef struct Element {
  BOOL exists;
  Object *obj;
  ULONG windows_type;
  ULONG x,y,w,h;
  const char *text;
  const char *help;
  ULONG flags;
} Element;

#define MY_TAGBASE 0xfece0000
enum {
  MA_src = MY_TAGBASE
};

#define GETDATA struct Data *data = (struct Data *)INST_DATA(cl, obj)
#define SETUPHOOK(x, func, data) { struct Hook *h = (x); h->h_Entry = (HOOKFUNC)&func; h->h_Data = (APTR)data; }
#define BEGINMTABLE AROS_UFH3(static ULONG,mDispatcher,AROS_UFHA(struct IClass*, cl, a0), AROS_UFHA(APTR, obj, a2), AROS_UFHA(Msg, msg, a1))
#define ENDMTABLE return DoSuperMethodA(cl, (Object *) obj, msg); }
#define HOOKPROTO(name, ret, obj, param) AROS_UFH2(ret, name, AROS_UFHA(APTR, param, A3), AROS_UFHA(Object *, obj, D0))

#define MakeHook(hookname, funcname) struct Hook hookname = {{NULL, NULL}, \
    (HOOKFUNC)funcname, NULL, NULL}


int init_class(void);
void delete_class(void);
ULONG xget(Object *obj, ULONG attr);

extern struct MUI_CustomClass *CL_Fixed;

#define WS_BORDER 0x00800000
#define WS_CAPTION 0x00C00000
#define WS_CHILD 0x40000000
#define WS_CHILDWINDOW 0x40000000
#define WS_CLIPCHILDREN 0x02000000
#define WS_CLIPSIBLINGS 0x04000000
#define WS_DISABLED 0x08000000
#define WS_DLGFRAME 0x00400000
#define WS_GROUP 0x00020000
#define WS_HSCROLL 0x00100000
#define WS_ICONIC 0x20000000
#define WS_MAXIMIZE 0x01000000
#define WS_MAXIMIZEBOX 0x00010000
#define WS_MINIMIZE 0x20000000
#define WS_MINIMIZEBOX 0x00020000
#define WS_OVERLAPPED 0x00000000
#define WS_OVERLAPPEDWINDOW WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
#define WS_POPUP unchecked((int)0x80000000)
#define WS_POPUPWINDOW WS_POPUP | WS_BORDER | WS_SYSMENU
#define WS_SIZEBOX 0x00040000
#define WS_SYSMENU 0x00080000
#define WS_TABSTOP 0x00010000
#define WS_THICKFRAME 0x00040000
#define WS_TILED 0x00000000
#define WS_TILEDWINDOW WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
#define WS_VISIBLE 0x10000000
#define WS_VSCROLL 0x00200000

/* Edit Control Styles*/
#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_NOHIDESEL        0x0100L
#define ES_OEMCONVERT       0x0400L
#define ES_READONLY         0x0800L
#define ES_WANTRETURN       0x1000L
#define ES_NUMBER           0x2000L

#endif

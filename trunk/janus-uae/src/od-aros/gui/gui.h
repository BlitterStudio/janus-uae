#ifndef __GUI_H__
#define __GUI_H__

#include <exec/types.h>
#include <libraries/mui.h>


enum {
  NJET,
  GROUPBOX,
  CONTROL,
  PUSHBUTTON,
  COMBOBOX,
  EDITTEXT,
  RTEXT,
  LTEXT,
  DEFPUSHBUTTON,
};

enum {
  IDI_APPICON, // "winuae.ico"
  IDI_FLOPPY, // "35floppy.ico"
  IDI_ABOUT, // "amigainfo.ico"
  IDI_HARDDISK, // "drive.ico"
  IDI_CPU, // "cpu.ico"
  IDI_GAMEPORTS, // "joystick.ico"
  IDI_IOPORTS, // "joystick.ico"
  IDI_INPUT, // "joystick.ico"
  IDI_MISC1, // "misc.ico"
  IDI_MISC2, // "misc.ico"
  IDI_MOVE_UP, // "move_up.ico"
  IDI_MOVE_DOWN, // "move_dow.ico"
  IDI_AVIOUTPUT, // "avioutput.ico"
  IDI_DISK, // "drive.ico"
  IDI_FOLDER, // "folder.ico"
  IDI_SOUND, // "sound.ico"
  IDI_DISPLAY, // "screen.ico"
  IDI_ROOT, // "root.ico"
  IDI_MEMORY, // "chip.ico"
  IDI_QUICKSTART, // "quickstart.ico"
  IDI_PATHS, // "paths.ico"
  IDI_DISKIMAGE, // "diskimage.ico"
  IDI_PORTS, // "port.ico"
  IDI_CONFIGFILE, // "configfile.ico"
  IDI_FILE, // "file.ico"
  IDI_EXPANSION, // "expansion.ico"
};

typedef struct Element {
  BOOL    exists;  // element exists
  ULONG   idc;     // windows rc identifyer (IDC_CPU0, IDC_Z3CHIPRAM, etc)
  Object *obj;     // Zune object pointer
  APTR    var;     // variable data pointer, dependent on windows_type
  ULONG   windows_type; // CONTROL / GROUPBOX etc rc types
  ULONG   x,y,w,h; // fixed position and height
  const char *text;// text of gadget
  const char *help;// mouse over text
  ULONG   flags;   // windows flags as bitfields (BS_AUTORADIOBUTTON)
  ULONG   flags2;  // more windows flags, which collide with flags above
} Element;

typedef Element *HWND;

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

/* Button Types */
#define BS_3STATE 5
#define BS_AUTO3STATE 6
#define BS_AUTOCHECKBOX 3
#define BS_AUTORADIOBUTTON  9
#define BS_BITMAP 128
#define BS_BOTTOM 0x800
#define BS_CENTER 0x300
#define BS_CHECKBOX 2
#define BS_DEFPUSHBUTTON  1
#define BS_GROUPBOX 7
#define BS_ICON 64
#define BS_LEFT 256
#define BS_LEFTTEXT 32
#define BS_MULTILINE  0x2000
#define BS_NOTIFY 0x4000
#define BS_OWNERDRAW  0xb
#define BS_PUSHBUTTON 0
#define BS_PUSHLIKE 4096
#define BS_RADIOBUTTON 4
#define BS_RIGHT  512
#define BS_RIGHTBUTTON  32
#define BS_TEXT 0
#define BS_TOP  0x400
#define BS_USERBUTTON 8
#define BS_VCENTER  0xc00
#define BS_FLAT 0x8000

#define WM_APP 32768
#define WM_ACTIVATE 6
#define WM_ACTIVATEAPP 28
/* FIXME/CHECK: Are WM_AFX{FIRST,LAST} valid for WINVER < 0x400? */
#define WM_AFXFIRST 864
#define WM_AFXLAST 895
#define WM_ASKCBFORMATNAME 780
#define WM_CANCELJOURNAL 75
#define WM_CANCELMODE 31
#define WM_CAPTURECHANGED 533
#define WM_CHANGECBCHAIN 781
#define WM_CHAR 258
#define WM_CHARTOITEM 47
#define WM_CHILDACTIVATE 34
#define WM_CLEAR 771
#define WM_CLOSE 16
#define WM_COMMAND 273
#define WM_COMMNOTIFY 68    /* obsolete */
#define WM_COMPACTING 65
#define WM_COMPAREITEM 57
#define WM_CONTEXTMENU 123
#define WM_COPY 769
#define WM_COPYDATA 74
#define WM_CREATE 1
#define WM_CTLCOLORBTN 309
#define WM_CTLCOLORDLG 310
#define WM_CTLCOLOREDIT 307
#define WM_CTLCOLORLISTBOX 308
#define WM_CTLCOLORMSGBOX 306
#define WM_CTLCOLORSCROLLBAR 311
#define WM_CTLCOLORSTATIC 312
#define WM_CUT 768
#define WM_DEADCHAR 259
#define WM_DELETEITEM 45
#define WM_DESTROY 2
#define WM_DESTROYCLIPBOARD 775
#define WM_DEVICECHANGE 537
#define WM_DEVMODECHANGE 27
#define WM_DISPLAYCHANGE 126
#define WM_DRAWCLIPBOARD 776
#define WM_DRAWITEM 43
#define WM_DROPFILES 563
#define WM_ENABLE 10
#define WM_ENDSESSION 22
#define WM_ENTERIDLE 289
#define WM_ENTERMENULOOP 529
#define WM_ENTERSIZEMOVE 561
#define WM_ERASEBKGND 20
#define WM_EXITMENULOOP 530
#define WM_EXITSIZEMOVE 562
#define WM_FONTCHANGE 29
#define WM_GETDLGCODE 135
#define WM_GETFONT 49
#define WM_GETHOTKEY 51
#define WM_GETICON 127
#define WM_GETMINMAXINFO 36
#define WM_GETTEXT 13
#define WM_GETTEXTLENGTH 14
/* FIXME/CHECK: Are WM_HANDHEL{FIRST,LAST} valid for WINVER < 0x400? */
#define WM_HANDHELDFIRST 856
#define WM_HANDHELDLAST 863
#define WM_HELP 83
#define WM_HOTKEY 786
#define WM_HSCROLL 276
#define WM_HSCROLLCLIPBOARD 782
#define WM_ICONERASEBKGND 39
#define WM_INITDIALOG 272
#define WM_INITMENU 278
#define WM_INITMENUPOPUP 279
#define WM_INPUTLANGCHANGE 81
#define WM_INPUTLANGCHANGEREQUEST 80
#define WM_KEYDOWN 256
#define WM_KEYUP 257
#define WM_KILLFOCUS 8
#define WM_MDIACTIVATE 546
#define WM_MDICASCADE 551
#define WM_MDICREATE 544
#define WM_MDIDESTROY 545
#define WM_MDIGETACTIVE 553
#define WM_MDIICONARRANGE 552
#define WM_MDIMAXIMIZE 549
#define WM_MDINEXT 548
#define WM_MDIREFRESHMENU 564
#define WM_MDIRESTORE 547
#define WM_MDISETMENU 560
#define WM_MDITILE 550
#define WM_MEASUREITEM 44
#if (WINVER >= 0x0500)
#define WM_UNINITMENUPOPUP 0x0125
#define WM_MENURBUTTONUP 290
#define WM_MENUCOMMAND 0x0126
#define WM_MENUGETOBJECT 0x0124
#define WM_MENUDRAG 0x0123
#endif
#define WM_MENUCHAR 288
#define WM_MENUSELECT 287
#define WM_NEXTMENU 531
#define WM_MOVE 3
#define WM_MOVING 534
#define WM_NCACTIVATE 134
#define WM_NCCALCSIZE 131
#define WM_NCCREATE 129
#define WM_NCDESTROY 130
#define WM_NCHITTEST 132
#define WM_NCLBUTTONDBLCLK 163
#define WM_NCLBUTTONDOWN 161
#define WM_NCLBUTTONUP 162
#define WM_NCMBUTTONDBLCLK 169
#define WM_NCMBUTTONDOWN 167
#define WM_NCMBUTTONUP 168
#if (_WIN32_WINNT >= 0x0500)
#define WM_NCXBUTTONDOWN 171
#define WM_NCXBUTTONUP 172
#define WM_NCXBUTTONDBLCLK 173
#define WM_NCMOUSEHOVER 0x02A0
#define WM_NCMOUSELEAVE 0x02A2
#endif
#define WM_NCMOUSEMOVE 160
#define WM_NCPAINT 133
#define WM_NCRBUTTONDBLCLK 166
#define WM_NCRBUTTONDOWN 164
#define WM_NCRBUTTONUP 165
#define WM_NEXTDLGCTL 40
#define WM_NEXTMENU 531
#define WM_NOTIFY 78
#define WM_NOTIFYFORMAT 85
#define WM_NULL 0
#define WM_PAINT 15
#define WM_PAINTCLIPBOARD 777
#define WM_PAINTICON 38
#define WM_PALETTECHANGED 785
#define WM_PALETTEISCHANGING 784
#define WM_PARENTNOTIFY 528
#define WM_PASTE 770
#define WM_PENWINFIRST 896
#define WM_PENWINLAST 911
#define WM_POWER 72
#define WM_POWERBROADCAST 536
#define WM_PRINT 791
#define WM_PRINTCLIENT 792
#define WM_QUERYDRAGICON 55
#define WM_QUERYENDSESSION 17
#define WM_QUERYNEWPALETTE 783
#define WM_QUERYOPEN 19
#define WM_QUEUESYNC 35
#define WM_QUIT 18
#define WM_RENDERALLFORMATS 774
#define WM_RENDERFORMAT 773
#define WM_SETCURSOR 32
#define WM_SETFOCUS 7
#define WM_SETFONT 48
#define WM_SETHOTKEY 50
#define WM_SETICON 128
#define WM_SETREDRAW 11
#define WM_SETTEXT 12
#define WM_SETTINGCHANGE 26
#define WM_SHOWWINDOW 24
#define WM_SIZE 5
#define WM_SIZECLIPBOARD 779
#define WM_SIZING 532
#define WM_SPOOLERSTATUS 42
#define WM_STYLECHANGED 125
#define WM_STYLECHANGING 124
#define WM_SYSCHAR 262
#define WM_SYSCOLORCHANGE 21
#define WM_SYSCOMMAND 274
#define WM_SYSDEADCHAR 263
#define WM_SYSKEYDOWN 260
#define WM_SYSKEYUP 261
#define WM_TCARD 82
#define WM_THEMECHANGED 794
#define WM_TIMECHANGE 30
#define WM_TIMER 275
#define WM_UNDO 772
#define WM_USER 1024
#define WM_USERCHANGED 84
#define WM_VKEYTOITEM 46
#define WM_VSCROLL 277
#define WM_VSCROLLCLIPBOARD 778
#define WM_WINDOWPOSCHANGED 71
#define WM_WINDOWPOSCHANGING 70
#define WM_WININICHANGE 26
#define WM_KEYFIRST 256
#define WM_KEYLAST 264
#define WM_SYNCPAINT  136
#define WM_MOUSEACTIVATE 33
#define WM_MOUSEMOVE 512
#define WM_LBUTTONDOWN 513
#define WM_LBUTTONUP 514
#define WM_LBUTTONDBLCLK 515
#define WM_RBUTTONDOWN 516
#define WM_RBUTTONUP 517
#define WM_RBUTTONDBLCLK 518
#define WM_MBUTTONDOWN 519
#define WM_MBUTTONUP 520
#define WM_MBUTTONDBLCLK 521
#define WM_MOUSEWHEEL 522
#define WM_MOUSEFIRST 512
#if (_WIN32_WINNT >= 0x0500)
#define WM_XBUTTONDOWN 523
#define WM_XBUTTONUP 524
#define WM_XBUTTONDBLCLK 525
#define WM_MOUSELAST 525
#else
#define WM_MOUSELAST 522
#endif
#define WM_MOUSEHOVER 0x2A1
#define WM_MOUSELEAVE 0x2A3

#define TBM_GETPOS (WM_USER)
#define TBM_GETRANGEMIN (WM_USER+1)
#define TBM_GETRANGEMAX (WM_USER+2)
#define TBM_GETTIC (WM_USER+3)
#define TBM_SETTIC (WM_USER+4)
#define TBM_SETPOS (WM_USER+5)
#define TBM_SETRANGE (WM_USER+6)
#define TBM_SETRANGEMIN (WM_USER+7)
#define TBM_SETRANGEMAX (WM_USER+8)
#define TBM_CLEARTICS (WM_USER+9)
#define TBM_SETSEL (WM_USER+10)
#define TBM_SETSELSTART (WM_USER+11)
#define TBM_SETSELEND (WM_USER+12)
#define TBM_GETPTICS (WM_USER+14)
#define TBM_GETTICPOS (WM_USER+15)
#define TBM_GETNUMTICS (WM_USER+16)
#define TBM_GETSELSTART (WM_USER+17)
#define TBM_GETSELEND (WM_USER+18)
#define TBM_CLEARSEL (WM_USER+19)
#define TBM_SETTICFREQ (WM_USER+20)
#define TBM_SETPAGESIZE (WM_USER+21)
#define TBM_GETPAGESIZE (WM_USER+22)
#define TBM_SETLINESIZE (WM_USER+23)
#define TBM_GETLINESIZE (WM_USER+24)
#define TBM_GETTHUMBRECT (WM_USER+25)
#define TBM_GETCHANNELRECT (WM_USER+26)
#define TBM_SETTHUMBLENGTH (WM_USER+27)
#define TBM_GETTHUMBLENGTH (WM_USER+28)
#define TBM_SETTOOLTIPS (WM_USER+29)
#define TBM_GETTOOLTIPS (WM_USER+30)
#define TBM_SETTIPSIDE (WM_USER+31)
#define TBM_SETBUDDY (WM_USER+32)
#define TBM_GETBUDDY (WM_USER+33) 

/* Combo box messages */
#define CB_GETEDITSEL            0x0140
#define CB_LIMITTEXT             0x0141
#define CB_SETEDITSEL            0x0142
#define CB_ADDSTRING             0x0143
#define CB_DELETESTRING          0x0144
#define CB_DIR                   0x0145
#define CB_GETCOUNT              0x0146
#define CB_GETCURSEL             0x0147
#define CB_GETLBTEXT             0x0148
#define CB_GETLBTEXTLEN          0x0149
#define CB_INSERTSTRING          0x014a
#define CB_RESETCONTENT          0x014b
#define CB_FINDSTRING            0x014c
#define CB_SELECTSTRING          0x014d
#define CB_SETCURSEL             0x014e
#define CB_SHOWDROPDOWN          0x014f
#define CB_GETITEMDATA           0x0150
#define CB_SETITEMDATA           0x0151
#define CB_GETDROPPEDCONTROLRECT 0x0152
#define CB_SETITEMHEIGHT         0x0153
#define CB_GETITEMHEIGHT         0x0154
#define CB_SETEXTENDEDUI         0x0155
#define CB_GETEXTENDEDUI         0x0156
#define CB_GETDROPPEDSTATE       0x0157
#define CB_FINDSTRINGEXACT       0x0158
#define CB_SETLOCALE             0x0159
#define CB_GETLOCALE             0x015a
#define CB_GETTOPINDEX           0x015b
#define CB_SETTOPINDEX           0x015c
#define CB_GETHORIZONTALEXTENT   0x015d
#define CB_SETHORIZONTALEXTENT   0x015e
#define CB_GETDROPPEDWIDTH       0x015f
#define CB_SETDROPPEDWIDTH       0x0160
#define CB_INITSTORAGE           0x0161
#define CB_MULTIPLEADDSTRING     0x0163
#define CB_GETCOMBOBOXINFO       0x0164
#define CB_MSGMAX                0x0165

#define MAKELONG(x,y) x*0x10000+y

#endif

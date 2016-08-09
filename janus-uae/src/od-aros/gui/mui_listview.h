#ifndef __MUI_LISTVIEW_H
#define __MUI_LISTVIEW_H


Object *new_listview(struct Element *src, ULONG i, void *f, struct Data *data, Object **nlistview);

typedef struct _LV_ITEM {
  UINT mask;
  int iItem;
  int iSubItem;
  UINT state;
  UINT stateMask;
  LPTSTR pszText;
  int cchTextMax;
  int iImage; // index of the list view item's icon
  LPARAM lParam; // 32-bit value to associate with item
} LV_ITEM, *LPLVITEM; 

typedef struct _LV_COLUMN {
  UINT mask;
  int fmt;
  int cx;
  LPTSTR pszText;
  int cchTextMax;
  int iSubItem;
} LV_COLUMN, *LPLVCOLUMN;


int  ListView_GetItemCount(HWND hwnd);
void ListView_SetItemState(HWND hwnd, int i, UINT state, UINT mask);
LONG Button_SetElevationRequiredState(HWND hDlg, int nIDDlgItem, BOOL fRequired);
int ListView_GetTopIndex(HWND hwnd);
int ListView_GetCountPerPage(HWND hwnd);
void ListView_SetExtendedListViewStyleEx(HWND hwndLV, DWORD dwExMask, DWORD dwExStyle);
void ListView_RemoveAllGroups(HWND hwnd);
BOOL ListView_DeleteAllItems(HWND hwnd);
int ListView_InsertColumn(HWND hwnd, int iCol, const LPLVCOLUMN pcol);
BOOL ListView_GetColumn(HWND hwnd, int iCol, LPLVCOLUMN pcol);
int ListView_InsertItem(HWND nIDDlgItem, const LPLVITEM pitem);
VOID ListView_SetItemText(HWND hwnd, int i, int iSubItem, const char *pszText);
int ListView_GetStringWidth(HWND nIDDlgItem, const char *psz);
UINT ListView_GetItemState(HWND nIDDlgItem, int  nr, UINT mask);

/* Windows Listview defines (commctrl.h) */

#define LVN_FIRST ((UINT)-100)
#define LVN_LAST ((UINT)-199) 

#define LVS_ICON 0
#define LVS_REPORT 1
#define LVS_SMALLICON 2
#define LVS_LIST 3
#define LVS_TYPEMASK 3
#define LVS_SINGLESEL 4
#define LVS_SHOWSELALWAYS 8
#define LVS_SORTASCENDING 16
#define LVS_SORTDESCENDING 32
#define LVS_SHAREIMAGELISTS 64
#define LVS_NOLABELWRAP 128
#define LVS_AUTOARRANGE 256
#define LVS_EDITLABELS 512
#define LVS_NOSCROLL 0x2000
#define LVS_TYPESTYLEMASK 0xfc00
#define LVS_ALIGNTOP 0
#define LVS_ALIGNLEFT 0x800
#define LVS_ALIGNMASK 0xc00
#define LVS_OWNERDRAWFIXED 0x400
#define LVS_NOCOLUMNHEADER 0x4000
#define LVS_NOSORTHEADER 0x8000
#define LVSIL_NORMAL 0
#define LVSIL_SMALL 1
#define LVSIL_STATE 2
#define LVM_GETBKCOLOR LVM_FIRST
#define LVM_SETBKCOLOR (LVM_FIRST+1)
#define LVM_GETIMAGELIST (LVM_FIRST+2)
#define LVM_SETIMAGELIST (LVM_FIRST+3)
#define LVM_GETITEMCOUNT (LVM_FIRST+4)
#define LVM_SORTITEMSEX (LVM_FIRST+81)
#define LVM_SETVIEW (LVM_FIRST+142)
#define LVM_GETVIEW (LVM_FIRST+143)
#define LVM_INSERTGROUP (LVM_FIRST+145)
#define LVM_SETGROUPINFO (LVM_FIRST+147)
#define LVM_GETGROUPINFO (LVM_FIRST+149)
#define LVM_REMOVEGROUP (LVM_FIRST+150)
#define LVM_MOVEGROUP (LVM_FIRST+151)
#define LVM_SETGROUPMETRICS (LVM_FIRST+155)
#define LVM_GETGROUPMETRICS (LVM_FIRST+156)
#define LVM_ENABLEGROUPVIEW (LVM_FIRST+157)
#define LVM_SORTGROUPS (LVM_FIRST+158)
#define LVM_INSERTGROUPSORTED (LVM_FIRST+159)
#define LVM_REMOVEALLGROUPS (LVM_FIRST+160)
#define LVM_HASGROUP (LVM_FIRST+161)
#define LVM_SETTILEVIEWINFO (LVM_FIRST+162)
#define LVM_GETTILEVIEWINFO (LVM_FIRST+163)
#define LVM_SETTILEINFO (LVM_FIRST+164)
#define LVM_GETTILEINFO (LVM_FIRST+165)
#define LVM_SETINSERTMARK (LVM_FIRST+166)
#define LVM_GETINSERTMARK (LVM_FIRST+167)
#define LVM_INSERTMARKHITTEST (LVM_FIRST+168)
#define LVM_GETINSERTMARKRECT (LVM_FIRST+169)
#define LVM_SETINSERTMARKCOLOR (LVM_FIRST+170)
#define LVM_GETINSERTMARKCOLOR (LVM_FIRST+171)
#define LVM_SETINFOTIP (LVM_FIRST+173)
#define LVM_GETSELECTEDCOLUMN (LVM_FIRST+174)
#define LVM_ISGROUPVIEWENABLED (LVM_FIRST+175)
#define LVM_GETOUTLINECOLOR (LVM_FIRST+176)
#define LVM_SETOUTLINECOLOR (LVM_FIRST+177)
#define LVM_CANCELEDITLABEL (LVM_FIRST+179)
#define LVM_MAPIDTOINDEX (LVM_FIRST+181)
#define LVIF_TEXT 1
#define LVIF_IMAGE 2
#define LVIF_PARAM 4
#define LVIF_STATE 8
#define LVIS_FOCUSED 1
#define LVIS_SELECTED 2
#define LVIS_CUT 4
#define LVIS_DROPHILITED 8
#define LVIS_OVERLAYMASK 0xF00
#define LVIS_STATEIMAGEMASK 0xF000
#define LPSTR_TEXTCALLBACKW ((LPWSTR)-1)
#define LPSTR_TEXTCALLBACKA ((LPSTR)-1)
#define I_IMAGECALLBACK (-1)
#define LVM_GETITEMA (LVM_FIRST+5)
#define LVM_GETITEMW (LVM_FIRST+75)
#define LVM_SETITEMA (LVM_FIRST+6)
#define LVM_SETITEMW (LVM_FIRST+76)
#define LVM_INSERTITEMA (LVM_FIRST+7)
#define LVM_INSERTITEMW (LVM_FIRST+77)
#define LVM_DELETEITEM (LVM_FIRST+8)
#define LVM_DELETEALLITEMS (LVM_FIRST+9)
#define LVM_GETCALLBACKMASK (LVM_FIRST+10)
#define LVM_SETCALLBACKMASK (LVM_FIRST+11)
#define LVNI_ALL 0
#define LVNI_FOCUSED 1
#define LVNI_SELECTED 2
#define LVNI_CUT 4
#define LVNI_DROPHILITED 8
#define LVNI_ABOVE 256
#define LVNI_BELOW 512
#define LVNI_TOLEFT 1024
#define LVNI_TORIGHT 2048
#define LVM_GETNEXTITEM (LVM_FIRST+12)
#define LVFI_PARAM 1
#define LVFI_STRING 2
#define LVFI_PARTIAL 8
#define LVFI_WRAP 32
#define LVFI_NEARESTXY 64
#define LVIF_DI_SETITEM 0x1000
#define LVM_FINDITEMA (LVM_FIRST+13)
#define LVM_FINDITEMW (LVM_FIRST+83)
#define LVIR_BOUNDS 0
#define LVIR_ICON 1
#define LVIR_LABEL 2
#define LVIR_SELECTBOUNDS 3
#define LVM_GETITEMRECT (LVM_FIRST+14)
#define LVM_SETITEMPOSITION (LVM_FIRST+15)
#define LVM_GETITEMPOSITION (LVM_FIRST+16)
#define LVM_GETSTRINGWIDTHA (LVM_FIRST+17)
#define LVM_GETSTRINGWIDTHW (LVM_FIRST+87)
#define LVHT_NOWHERE 1
#define LVHT_ONITEMICON 2
#define LVHT_ONITEMLABEL 4
#define LVHT_ONITEMSTATEICON 8
#define LVHT_ONITEM (LVHT_ONITEMICON|LVHT_ONITEMLABEL|LVHT_ONITEMSTATEICON)
#define LVHT_ABOVE 8
#define LVHT_BELOW 16
#define LVHT_TORIGHT 32
#define LVHT_TOLEFT 64
#define LVM_HITTEST (LVM_FIRST+18)
#define LVM_ENSUREVISIBLE (LVM_FIRST+19)
#define LVM_SCROLL (LVM_FIRST+20)
#define LVM_REDRAWITEMS (LVM_FIRST+21)
#define LVA_DEFAULT 0
#define LVA_ALIGNLEFT 1
#define LVA_ALIGNTOP 2
#define LVA_SNAPTOGRID 5
#define LVM_ARRANGE (LVM_FIRST+22)
#define LVM_EDITLABELA (LVM_FIRST+23)
#define LVM_EDITLABELW (LVM_FIRST+118)
#define LVM_GETEDITCONTROL (LVM_FIRST+24)
#define LVCF_FMT 1
#define LVCF_WIDTH 2
#define LVCF_TEXT 4
#define LVCF_SUBITEM 8
#define LVCFMT_LEFT 0
#define LVCFMT_RIGHT 1
#define LVCFMT_CENTER 2
#define LVCFMT_JUSTIFYMASK 3
#define LVM_GETCOLUMNA (LVM_FIRST+25)
#define LVM_GETCOLUMNW (LVM_FIRST+95)
#define LVM_SETCOLUMNA (LVM_FIRST+26)
#define LVM_SETCOLUMNW (LVM_FIRST+96)
#define LVM_INSERTCOLUMNA (LVM_FIRST+27)
#define LVM_INSERTCOLUMNW (LVM_FIRST+97)
#define LVM_DELETECOLUMN (LVM_FIRST+28)
#define LVM_GETCOLUMNWIDTH (LVM_FIRST+29)
#define LVSCW_AUTOSIZE (-1)
#define LVSCW_AUTOSIZE_USEHEADER (-2)
#define LVM_SETCOLUMNWIDTH (LVM_FIRST+30)
#define LVM_CREATEDRAGIMAGE (LVM_FIRST+33)
#define LVM_GETVIEWRECT (LVM_FIRST+34)
#define LVM_GETTEXTCOLOR (LVM_FIRST+35)
#define LVM_SETTEXTCOLOR (LVM_FIRST+36)
#define LVM_GETTEXTBKCOLOR (LVM_FIRST+37)
#define LVM_SETTEXTBKCOLOR (LVM_FIRST+38)
#define LVM_GETTOPINDEX (LVM_FIRST+39)
#define LVM_GETCOUNTPERPAGE (LVM_FIRST+40)
#define LVM_GETORIGIN (LVM_FIRST+41)
#define LVM_GETORIGIN (LVM_FIRST+41)
#define LVM_UPDATE (LVM_FIRST+42)
#define LVM_SETITEMSTATE (LVM_FIRST+43)
#define LVM_GETITEMSTATE (LVM_FIRST+44)
#define LVM_GETITEMTEXTA (LVM_FIRST+45)
#define LVM_GETITEMTEXTW (LVM_FIRST+115)
#define LVM_SETITEMTEXTA (LVM_FIRST+46)
#define LVM_SETITEMTEXTW (LVM_FIRST+116)
#define LVM_SETITEMCOUNT (LVM_FIRST+47)
#define LVM_SORTITEMS (LVM_FIRST+48)
#define LVM_SETITEMPOSITION32 (LVM_FIRST+49)
#define LVM_GETSELECTEDCOUNT (LVM_FIRST+50)
#define LVM_GETITEMSPACING (LVM_FIRST+51)
#define LVM_GETISEARCHSTRINGA (LVM_FIRST+52)
#define LVM_GETISEARCHSTRINGW (LVM_FIRST+117)
#define LVN_ITEMCHANGING LVN_FIRST
#define LVN_ITEMCHANGED (LVN_FIRST-1)
#define LVN_INSERTITEM (LVN_FIRST-2)
#define LVN_DELETEITEM (LVN_FIRST-3)
#define LVN_DELETEALLITEMS (LVN_FIRST-4)
#define LVN_BEGINLABELEDITA (LVN_FIRST-5)
#define LVN_BEGINLABELEDITW (LVN_FIRST-75)
#define LVN_ENDLABELEDITA (LVN_FIRST-6)
#define LVN_ENDLABELEDITW (LVN_FIRST-76)
#define LVN_COLUMNCLICK (LVN_FIRST-8)
#define LVN_BEGINDRAG (LVN_FIRST-9)
#define LVN_BEGINRDRAG (LVN_FIRST-11)
#define LVN_GETDISPINFOA (LVN_FIRST-50)
#define LVN_GETDISPINFOW (LVN_FIRST-77)
#define LVN_SETDISPINFOA (LVN_FIRST-51)
#define LVN_SETDISPINFOW (LVN_FIRST-78)
#define LVN_KEYDOWN (LVN_FIRST-55) 

#endif

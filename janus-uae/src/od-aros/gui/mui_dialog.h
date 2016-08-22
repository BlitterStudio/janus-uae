#ifndef _MUI_DIALOG_H_
#define _MUI_DIALOG_H_

#include "muigui.h" /* HINSTANCE declaration */

#define INT_PTR int
#define DLGPROC IPTR

typedef struct {
  DWORD style;
  DWORD dwExtendedStyle;
  WORD  cdit;
  short x;
  short y;
  short cx;
  short cy;
} DLGTEMPLATE, *LPDLGTEMPLATE, *LPCDLGTEMPLATEW, *LPDLGTEMPLATE;

typedef DLGTEMPLATE *LPCDLGTEMPLATE;

struct newresource
{
    LPCDLGTEMPLATEW resource;
    HINSTANCE inst;
    int size;
    int tmpl;
    int width, height;
};

int GetDlgCtrlID(HWND hwndCtl);
BOOL EndDialog(HWND hDlg, int nResult);
UINT GetDlgItemInt(HWND hDlg, int  nIDDlgItem, BOOL *lpTranslated,  BOOL bSigned);
INT_PTR DialogBoxIndirect(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, INT_PTR (*func) (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam));

#endif

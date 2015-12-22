#ifndef WIN32GUI_H
#define WIN32GUI_H

#include "registry.h"

#define CFG_DESCRIPTION_LENGTH 128

#define CONFIG_SAVE   0
#define CONFIG_LOAD   1
#define CONFIG_SAVE_FULL 2
#define CONFIG_LOAD_FULL 3
#define CONFIG_DELETE 4

void WIN32GUI_LoadUIString (DWORD id, TCHAR *string, DWORD dwStringLen);
extern int DiskSelection (HWND hDlg, WPARAM wParam, int flag, struct uae_prefs *prefs, TCHAR *);
void InitializeListView (HWND hDlg);
extern void pre_gui_message (const TCHAR*,...);
extern void gui_message_id (int id);
#if 0
int dragdrop (HWND hDlg, HDROP hd, struct uae_prefs *prefs, int currentpage);
#endif
UAEREG *read_disk_history (int type);
void write_disk_history (void);

typedef APTR HINSTANCE;

#if 0
struct newresource
{
    LPCDLGTEMPLATEW resource;
    HINSTANCE inst;
    int size;
    int tmpl;
    int width, height;
};
#endif

#define GUI_INTERNAL_WIDTH 800
#define GUI_INTERNAL_HEIGHT 600
#define GUI_INTERNAL_FONT 8

struct newresource *scaleresource (struct newresource *res, HWND parent, int resize, int fullscreen, DWORD exstyle);
extern void freescaleresource (struct newresource*);
extern void scaleresource_setmult (HWND hDlg, int w, int h);
extern void scaleresource_getmult (int *mx, int *my);
#if 0
extern HWND CustomCreateDialog (int templ, HWND hDlg, DLGPROC proc);
extern INT_PTR CustomDialogBox (int templ, HWND hDlg, DLGPROC proc);
#endif
extern struct newresource *getresource (int tmpl);
extern struct newresource *resourcefont (struct newresource*, TCHAR *font, int size);
extern void scaleresource_init (const TCHAR*);
extern int scaleresource_choosefont (HWND hDlg, int fonttype);
extern void scaleresource_setdefaults (void);
extern void scaleresource_setfont (HWND hDlg);
extern double scaleresource_getdpimult (void);

#define GetDlgItem(a,b) a,b
BOOL mui_get_filename(TCHAR *lpstrTitle, TCHAR *lpstrInitialDir, TCHAR *lpstrFile, TCHAR *lpstrFilter, TCHAR *lpstrFileTitle, ULONG flags, TCHAR *defext);
#endif

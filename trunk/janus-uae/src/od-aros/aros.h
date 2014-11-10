#ifndef __AROS_H__
#define __AROS_H__
#include "sys/mman.h"
#include "gui/gui.h"

#define GETBDY(x) ((x) / 1000000 + 2000)
#define GETBDM(x) (((x) - ((x / 10000) * 10000)) / 100)
#define GETBDD(x) ((x) % 100)


#define WINUAEPUBLICBETA 0

#define WINUAEBETA ""
#define WINUAEDATE MAKEBD(2014, 6, 18)
#define WINUAEEXTRA ""
#define WINUAEREV ""
#define MAKEBD(x,y,z) ((((x) - 2000) * 10000 + (y)) * 100 + (z))

#define INVALID_FILE_ATTRIBUTES 0

#define MAX_DISPLAYS 10
extern struct MultiDisplay Displays[MAX_DISPLAYS + 1];
struct MultiDisplay *getdisplay (struct uae_prefs *p);

int WIN32GFX_IsPicassoScreen (void);


extern int screen_is_picasso, scalepicasso;
extern int default_freq;
extern TCHAR start_path_data[MAX_DPATH];
extern TCHAR VersionStr[256];
extern TCHAR BetaStr[64];
extern void keyboard_settrans (void);
extern bool winuaelog_temporary_enable;
extern int af_path_2005;
extern TCHAR start_path_new1[MAX_DPATH], start_path_new2[MAX_DPATH];
extern TCHAR bootlogpath[MAX_DPATH];

enum pathtype { PATH_TYPE_DEFAULT, PATH_TYPE_WINUAE, PATH_TYPE_NEWWINUAE, PATH_TYPE_NEWAF, PATH_TYPE_AMIGAFOREVERDATA, PATH_TYPE_END };
extern pathtype path_type;


/* http://msdn.microsoft.com/en-us/library/htb3tdkc%28v=vs.80%29.aspx: 
 * _daylight, _dstbias, _timezone, and _tzname are used in some time and date routines 
 * to make local-time adjustments. These global variables have been deprecated in 
 * Visual C++ 2005 for the more secure functional versions, which should be used 
 * in place of the global variables.
 */
extern int _daylight; 
extern int _dstbias; 
extern long _timezone; 
extern char *_tzname[2];

/* ? */
void _tzset(void);


void sleep_millis (int ms);
void sleep_millis_main (int ms);
void sleep_millis_busy (int ms);
void gui_restart (void);
void fetch_path (const TCHAR *name, TCHAR *out, int size);

/* taken from FS-UAE */
extern uaecptr p96ram_start;

int GetFileAttributes(char *path);

/* gui */
extern void *hInst;
extern void *hUIDLL;
int LoadString(APTR hInstance, TCHAR *uID, TCHAR * lpBuffer, int nBufferMax);
BOOL SetDlgItemText(struct Element *elem, int nIDDlgItem, TCHAR *lpString);
LONG SendDlgItemMessage(struct Element *hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CheckDlgButton(Element *elem, int button, UINT uCheck);
BOOL SetWindowText(HWND hWnd, TCHAR *lpString);
BOOL CheckRadioButton(HWND elem, int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
int MessageBox(HWND hWnd, TCHAR *lpText, TCHAR *lpCaption, UINT uType);

void read_rom_list (void);
extern int quickstart, configurationcache, relativepaths;

/* gfx */
void updatewinfsmode (struct uae_prefs *p);

#define CALLBACK
#define FAR
#define INT_PTR int 

typedef struct {
  BYTE fVirt;
  WORD key;
  WORD cmd;
} ACCEL;


#endif /* __AROS_H__ */

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

int  aros_show_gui(void);
void aros_hide_gui(void);
void aros_gui_exit(void);

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
BOOL SetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned);
BOOL SetWindowText(HWND hWnd, TCHAR *lpString);
BOOL CheckRadioButton(HWND elem, int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
int MessageBox(HWND hWnd, TCHAR *lpText, TCHAR *lpCaption, UINT uType);
UINT IsDlgButtonChecked(HWND elem, int item);

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

typedef ULONG LRESULT;

#define MAX_SOUND_DEVICES 100
#define SOUND_DEVICE_DS 1
#define SOUND_DEVICE_AL 2
#define SOUND_DEVICE_PA 3
#define SOUND_DEVICE_WASAPI 4
#define SOUND_DEVICE_WASAPI_EXCLUSIVE 5
#define SOUND_DEVICE_XAUDIO2 6

struct sound_device
{
    //GUID guid;
    TCHAR *name;
    TCHAR *alname;
    TCHAR *cfgname;
    int panum;
    int type;
};
extern struct sound_device *sound_devices[MAX_SOUND_DEVICES];
extern struct sound_device *record_devices[MAX_SOUND_DEVICES];

/* better autogenerate those from resource? */
#define IDC_FREQUENCY                   1237
#define IDC_SOUNDADJUST                 1575
#define IDC_SOUNDADJUSTNUM              1578
#define IDC_SOUNDBUFFERTEXT             1576
#define IDC_AUDIOSYNC                   1590
#define IDC_AUDIOSYNC                   1590
#define IDC_SOUNDCALIBRATE              1641


#endif /* __AROS_H__ */

#ifndef __AROS_H__
#define __AROS_H__

#include <stdint.h>

#include "sys/mman.h"
#include "gui/gui_mui.h"
#include "zfile.h"

#if !defined(XGET)
#define XGET(object, attribute)                 \
({                                              \
    IPTR __storage = 0;                         \
    GetAttr((attribute), (object), &__storage); \
    __storage;                                  \
})
#endif /* !XGET */

#define GETBDY(x) ((x) / 1000000 + 2000)
#define GETBDM(x) (((x) - ((x / 10000) * 10000)) / 100)
#define GETBDD(x) ((x) % 100)

#define WINUAEPUBLICBETA 1

#define __DAY__ ((__DATE__[4] - '0') * 10 + __DATE__[5] - '0')
#define __MONTH__ (\
    __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
  : __DATE__ [2] == 'b' ? 2 \
  : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
  : __DATE__ [2] == 'y' ? 5 \
  : __DATE__ [2] == 'l' ? 7 \
  : __DATE__ [2] == 'g' ? 8 \
  : __DATE__ [2] == 'p' ? 9 \
  : __DATE__ [2] == 't' ? 10 \
  : __DATE__ [2] == 'v' ? 11 \
  : 12)
#define __YEAR__ ((__DATE__[7] - '0') * 1000 +  (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + __DATE__[10] - '0')

#define WINUAEBETA "02"
#define WINUAEDATE MAKEBD(__YEAR__, __MONTH__, __DAY__)
#define WINUAEEXTRA ""
#define WINUAEREV ""
#define MAKEBD(x,y,z) ((((x) - 2000) * 10000 + (y)) * 100 + (z))

#define MAX_DISPLAYS 10

typedef LONG LRESULT;
typedef const void* LPCVOID;
typedef DWORD *LPDWORD;
typedef TCHAR *LPCTSTR;


extern int os_admin, os_64bit, os_win7, os_vista, cpu_number, os_touch;
extern int max_uae_width, max_uae_height;

extern struct MultiDisplay Displays[MAX_DISPLAYS + 1];
struct MultiDisplay *getdisplay (struct uae_prefs *p);

int WIN32GFX_IsPicassoScreen (void);

extern TCHAR *inipath;

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
extern TCHAR logpath[MAX_DPATH];

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
//extern long _timezone; 
extern char *_tzname[2];

/* ? */
void _tzset(void);

extern int mouseactive;
extern int minimized;

extern TCHAR start_path_exe[MAX_DPATH];
extern int start_data;

void sleep_millis (int ms);
void sleep_millis_main (int ms);
void sleep_millis_busy (int ms);
void gui_restart (void);

int  aros_show_gui(void);
void aros_hide_gui(void);
void aros_gui_exit(void);
void exit_gui (int);

void fetch_path (const TCHAR *name, TCHAR *out, int size);

/* taken from FS-UAE */
extern uaecptr p96ram_start;

int GetFileAttributes(char *path);

/* file stuff */
int my_existsdir (const TCHAR *name);
int my_mkdir (const TCHAR *name);
int my_rmdir (const TCHAR *name);

/* gui */
extern void *hInst;
extern void *hUIDLL;
int LoadString(APTR hInstance, TCHAR *uID, TCHAR * lpBuffer, int nBufferMax);
BOOL SetDlgItemText(struct Element *elem, int nIDDlgItem, const TCHAR *lpString);
LONG SendDlgItemMessage(struct Element *hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CheckDlgButton(Element *elem, int button, UINT uCheck);
BOOL SetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned);
BOOL SetWindowText(HWND hWnd, TCHAR *lpString);
BOOL SetWindowText(HWND hWnd, DWORD id, TCHAR *lpString);
int GetWindowText(HWND   hWnd, LPTSTR lpString, int nMaxCount);
int GetWindowText(HWND   hWnd, DWORD id, LPTSTR lpString, int nMaxCount);
BOOL CheckRadioButton(HWND elem, int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
int MessageBox(HWND hWnd, TCHAR *lpText, TCHAR *lpCaption, UINT uType);
int MessageBox_fixed(HWND hWnd, TCHAR *lpText, TCHAR *lpCaption, UINT uType);
UINT IsDlgButtonChecked(HWND elem, int item);
BOOL EnableWindow(HWND hWnd, DWORD id, BOOL bEnable);
UINT GetDlgItemText(HWND elem, int nIDDlgItem, TCHAR *lpString, int nMaxCount);
LRESULT SendMessage(int hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT SendMessage(struct Element *hWnd, DWORD id, UINT Msg, WPARAM wParam, LPARAM lParam);

void *ShellExecute(HWND hwnd, TCHAR *lpOperation, TCHAR *lpFile, TCHAR *lpParameters, TCHAR *lpDirectory, int nShowCmd);
DWORD GetTempPath(DWORD nBufferLength, TCHAR *lpBuffer);
DWORD GetFullPathName(const TCHAR *lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart);

void read_rom_list (void);
void set_path (const TCHAR *name, TCHAR *path, pathtype mode);
void set_path (const TCHAR *name, TCHAR *path);
void setpathmode (pathtype pt);
void gui_message_id (TCHAR *id);


/* this function is defined in uaeunp.cpp, uses one parameter less that
 * the amiga_to_timeval from filesys.cpp ..
 */
void amiga_to_timeval (struct mytimeval *tv, int days, int mins, int ticks);
extern bool resumepaused (int priority);
extern bool setpaused (int priority);

extern int quickstart, configurationcache, saveimageoriginalpath, relativepaths, recursiveromscan;
extern Object *app, *win;
int aros_init_gui(void);

/* gfx */
void updatewinfsmode (struct uae_prefs *p);
void sortdisplays (void);

void create_afnewdir (int remove);

extern int isfocus (void);

#define CALLBACK
#define FAR
#define INT_PTR int 
#define ULONG_PTR int

typedef struct {
  BYTE fVirt;
  WORD key;
  WORD cmd;
} ACCEL;

#define FILETIME mytimeval

typedef struct _WIN32_FIND_DATA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  TCHAR    cFileName[MAX_PATH];
  TCHAR    cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;


#define HANDLE void *
HANDLE FindFirstFile(const TCHAR *lpFileName, WIN32_FIND_DATA *lpFindFileData);
BOOL FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
BOOL FindClose(HANDLE hFindFile);

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
#if 0
#define IDC_FREQUENCY                   1237
#define IDC_SOUNDADJUST                 1575
#define IDC_SOUNDADJUSTNUM              1578
#define IDC_SOUNDBUFFERTEXT             1576
#define IDC_AUDIOSYNC                   1590
#define IDC_AUDIOSYNC                   1590
#define IDC_SOUNDCALIBRATE              1641
#endif


struct contextcommand
{
	TCHAR *shellcommand;
	TCHAR *command;
	int icon;
};

struct assext {
    TCHAR *ext;
    TCHAR *cmd;
    TCHAR *desc;
    int icon;
    struct contextcommand *cc;
    int enabled;
};

extern struct assext exts[];

void associate_file_extensions (void);

struct winuae_currentmode {
	unsigned int flags;
	int native_width, native_height, native_depth, pitch;
	int current_width, current_height, current_depth;
	int amiga_width, amiga_height;
	int initdone;
	int fullfill;
	int vsync;
	int freq;
};

extern struct winuae_currentmode *currentmode;

#endif /* __AROS_H__ */


#define GETBDY(x) ((x) / 1000000 + 2000)
#define GETBDM(x) (((x) - ((x / 10000) * 10000)) / 100)
#define GETBDD(x) ((x) % 100)


#define WINUAEPUBLICBETA 0

#define WINUAEBETA ""
#define WINUAEDATE MAKEBD(2011, 2, 26)
#define WINUAEEXTRA ""
#define WINUAEREV ""
#define MAKEBD(x,y,z) ((((x) - 2000) * 10000 + (y)) * 100 + (z))

#define MAX_DISPLAYS 10
extern struct MultiDisplay Displays[MAX_DISPLAYS + 1];
struct MultiDisplay *getdisplay (struct uae_prefs *p);

int WIN32GFX_IsPicassoScreen (void);


extern int screen_is_picasso, scalepicasso;
extern int default_freq;
extern TCHAR start_path_data[MAX_DPATH];
extern TCHAR VersionStr[256];
extern void keyboard_settrans (void);

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

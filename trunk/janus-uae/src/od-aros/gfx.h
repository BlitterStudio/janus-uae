#ifndef __GFX_H__
#define __GFX_H__

#include <intuition/intuition.h>

#include "sysdeps.h"
#include "aros.h"
#include "sys/mman.h"

#define REFRESH_RATE_RAW 1
#define REFRESH_RATE_LACE 2

#define RTG_MODE_SCALE 1
#define RTG_MODE_CENTER 2
#define RTG_MODE_INTEGER_SCALE 3

void DX_Invalidate (int x, int y, int width, int height);
int DX_Fill (int dstx, int dsty, int width, int height, int color, unsigned int rgbtype);
double getcurrentvblankrate (void);

int isvsync_chipset (void);
int isvsync_rtg (void);
int isvsync (void);

extern struct Window *hAmigaWnd;
extern HWND hHiddenWnd;

extern void enumeratedisplays (int);


#if 0
#define MAX_REFRESH_RATES 100
struct PicassoResolution
{
	struct ScreenResolution res;
	int depth;   /* depth in bytes-per-pixel */
	int residx;
	int refresh[MAX_REFRESH_RATES]; /* refresh-rates in Hz */
	int refreshtype[MAX_REFRESH_RATES]; /* 0=dx,1=enumdisplaysettings */
	TCHAR name[25];
	/* Bit mask of RGBFF_xxx values.  */
	uae_u32 colormodes;
        int rawmode;
        bool lace;
};
extern struct PicassoResolution DisplayModes[MAX_PICASSO_MODES];
#endif

/* in od-win/dxwrap.h */
#define MAX_REFRESH_RATES 100
#define MAX_PICASSO_MODES 300

struct MultiDisplay {
  bool primary;
//  GUID guid;
  TCHAR *adaptername, *adapterid, *adapterkey;
  TCHAR *monitorname, *monitorid;
  TCHAR *fullname;
  struct PicassoResolution *DisplayModes;
  RECT rect;
};

int WIN32GFX_AdjustScreenmode (struct MultiDisplay *md, int *pwidth, int *pheight, int *ppixbits);

struct ScreenResolution {

  uae_u32 width;  /* in pixels */
    uae_u32 height; /* in pixels */
};


struct PicassoResolution
{
	struct ScreenResolution res;
	int depth;   /* depth in bytes-per-pixel */
	int residx;
	int refresh[MAX_REFRESH_RATES]; /* refresh-rates in Hz */
	int refreshtype[MAX_REFRESH_RATES]; /* 0=normal,1=raw,2=lace */
	TCHAR name[25];
	/* Bit mask of RGBFF_xxx values.  */
	uae_u32 colormodes;
	int rawmode;
	bool lace; // all modes lace
};

#endif /* __GFX_H__ */


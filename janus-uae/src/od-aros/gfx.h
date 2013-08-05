#include <intuition/intuition.h>

#include "aros.h"

void DX_Invalidate (int y, int height);
int DX_Fill (int dstx, int dsty, int width, int height, int color, unsigned int rgbtype);

extern struct Window *hAmigaWnd;

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
#define MAX_DISPLAYS 10

struct MultiDisplay {
  int primary, disabled, gdi;
//  GUID guid;
  TCHAR *name, *name2, *name3;
  struct PicassoResolution *DisplayModes;
//  RECT rect;
};

extern struct MultiDisplay Displays[MAX_DISPLAYS];


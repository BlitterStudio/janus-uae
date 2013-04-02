 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Misc support code for AmigaOS target
  *
  * Copyright 2003-2005 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <proto/timer.h>

#include "sleep.h"
#include "osdep/hrtimer.h"

int truncate (const char *path, off_t length)
{
   /* FIXME */
   return 0;
}


int daylight=0; 
int dstbias=0 ;
long timezone=0; 
char tzname[2][4]={"PST", "PDT"};

void tzset(void) {
}

size_t wcscspn( const wchar_t *str, const wchar_t *strCharSet) {
  return strcspn(str, strCharSet);
}

int DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color, int rgbtype);
#if 0
int DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color, int rgbtype) {
  kprintf("WARNING: DX_Fill is not yet implemented!\n");
}
#endif

APTR picasso_memory;

#if 0
uae_u8 *gfx_lock_picasso (void)
{
    return picasso_memory;
}
void gfx_unlock_picasso (void)
{
}
#endif


uae_u8 *veccode;

frame_time_t timebase;

#ifndef __AROS__
int osdep_inithrtimer (void)
{
    static int done = 0;

    if (!done) {
	struct EClockVal etime;

	timebase  = ReadEClock (&etime);
	write_log ("EClock frequency:%.6f MHz\n", timebase/1e6);

	done = 1;
    }

    return 1;
}

frame_time_t osdep_gethrtimebase (void)
{
    return timebase;
}
#endif

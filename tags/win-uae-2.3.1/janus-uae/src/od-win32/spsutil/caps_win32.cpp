
#include "types.h"
#include <stdio.h>

#include "caps_win32.h"

#include <windows.h>

#include "ComType.h"
#include "CapsAPI.h"

static SDWORD caps_cont[4]= {-1, -1, -1, -1};
static int caps_locked[4];
static int caps_flags = DI_LOCK_DENVAR|DI_LOCK_DENNOISE|DI_LOCK_NOISE|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;
#define LIB_TYPE 1

typedef SDWORD (__cdecl* CAPSINIT)(void);
static CAPSINIT pCAPSInit;
typedef SDWORD (__cdecl* CAPSADDIMAGE)(void);
static CAPSADDIMAGE pCAPSAddImage;
typedef SDWORD (__cdecl* CAPSLOCKIMAGEMEMORY)(SDWORD,PUBYTE,UDWORD,UDWORD);
static CAPSLOCKIMAGEMEMORY pCAPSLockImageMemory;
typedef SDWORD (__cdecl* CAPSUNLOCKIMAGE)(SDWORD);
static CAPSUNLOCKIMAGE pCAPSUnlockImage;
typedef SDWORD (__cdecl* CAPSLOADIMAGE)(SDWORD,UDWORD);
static CAPSLOADIMAGE pCAPSLoadImage;
typedef SDWORD (__cdecl* CAPSGETIMAGEINFO)(PCAPSIMAGEINFO,SDWORD);
static CAPSGETIMAGEINFO pCAPSGetImageInfo;
typedef SDWORD (__cdecl* CAPSLOCKTRACK)(PCAPSTRACKINFO,SDWORD,UDWORD,UDWORD,UDWORD);
static CAPSLOCKTRACK pCAPSLockTrack;
typedef SDWORD (__cdecl* CAPSUNLOCKTRACK)(SDWORD,UDWORD);
static CAPSUNLOCKTRACK pCAPSUnlockTrack;
typedef SDWORD (__cdecl* CAPSUNLOCKALLTRACKS)(SDWORD);
static CAPSUNLOCKALLTRACKS pCAPSUnlockAllTracks;
typedef SDWORD (__cdecl* CAPSGETVERSIONINFO)(PCAPSVERSIONINFO,UDWORD);
static CAPSGETVERSIONINFO pCAPSGetVersionInfo;

int caps_init (void)
{
    static int init, noticed;
    int i;
    HMODULE h;
    struct CapsVersionInfo cvi;

    if (init)
	return 1;
    h = LoadLibrary ("CAPSImg.dll");
    if (!h) {
	h = LoadLibrary("plugins\\CAPSImg.dll");
	if (!h) {
	    printf("CAPSImg.dll missing\n");
	    return 0;
	}
    }
    if (GetProcAddress(h, "CAPSLockImageMemory") == 0 || GetProcAddress(h, "CAPSGetVersionInfo") == 0) {
	printf("Too old CAPSImg.dll\n");
	return 0;
    }
    pCAPSInit = (CAPSINIT)GetProcAddress (h, "CAPSInit");
    pCAPSAddImage = (CAPSADDIMAGE)GetProcAddress (h, "CAPSAddImage");
    pCAPSLockImageMemory = (CAPSLOCKIMAGEMEMORY)GetProcAddress (h, "CAPSLockImageMemory");
    pCAPSUnlockImage = (CAPSUNLOCKIMAGE)GetProcAddress (h, "CAPSUnlockImage");
    pCAPSLoadImage = (CAPSLOADIMAGE)GetProcAddress (h, "CAPSLoadImage");
    pCAPSGetImageInfo = (CAPSGETIMAGEINFO)GetProcAddress (h, "CAPSGetImageInfo");
    pCAPSLockTrack = (CAPSLOCKTRACK)GetProcAddress (h, "CAPSLockTrack");
    pCAPSUnlockTrack = (CAPSUNLOCKTRACK)GetProcAddress (h, "CAPSUnlockTrack");
    pCAPSUnlockAllTracks = (CAPSUNLOCKALLTRACKS)GetProcAddress (h, "CAPSUnlockAllTracks");
    pCAPSGetVersionInfo = (CAPSGETVERSIONINFO)GetProcAddress (h, "CAPSGetVersionInfo");
    init = 1;
    cvi.type = LIB_TYPE;
    pCAPSGetVersionInfo (&cvi, 0);
    printf ("CAPS: library version %d.%d\n", cvi.release, cvi.revision);
    for (i = 0; i < 4; i++)
	caps_cont[i] = pCAPSAddImage();
    return 1;
}

void caps_unloadimage (int drv)
{
    if (!caps_locked[drv])
	return;
    pCAPSUnlockAllTracks (caps_cont[drv]);
    pCAPSUnlockImage (caps_cont[drv]);
    caps_locked[drv] = 0;
}

int caps_loadimage (char *filename, int drv, int *num_tracks)
{
    struct CapsImageInfo ci;
    int len, ret;
    uae_u8 *buf;
    char s1[100];
    struct CapsDateTimeExt *cdt;
    FILE *z;

    if (!caps_init ())
	return 0;
    caps_unloadimage (drv);
    z = fopen(filename, "rb");
    if (!z) {
	printf("Couldn't open '%s'\n", filename);
	return 0;
    }
    fseek (z, 0, SEEK_END);
    len = ftell (z);
    fseek (z, 0, SEEK_SET);
    buf = malloc (len);
    if (!buf) {
	printf("out of memory\n");
	return 0;
    }
    if (fread (buf, len, 1, z) == 0) {
	printf("read error\n");
	return 0;
    }
    ret = pCAPSLockImageMemory(caps_cont[drv], buf, len, 0);
    free (buf);
    if (ret != imgeOk) {
	printf("Corrupt or incompatible image, error %d\n", ret);
	free (buf);
	return 0;
    }
    caps_locked[drv] = 1;
    pCAPSGetImageInfo(&ci, caps_cont[drv]);
    *num_tracks = (ci.maxcylinder - ci.mincylinder + 1) * (ci.maxhead - ci.minhead + 1);
    pCAPSLoadImage(caps_cont[drv], caps_flags);
    cdt = &ci.crdt;
    sprintf (s1, "%d.%d.%d %d:%d:%d", cdt->day, cdt->month, cdt->year, cdt->hour, cdt->min, cdt->sec);
    printf ("type:%d date:%s rel:%d rev:%d tracks:%d\n",
	ci.type, s1, ci.release, ci.revision, *num_tracks);
    return 1;
}

#if 0
static void outdisk (void)
{
    struct CapsTrackInfo ci;
    int tr;
    FILE *f;
    static int done;

    if (done)
	return;
    done = 1;
    f = fopen("c:\\out3.dat", "wb");
    if (!f)
	return;
    for (tr = 0; tr < 160; tr++) {
	pCAPSLockTrack(&ci, caps_cont[0], tr / 2, tr & 1, caps_flags);
	fwrite (ci.trackdata[0], ci.tracksize[0], 1, f);
	fwrite ("XXXX", 4, 1, f);
    }
    fclose (f);
}
#endif

int caps_loadrevolution (uae_u16 *mfmbuf, int drv, int track, int *tracklength)
{
    int len, i;
    uae_u16 *mfm;
    struct CapsTrackInfoT1 ci;

    ci.type = LIB_TYPE;
    pCAPSLockTrack((PCAPSTRACKINFO)&ci, caps_cont[drv], track / 2, track & 1, caps_flags);
    len = ci.tracklen;
    *tracklength = len * 8;
    mfm = mfmbuf;
    for (i = 0; i < (len + 1) / 2; i++) {
	uae_u8 *data = ci.trackbuf + i * 2;
	*mfm++ = 256 * *data + *(data + 1);
    }
    return 1;
}

int caps_loadtrack (uae_u16 *mfmbuf, uae_u16 *tracktiming, int drv, int track, int *tracklength, int *multirev, int *gapoffset)
{
    int i, len, type;
    uae_u16 *mfm;
    struct CapsTrackInfoT1 ci;

    ci.type = LIB_TYPE;
    *tracktiming = 0;
    pCAPSLockTrack((PCAPSTRACKINFO)&ci, caps_cont[drv], track / 2, track & 1, caps_flags);
    mfm = mfmbuf;
    *multirev = (ci.type & CTIT_FLAG_FLAKEY) ? 1 : 0;
    type = ci.type & CTIT_MASK_TYPE;
    len = ci.tracklen;
    *tracklength = len * 8;
    *gapoffset = ci.overlap >= 0 ? ci.overlap * 8 : -1;
    for (i = 0; i < (len + 1) / 2; i++) {
	uae_u8 *data = ci.trackbuf + i * 2;
	*mfm++ = 256 * *data + *(data + 1);
    }
#if 0
    {
	FILE *f=fopen("c:\\1.txt","wb");
	fwrite (ci.trackbuf, len, 1, f);
	fclose (f);
    }
#endif
    if (ci.timelen > 0) {
	for (i = 0; i < ci.timelen; i++)
	    tracktiming[i] = (uae_u16)ci.timebuf[i];
    }
#if 0
    write_log ("caps: drive:%d track:%d len:%d multi:%d timing:%d type:%d overlap:%d\n",
	drv, track, len, *multirev, ci.timelen, type, ci.overlap);
#endif
    return 1;
}

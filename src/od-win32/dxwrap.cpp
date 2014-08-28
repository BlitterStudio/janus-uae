#include "sysconfig.h"

#include "sysdeps.h"
#include "options.h"

#include "dxwrap.h"
#include "win32gfx.h"
#include "statusline.h"

#include <d3d9.h>
#include <dxerr.h>


struct ddstuff dxdata;
struct ddcaps dxcaps;
static int flipinterval_supported = 1;
int ddforceram = DDFORCED_DEFAULT;

HRESULT DirectDraw_GetDisplayMode (void)
{
	HRESULT ddrval;

	dxdata.native.dwSize = sizeof (DDSURFACEDESC2);
	ddrval = IDirectDraw7_GetDisplayMode (dxdata.maindd, &dxdata.native);
	if (FAILED (ddrval))
		write_log (L"IDirectDraw7_GetDisplayMode: %s\n", DXError (ddrval));
	return ddrval;
}

#define releaser(x, y) if (x) { y (x); x = NULL; }

static LPDIRECTDRAWSURFACE7 getlocksurface (void)
{
	if (dxdata.backbuffers > 0 && currprefs.gfx_afullscreen > 0 && !WIN32GFX_IsPicassoScreen ())
		return dxdata.flipping[0];
	return dxdata.secondary;
}

static void freemainsurface (void)
{
	if (dxdata.dclip) {
		DirectDraw_SetClipper (NULL);
		releaser (dxdata.dclip, IDirectDrawClipper_Release);
	}
	releaser (dxdata.flipping[1], IDirectDrawSurface7_Release);
	releaser (dxdata.flipping[0], IDirectDrawSurface7_Release);
	releaser (dxdata.primary, IDirectDrawSurface7_Release);
	releaser (dxdata.secondary, IDirectDrawSurface7_Release);
	releaser (dxdata.statussurface, IDirectDrawSurface7_Release);
	dxdata.backbuffers = 0;
}

static HRESULT restoresurface_2 (LPDIRECTDRAWSURFACE7 surf)
{
	HRESULT ddrval;

	if (surf == dxdata.flipping[0] || surf == dxdata.flipping[1])
		surf = dxdata.primary;
	ddrval = IDirectDrawSurface7_IsLost (surf);
	if (SUCCEEDED (ddrval))
		return ddrval;
	ddrval = IDirectDrawSurface7_Restore (surf);
	return ddrval;
}

HRESULT restoresurface (LPDIRECTDRAWSURFACE7 surf)
{
	HRESULT ddrval;

	if (surf == NULL)
		return E_FAIL;
	if (surf == dxdata.flipping[0] || surf == dxdata.flipping[1])
		surf = dxdata.primary;
	ddrval = IDirectDrawSurface7_IsLost (surf);
	if (SUCCEEDED (ddrval))
		return ddrval;
	ddrval = IDirectDrawSurface7_Restore (surf);
	if (FAILED (ddrval)) {
		write_log (L"IDirectDrawSurface7_Restore: %s\n", DXError (ddrval));
	}
	return ddrval;
}

static HRESULT restoresurfacex (LPDIRECTDRAWSURFACE7 surf1, LPDIRECTDRAWSURFACE7 surf2)
{
	HRESULT r1, r2;

	r1 = restoresurface (surf1);
	r2 = restoresurface (surf2);
	if (SUCCEEDED (r1) && SUCCEEDED (r2))
		return r1;
	if (SUCCEEDED (r1))
		return r2;
	return r1;
}

static void clearsurf (LPDIRECTDRAWSURFACE7 surf, DWORD color)
{
	HRESULT ddrval;
	DDBLTFX ddbltfx;

	if (surf == NULL)
		return;
	memset(&ddbltfx, 0, sizeof (ddbltfx));
	ddbltfx.dwFillColor = color;
	ddbltfx.dwSize = sizeof (ddbltfx);
	while (FAILED (ddrval = IDirectDrawSurface7_Blt (surf, NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx))) {
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = restoresurface (surf);
			if (FAILED (ddrval))
				break;
		}
		break;
	}
}

void clearsurface (LPDIRECTDRAWSURFACE7 surf)
{
	if (surf == NULL)
		surf = getlocksurface ();
	clearsurf (surf, 0);
}


int locksurface (LPDIRECTDRAWSURFACE7 surf, LPDDSURFACEDESC2 desc)
{
	static int cnt = 50;
	HRESULT ddrval;
	desc->dwSize = sizeof (*desc);
	while (FAILED (ddrval = IDirectDrawSurface7_Lock (surf, NULL, desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL))) {
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = restoresurface_2 (surf);
			if (FAILED (ddrval))
				return 0;
		} else if (ddrval != DDERR_SURFACEBUSY) {
			if (cnt > 0) {
				cnt--;
				write_log (L"locksurface %d: %s\n", cnt, DXError (ddrval));
			}
			return 0;
		}
	}
	return 1;
}
void unlocksurface (LPDIRECTDRAWSURFACE7 surf)
{
	HRESULT ddrval;

	ddrval = IDirectDrawSurface7_Unlock (surf, NULL);
	if (FAILED (ddrval))
		write_log (L"IDirectDrawSurface7_Unlock: %s\n", DXError (ddrval));
}

static void setsurfacecap (DDSURFACEDESC2 *desc, int w, int h, int mode)
{
	desc->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
	if (mode >= DDFORCED_DEFAULT)
		desc->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	if (mode == DDFORCED_VIDMEM)
		desc->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	if (w > dxcaps.maxwidth || h > dxcaps.maxheight || mode == DDFORCED_SYSMEM)
		desc->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	desc->dwWidth = w;
	desc->dwHeight = h;
}

STATIC_INLINE uae_u16 rgb32torgb16pc (uae_u32 rgb)
{
	return (((rgb >> (16 + 3)) & 0x1f) << 11) | (((rgb >> (8 + 2)) & 0x3f) << 5) | (((rgb >> (0 + 3)) & 0x1f) << 0);
}

static TCHAR *alloctexts[] = { L"NonLocalVRAM", L"DefaultRAM", L"VRAM", L"RAM" };
static LPDIRECTDRAWSURFACE7 allocsurface_3 (int width, int height, uae_u8 *ptr, int pitch, int ck, int forcemode)
{
	HRESULT ddrval;
	DDSURFACEDESC2 desc;
	LPDIRECTDRAWSURFACE7 surf = NULL;

	memset (&desc, 0, sizeof desc);
	desc.dwSize = sizeof (desc);
	desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	setsurfacecap (&desc, width, height, forcemode);
	memcpy (&desc.ddpfPixelFormat, &dxdata.native.ddpfPixelFormat, sizeof (DDPIXELFORMAT));

	if (ck) {
		DWORD mask = 0xff00fe;
		if (desc.ddpfPixelFormat.dwRGBBitCount == 16)
			mask = rgb32torgb16pc (mask);
		else if (desc.ddpfPixelFormat.dwRGBBitCount == 8)
			mask = 16;
		dxdata.colorkey = mask;
		if (dxcaps.cancolorkey) {
			desc.dwFlags |= DDSD_CKSRCBLT;
			desc.ddckCKSrcBlt.dwColorSpaceLowValue = mask;
			desc.ddckCKSrcBlt.dwColorSpaceHighValue = mask;
		}
	}

	if (ptr) {
		desc.dwFlags |= DDSD_LPSURFACE | DDSD_PITCH;
		desc.lPitch = pitch;
		desc.lpSurface = ptr;
	}
	ddrval = IDirectDraw7_CreateSurface (dxdata.maindd, &desc, &surf, NULL);
	if (FAILED (ddrval)) {
		write_log (L"IDirectDraw7_CreateSurface (%dx%d,%s): %s\n", width, height, alloctexts[forcemode], DXError (ddrval));
	} else {
		write_log (L"Created %dx%dx%d (%p) surface in %s (%d)%s\n", width, height, desc.ddpfPixelFormat.dwRGBBitCount, surf,
			alloctexts[forcemode], forcemode, ck ? (dxcaps.cancolorkey ? L" hardware colorkey" : L" software colorkey") : L"");
	}
	return surf;
}

static LPDIRECTDRAWSURFACE7 allocsurface_2 (int width, int height, int ck)
{
	LPDIRECTDRAWSURFACE7 s;
	int mode = ddforceram;
	static int failednonlocal;

	if (failednonlocal && mode == DDFORCED_NONLOCAL)
		mode = DDFORCED_DEFAULT;
	for (;;) {
		s = allocsurface_3 (width, height, NULL, 0, ck, mode);
		if (s) {
			clearsurf (s, 0);
			return s;
		}
		if (mode == DDFORCED_NONLOCAL)
			failednonlocal = 1;
		mode++;
		if (mode >= 4)
			mode = 0;
		if (mode == ddforceram)
			return NULL;
	}
}

LPDIRECTDRAWSURFACE7 allocsurface (int width, int height)
{
	return allocsurface_2 (width, height, FALSE);
}
LPDIRECTDRAWSURFACE7 allocsystemsurface (int width, int height)
{
	return allocsurface_3 (width, height, NULL, 0, FALSE, DDFORCED_SYSMEM);
}

LPDIRECTDRAWSURFACE7 createsurface (uae_u8 *ptr, int pitch, int width, int height)
{
	return allocsurface_3 (width, height, ptr, pitch, FALSE, DDFORCED_SYSMEM);
}

void freesurface (LPDIRECTDRAWSURFACE7 surf)
{
	if (surf)
		IDirectDrawSurface7_Release (surf);
}

void DirectDraw_FreeMainSurface (void)
{
	freemainsurface ();
}

#if 0
static int testck2 (LPDIRECTDRAWSURFACE7 tmp, RECT *r)
{
	DDSURFACEDESC2 desc;
	if (locksurface (tmp, &desc)) {
		uae_u8 *p = (uae_u8*)desc.lpSurface + r->top * desc.lPitch + r->left * desc.ddpfPixelFormat.dwRGBBitCount / 8;
		DWORD v1 = ((uae_u32*)p)[0];
		DWORD v2 = ((uae_u32*)p)[1];
		unlocksurface (tmp);
		// no more black = failure
		if (v1 != 0 || v2 != 0)
			return 0;
	}
	return 1;
}

int dx_testck (void)
{
	int failed = 0;
	LPDIRECTDRAWSURFACE7 cksurf;
	LPDIRECTDRAWSURFACE7 tmp;
	RECT r1;
	int x;

	cksurf = dxdata.cursorsurface1;
	tmp = dxdata.secondary;
	if (!dxcaps.cancolorkey || !cksurf || !tmp)
		return 1;
	r1.left = 0;
	r1.top = 0;
	r1.right = dxcaps.cursorwidth;
	r1.bottom = dxcaps.cursorheight;
	failed = 0;
	// test by blitting surface filled with color key color to destination filled with black
	clearsurf (cksurf, dxdata.colorkey);
	clearsurf (tmp, 0);
	for (x = 0; x < 16; x++) {
		DirectDraw_BlitRectCK (tmp, &r1, cksurf, NULL);
		if (!testck2 (tmp, &r1)) // non-black = failed
			failed = 1;
		r1.left++;
		r1.right++;
		if (x & 1) {
			r1.top++;
			r1.bottom++;
		}
	}
	clearsurface (cksurf);
	clearsurface (tmp);
	if (failed) {
		write_log (L"Color key test failure, display driver bug, falling back to software emulation.\n");
		dxcaps.cancolorkey = 0;
		releaser (dxdata.cursorsurface1, IDirectDrawSurface7_Release);
		dxdata.cursorsurface1 = allocsurface_2 (dxcaps.cursorwidth, dxcaps.cursorheight, TRUE);
		return 0;
	}
	return 1;
}
#endif

static void createstatussurface (void)
{
	releaser (dxdata.statussurface, IDirectDrawSurface7_Release);
	dxdata.statussurface = allocsurface_2 (dxdata.statuswidth, dxdata.statusheight, FALSE);
	if (dxdata.statussurface)
		clearsurf (dxdata.statussurface, 0);
}

HRESULT DirectDraw_CreateMainSurface (int width, int height)
{
	HRESULT ddrval;
	DDSURFACEDESC2 desc = { 0 };
	LPDIRECTDRAWSURFACE7 surf;

	width = (width + 7) & ~7;
	desc.dwSize = sizeof (desc);
	desc.dwFlags = DDSD_CAPS;
	desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (dxdata.fsmodeset) {
		int ok = 0;
		DWORD oldcaps = desc.ddsCaps.dwCaps;
		DWORD oldflags = desc.dwFlags;
		desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
		desc.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
		desc.dwBackBufferCount = currprefs.gfx_backbuffers;
		if (desc.dwBackBufferCount > 0) {
			ddrval = IDirectDraw7_CreateSurface (dxdata.maindd, &desc, &dxdata.primary, NULL);
			if (SUCCEEDED (ddrval)) {
				DDSCAPS2 ddscaps;
				memset (&ddscaps, 0, sizeof (ddscaps));
				ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
				ddrval = IDirectDrawSurface7_GetAttachedSurface (dxdata.primary, &ddscaps, &dxdata.flipping[0]);
				if(SUCCEEDED (ddrval)) {
					if (desc.dwBackBufferCount > 1) {
						memset (&ddscaps, 0, sizeof (ddscaps));
						ddscaps.dwCaps = DDSCAPS_FLIP;
						ddrval = IDirectDrawSurface7_GetAttachedSurface (dxdata.flipping[0], &ddscaps, &dxdata.flipping[1]);
					}
				}
				if (FAILED (ddrval))
					write_log (L"IDirectDrawSurface7_GetAttachedSurface: %s\n", DXError (ddrval));
				ok = 1;
			}
		}
		if (!ok) {
			desc.dwBackBufferCount = 0;
			desc.ddsCaps.dwCaps = oldcaps;
			desc.dwFlags = oldflags;
			ddrval = IDirectDraw7_CreateSurface (dxdata.maindd, &desc, &dxdata.primary, NULL);
		}
	} else {
		ddrval = IDirectDraw7_CreateSurface (dxdata.maindd, &desc, &dxdata.primary, NULL);
	}
	if (FAILED (ddrval)) {
		write_log (L"IDirectDraw7_CreateSurface: %s\n", DXError (ddrval));
		return ddrval;
	}
	dxdata.native.dwSize = sizeof (DDSURFACEDESC2);
	ddrval = IDirectDrawSurface7_GetSurfaceDesc (dxdata.primary, &dxdata.native);
	if (FAILED (ddrval))
		write_log (L"IDirectDrawSurface7_GetSurfaceDesc: %s\n", DXError (ddrval));
	if (dxdata.fsmodeset) {
		clearsurf (dxdata.primary, 0);
		dxdata.fsmodeset = 1;
	}
	dxdata.backbuffers = desc.dwBackBufferCount;
	clearsurf (dxdata.flipping[0], 0);
	clearsurf (dxdata.flipping[1], 0);
	surf = allocsurface (width, height);
	if (surf) {
		dxdata.secondary = surf;
		dxdata.swidth = width;
		dxdata.sheight = height;
		dxdata.pitch = 0;
		if (locksurface (surf, &desc)) {
			dxdata.pitch = desc.lPitch;
			unlocksurface (surf);
		} else {
			write_log (L"Couldn't get surface pitch!\n");
		}
		createstatussurface ();
	} else {
		ddrval = DD_FALSE;
	}
	write_log (L"DDRAW: primary surface %p, secondary %p (%dx%dx%d) bb=%d\n",
		dxdata.primary, surf, width, height, dxdata.native.ddpfPixelFormat.dwRGBBitCount, dxdata.backbuffers);
	return ddrval;
}

HRESULT DirectDraw_SetDisplayMode (int width, int height, int bits, int freq)
{
	HRESULT ddrval;

	if (dxdata.fsmodeset && dxdata.width == width && dxdata.height == height &&
		dxdata.depth == bits && dxdata.freq == freq)
		return DD_OK;
	ddrval = IDirectDraw7_SetDisplayMode (dxdata.maindd, width, height, bits, freq, 0);
	if (FAILED (ddrval)) {
		write_log (L"IDirectDraw7_SetDisplayMode: %s\n", DXError (ddrval));
		IDirectDraw7_RestoreDisplayMode (dxdata.maindd);
		dxdata.fsmodeset = 0;
	} else {
		dxdata.fsmodeset = -1;
		dxdata.width = width;
		dxdata.height = height;
		dxdata.depth = bits;
		dxdata.freq = freq;
	}
	return ddrval;
}

HRESULT DirectDraw_SetCooperativeLevel (HWND window, int fullscreen, int doset)
{
	HRESULT ddrval;

	if (doset) {
		dxdata.hwnd = window;
		ddrval = IDirectDraw7_SetCooperativeLevel (dxdata.maindd, window, fullscreen ?
			DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN : DDSCL_NORMAL);
		if (FAILED (ddrval))
			write_log (L"IDirectDraw7_SetCooperativeLevel: SET %s\n", DXError (ddrval));
	} else {
		ddrval = IDirectDraw7_SetCooperativeLevel (dxdata.maindd, dxdata.hwnd, DDSCL_NORMAL);
		if (FAILED (ddrval))
			write_log (L"IDirectDraw7_SetCooperativeLevel: RESET %s\n", DXError (ddrval));
	}
	return ddrval;
}

HRESULT DirectDraw_CreateClipper (void)
{
	HRESULT ddrval;

	ddrval = IDirectDraw7_CreateClipper (dxdata.maindd, 0, &dxdata.dclip, NULL);
	if (FAILED (ddrval))
		write_log (L"IDirectDraw7_CreateClipper: %s\n", DXError (ddrval));
	return ddrval;
}

HRESULT DirectDraw_SetClipper (HWND hWnd)
{
	HRESULT ddrval;

	if (dxdata.primary == NULL)
		return DD_FALSE;
	ddrval = IDirectDrawSurface7_SetClipper (dxdata.primary, hWnd ? dxdata.dclip : NULL);
	if (FAILED (ddrval))
		write_log (L"IDirectDrawSurface7_SetClipper: %s\n", DXError (ddrval));
	if(hWnd && SUCCEEDED (ddrval)) {
		ddrval = IDirectDrawClipper_SetHWnd (dxdata.dclip, 0, hWnd);
		if (FAILED (ddrval))
			write_log (L"IDirectDrawClipper_SetHWnd: %s\n", DXError (ddrval));
	}
	return ddrval;
}


TCHAR *outGUID (const GUID *guid)
{
	static TCHAR gb[64];
	if (guid == NULL)
		return L"NULL";
	_stprintf (gb, L"%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
		guid->Data1, guid->Data2, guid->Data3,
		guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
		guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
	return gb;
}

const TCHAR *DXError (HRESULT ddrval)
{
	static TCHAR dderr[1000];
	_stprintf (dderr, L"%08X S=%d F=%04X C=%04X (%d) (%s)",
		ddrval, (ddrval & 0x80000000) ? 1 : 0,
		HRESULT_FACILITY(ddrval),
		HRESULT_CODE(ddrval),
		HRESULT_CODE(ddrval),
		DXGetErrorDescription (ddrval));
	return dderr;
}

RGBFTYPE DirectDraw_GetSurfacePixelFormat (LPDDSURFACEDESC2 surface)
{
	int surface_is = 0;
	DDPIXELFORMAT *pfp = NULL;
	DWORD r, g, b;
	DWORD surf_flags;

	if (surface == NULL)
		surface = &dxdata.native;
	surf_flags = surface->dwFlags;
	pfp = &surface->ddpfPixelFormat;

	if ((surf_flags & DDSD_PIXELFORMAT) == 0x0)
		return RGBFB_NONE;

	if ((pfp->dwFlags & DDPF_RGB) == 0)
		return RGBFB_NONE;

	r = pfp->dwRBitMask;
	g = pfp->dwGBitMask;
	b = pfp->dwBBitMask;
	switch (pfp->dwRGBBitCount)
	{
	case 8:
		break;

	case 16:
		if (r == 0xF800 && g == 0x07E0 && b == 0x001F)
			return RGBFB_R5G6B5PC;
		if (r == 0x7C00 && g == 0x03E0 && b == 0x001F)
			return RGBFB_R5G5B5PC;
		if (b == 0xF800 && g == 0x07E0 && r == 0x001F)
			return RGBFB_B5G6R5PC;
		if (b == 0x7C00 && g == 0x03E0 && r == 0x001F)
			return RGBFB_B5G5R5PC;
		break;

	case 24:
		if (r == 0xFF0000 && g == 0x00FF00 && b == 0x0000FF)
			return RGBFB_B8G8R8;
		if (r == 0x0000FF && g == 0x00FF00 && b == 0xFF0000)
			return RGBFB_R8G8B8;
		break;

	case 32:
		if (r == 0x00FF0000 && g == 0x0000FF00 && b == 0x000000FF)
			return RGBFB_B8G8R8A8;
		if (r == 0x000000FF && g == 0x0000FF00 && b == 0x00FF0000)
			return RGBFB_R8G8B8A8;
		if (r == 0xFF000000 && g == 0x00FF0000 && b == 0x0000FF00)
			return RGBFB_A8B8G8R8;
		if (r == 0x0000FF00 && g == 0x00FF0000 && b == 0xFF000000)
			return RGBFB_A8R8G8B8;
		break;

	default:
		write_log (L"Unknown %d bit format %d %d %d\n", pfp->dwRGBBitCount, r, g, b);
		break;
	}
	return RGBFB_NONE;
}

HRESULT DirectDraw_EnumDisplayModes (DWORD flags, LPDDENUMMODESCALLBACK2 callback, void *context)
{
	HRESULT result;
	result = IDirectDraw7_EnumDisplayModes (dxdata.maindd, flags, NULL, context, callback);
	return result;
}

HRESULT DirectDraw_EnumDisplays (LPDDENUMCALLBACKEXA callback)
{
	HRESULT result;
	result = DirectDrawEnumerateExA (callback, 0, DDENUM_DETACHEDSECONDARYDEVICES | DDENUM_ATTACHEDSECONDARYDEVICES);
	return result;
}

DWORD DirectDraw_CurrentWidth (void)
{
	return dxdata.native.dwWidth;
}
DWORD DirectDraw_CurrentHeight (void)
{
	return dxdata.native.dwHeight;
}
DWORD DirectDraw_GetCurrentDepth (void)
{
	return dxdata.native.ddpfPixelFormat.dwRGBBitCount;
}

int DirectDraw_SurfaceLock (void)
{
	int ok;
	LPDIRECTDRAWSURFACE7 surf;

	surf = getlocksurface ();
	if (surf == NULL)
		return 0;
	if (FAILED (IDirectDrawSurface7_IsLost (surf))) {
		restoresurface (surf);
		return 0;
	}
	if (dxdata.lockcnt > 0)
		return 1;
	ok = locksurface (getlocksurface (), &dxdata.locksurface);
	if (ok)
		dxdata.lockcnt++;
	return ok;
}
void DirectDraw_SurfaceUnlock (void)
{
	if (dxdata.lockcnt < 0)
		write_log (L"DirectDraw_SurfaceUnlock negative lock count %d!\n", dxdata.lockcnt);
	if (dxdata.lockcnt == 0)
		return;
	dxdata.lockcnt--;
	unlocksurface (getlocksurface ());
}

uae_u8 *DirectDraw_GetSurfacePointer (void)
{
	return (uae_u8*)dxdata.locksurface.lpSurface;
}
DWORD DirectDraw_GetSurfacePitch (void)
{
	return dxdata.locksurface.lPitch;
}
int DirectDraw_IsLocked (void)
{
	return dxdata.lockcnt;
}
DWORD DirectDraw_GetPixelFormatBitMask (DirectDraw_Mask_e mask)
{
	DWORD result = 0;
	switch(mask)
	{
	case red_mask:
		result = dxdata.native.ddpfPixelFormat.dwRBitMask;
		break;
	case green_mask:
		result = dxdata.native.ddpfPixelFormat.dwGBitMask;
		break;
	case blue_mask:
		result = dxdata.native.ddpfPixelFormat.dwBBitMask;
		break;
	}
	return result;
}
RGBFTYPE DirectDraw_GetPixelFormat (void)
{
	return (RGBFTYPE)DirectDraw_GetSurfacePixelFormat (&dxdata.native);
}
DWORD DirectDraw_GetBytesPerPixel (void)
{
	return (dxdata.native.ddpfPixelFormat.dwRGBBitCount + 7) >> 3;
}

HRESULT DirectDraw_GetDC (HDC *hdc)
{
	if (getlocksurface () == NULL)
		return E_FAIL;
	return IDirectDrawSurface7_GetDC (getlocksurface (), hdc);
}
HRESULT DirectDraw_ReleaseDC (HDC hdc)
{
	if (getlocksurface () == NULL)
		return E_FAIL;
	return IDirectDrawSurface7_ReleaseDC (getlocksurface (), hdc);
}
int DirectDraw_GetVerticalBlankStatus (void)
{
	BOOL status;
	if (FAILED (IDirectDraw7_GetVerticalBlankStatus (dxdata.maindd, &status)))
		return -1;
	return status;
}
void DirectDraw_GetPrimaryPixelFormat (DDSURFACEDESC2 *desc)
{
	memcpy (&desc->ddpfPixelFormat, &dxdata.native.ddpfPixelFormat, sizeof (DDPIXELFORMAT));
	desc->dwFlags |= DDSD_PIXELFORMAT;
}
DWORD DirectDraw_CurrentRefreshRate (void)
{
	DirectDraw_GetDisplayMode ();
	return dxdata.native.dwRefreshRate;
}

HRESULT DirectDraw_FlipToGDISurface (void)
{
	if (!dxdata.ddinit || !dxdata.fsmodeset)
		return DD_OK;
	return IDirectDraw7_FlipToGDISurface (dxdata.maindd);
}

int DirectDraw_BlitToPrimaryScale (RECT *dstrect, RECT *srcrect)
{
	LPDIRECTDRAWSURFACE7 dst;
	int result = 0;
	HRESULT ddrval;
	RECT dstrect2;
	int x = 0, y = 0, w = dxdata.swidth, h = dxdata.sheight;

	dst = dxdata.primary;
	if (dstrect == NULL) {
		dstrect = &dstrect2;
		SetRect (dstrect, x, y, x + w, y + h);
	}
	centerdstrect (dstrect);
	while (FAILED (ddrval = IDirectDrawSurface7_Blt (dst, dstrect, dxdata.secondary, srcrect, DDBLT_WAIT, NULL))) {
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = restoresurfacex (dst, dxdata.secondary);
			if (FAILED (ddrval))
				return 0;
		} else if (ddrval != DDERR_SURFACEBUSY) {
			write_log (L"DirectDraw_BlitToPrimary: %s\n", DXError (ddrval));
			if (srcrect)
				write_log (L"SRC=%dx%d %dx%d\n", srcrect->left, srcrect->top, srcrect->right, srcrect->bottom);
			if (srcrect)
				write_log (L"DST=%dx%d %dx%d\n", dstrect->left, dstrect->top, dstrect->right, dstrect->bottom);
			break;
		}
	}
	if (SUCCEEDED(ddrval))
		result = 1;
	return result;
}

static int DirectDraw_BlitToPrimary2 (RECT *rect, int dooffset)
{
	LPDIRECTDRAWSURFACE7 dst;
	int result = 0;
	HRESULT ddrval;
	RECT srcrect, dstrect;
	int x = 0, y = 0, w = dxdata.swidth, h = dxdata.sheight;

	dst = dxdata.primary;
	if (dst == NULL)
		return DD_FALSE;
	if (rect) {
		x = rect->left;
		y = rect->top;
		w = rect->right - rect->left;
		h = rect->bottom - rect->top;
	}
	if (w > dxdata.swidth - x)
		w = dxdata.swidth - x;
	if (h > dxdata.sheight - y)
		h = dxdata.sheight - y;
	SetRect (&srcrect, x, y, x + w, y + h);
	SetRect (&dstrect, x, y, x + w, y + h);
	if (rect || dooffset)
		centerdstrect (&dstrect);
	while (FAILED(ddrval = IDirectDrawSurface7_Blt (dst, &dstrect, dxdata.secondary, &srcrect, DDBLT_WAIT, NULL))) {
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = restoresurfacex (dst, dxdata.secondary);
			if (FAILED (ddrval))
				return 0;
		} else if (ddrval != DDERR_SURFACEBUSY) {
			write_log (L"DirectDraw_BlitToPrimary: %s\n", DXError (ddrval));
			break;
		}
	}
	if (SUCCEEDED(ddrval))
		result = 1;
	return result;
}

int DirectDraw_BlitToPrimary (RECT *rect)
{
	return DirectDraw_BlitToPrimary2 (rect, FALSE);
}

static int DirectDraw_Blt_EmuCK (LPDIRECTDRAWSURFACE7 dst, RECT *dstrect, LPDIRECTDRAWSURFACE7 src, RECT *srcrect)
{
	DDSURFACEDESC2 dstd, srcd;
	int x, y, w, h, bpp;
	int sx, sy, dx, dy;
	int ok;
	DWORD ck;

	ok = 0;
	ck = dxdata.colorkey;
	sx = sy = dx = dy = 0;
	if (srcrect) {
		sx = srcrect->left;
		sy = srcrect->top;
	}
	if (dstrect) {
		dx = dstrect->left;
		dy = dstrect->top;
	}
	if (locksurface (dst, &dstd)) {
		if (locksurface (src, &srcd)) {
			bpp = srcd.ddpfPixelFormat.dwRGBBitCount / 8;
			h = srcd.dwHeight;
			w = srcd.dwWidth;
			if (srcrect)
				w = srcrect->right - srcrect->left;
			if (srcrect)
				h = srcrect->bottom - srcrect->top;
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					uae_u8 *sp = (uae_u8*)srcd.lpSurface + srcd.lPitch * (y + sy) + (x + sx) * bpp;
					uae_u8 *dp = (uae_u8*)dstd.lpSurface + dstd.lPitch * (y + dy) + (x + dx) * bpp;
					if (bpp == 1) {
						if (*sp != ck)
							*dp = *sp;
					} else if (bpp == 2) {
						if (((uae_u16*)sp)[0] != ck)
							((uae_u16*)dp)[0] = ((uae_u16*)sp)[0];
					} else if (bpp == 4) {
						if (((uae_u32*)sp)[0] != ck)
							((uae_u32*)dp)[0] = ((uae_u32*)sp)[0];
					}
				}
			}
			ok = 1;
			unlocksurface (src);
		}
		unlocksurface (dst);
	}
	return ok;
}

static int DirectDraw_Blt (LPDIRECTDRAWSURFACE7 dst, RECT *dstrect, LPDIRECTDRAWSURFACE7 src, RECT *srcrect, int ck)
{
	HRESULT ddrval;

	if (dst == NULL)
		dst = getlocksurface ();
	if (src == NULL)
		src = getlocksurface ();
	if (dst == src)
		return 1;
	if (ck && dxcaps.cancolorkey == 0)
		return DirectDraw_Blt_EmuCK (dst, dstrect, src, srcrect);
	while (FAILED(ddrval = IDirectDrawSurface7_Blt (dst, dstrect, src, srcrect, DDBLT_WAIT | (ck ? DDBLT_KEYSRC : 0), NULL))) {
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = restoresurfacex (dst, src);
			if (FAILED (ddrval))
				return 0;
		} else if (ddrval != DDERR_SURFACEBUSY) {
			write_log (L"DirectDraw_Blit: %s\n", DXError (ddrval));
			return 0;
		}
	}
	return 1;
}
int DirectDraw_Blit (LPDIRECTDRAWSURFACE7 dst, LPDIRECTDRAWSURFACE7 src)
{
	return DirectDraw_Blt (dst, NULL, src, NULL, FALSE);
}
int DirectDraw_BlitRect (LPDIRECTDRAWSURFACE7 dst, RECT *dstrect, LPDIRECTDRAWSURFACE7 src, RECT *scrrect)
{
	return DirectDraw_Blt (dst, dstrect, src, scrrect, FALSE);
}
int DirectDraw_BlitRectCK (LPDIRECTDRAWSURFACE7 dst, RECT *dstrect, LPDIRECTDRAWSURFACE7 src, RECT *scrrect)
{
	return DirectDraw_Blt (dst, dstrect, src, scrrect, TRUE);
}

void DirectDraw_FillSurface (LPDIRECTDRAWSURFACE7 dst, RECT *rect, uae_u32 color)
{
	HRESULT ddrval;
	DDBLTFX ddbltfx;

	if (!dst)
		return;
	memset (&ddbltfx, 0, sizeof (ddbltfx));
	ddbltfx.dwFillColor = color;
	ddbltfx.dwSize = sizeof (ddbltfx);
	while (FAILED (ddrval = IDirectDrawSurface7_Blt (dst, rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx))) {
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = restoresurface (dst);
			if (FAILED (ddrval))
				break;
		} else if (ddrval != DDERR_SURFACEBUSY) {
			write_log (L"DirectDraw_Fill: %s\n", DXError (ddrval));
			break;
		}
	}

}

void DirectDraw_Fill (RECT *rect, uae_u32 color)
{
	DirectDraw_FillSurface (getlocksurface (), rect, color);
}

void DirectDraw_FillPrimary (void)
{
	DirectDraw_FillSurface (dxdata.primary, NULL, 0);
}

extern int vblank_skip;
static void flip (void)
{
	int result = 0;
	HRESULT ddrval = DD_OK;
	DWORD flags = DDFLIP_WAIT;

	if (currprefs.turbo_emulation)
		flags |= DDFLIP_NOVSYNC;
	if (dxdata.backbuffers == 2) {
		DirectDraw_Blit (dxdata.flipping[1], dxdata.flipping[0]);
		if (currprefs.gfx_avsync) {
			if (vblank_skip >= 0 || currprefs.turbo_emulation) {
				ddrval = IDirectDrawSurface7_Flip (dxdata.primary, NULL, flags);
			} else {
				if (flipinterval_supported) {
					ddrval = IDirectDrawSurface7_Flip (dxdata.primary, NULL, flags | DDFLIP_INTERVAL2);
				} else {
					ddrval = IDirectDrawSurface7_Flip (dxdata.primary, NULL, flags);
					DirectDraw_Blit (dxdata.flipping[1], dxdata.primary);
					ddrval = IDirectDrawSurface7_Flip (dxdata.primary, NULL, flags);
				}
			}
		} else {
			ddrval = IDirectDrawSurface7_Flip (dxdata.primary, NULL, flags| DDFLIP_NOVSYNC);
		}
	} else if(dxdata.backbuffers == 1) {
		if (currprefs.gfx_avsync) { 
			ddrval = IDirectDrawSurface7_Flip (dxdata.primary, NULL, flags);
		} else {
			ddrval = IDirectDrawSurface7_Flip (dxdata.primary, NULL, flags | DDFLIP_NOVSYNC);
		}
		DirectDraw_Blit (dxdata.flipping[0], dxdata.primary);
	}
	if (ddrval == DDERR_SURFACELOST) {
		static int recurse;
		restoresurface (dxdata.primary);
		if (!recurse) {
			recurse++;
			flip ();
			recurse--;
		}
	} else if (FAILED (ddrval)) {
		write_log (L"IDirectDrawSurface7_Flip: %s\n", DXError (ddrval));
	}
}

int DirectDraw_Flip (int doflip)
{
	if (dxdata.primary == NULL)
		return 0;
	if (getlocksurface () != dxdata.secondary) {
		if (doflip) {
			flip ();
			return 1;
		} else {
			DirectDraw_Blit (dxdata.primary, getlocksurface ());
		}
	} else {
		DirectDraw_BlitToPrimary2 (NULL, TRUE);
	}
	return 1;
}

void DirectDraw_Release (void)
{
	if (!dxdata.ddinit)
		return;
	dxdata.isoverlay = 0;
	dxdata.islost = 0;
	dxdata.ddinit = 0;
	freemainsurface ();
	if (dxdata.fsmodeset)
		IDirectDraw7_RestoreDisplayMode (dxdata.maindd);
	dxdata.fsmodeset = 0;
	IDirectDraw7_SetCooperativeLevel (dxdata.maindd, dxdata.hwnd, DDSCL_NORMAL);
	releaser (dxdata.dclip, IDirectDrawClipper_Release);
	releaser (dxdata.maindd, IDirectDraw7_Release);
	memset (&dxdata, 0, sizeof (dxdata));
}

struct dxcap {
	int num;
	TCHAR *name;
	DWORD mask;
};
static struct dxcap dxcapsinfo[] = 
{
	{ 1, L"DDCAPS_BLT", DDCAPS_BLT },
	{ 1, L"DDCAPS_BLTQUEUE", DDCAPS_BLTQUEUE },
	{ 1, L"DDCAPS_BLTFOURCC", DDCAPS_BLTFOURCC },
	{ 1, L"DDCAPS_BLTCOLORFILL", DDCAPS_BLTSTRETCH },
	{ 1, L"DDCAPS_BLTSTRETCH", DDCAPS_BLTSTRETCH },
	{ 1, L"DDCAPS_CANBLTSYSMEM", DDCAPS_CANBLTSYSMEM },
	{ 1, L"DDCAPS_CANCLIP", DDCAPS_CANCLIP },
	{ 1, L"DDCAPS_CANCLIPSTRETCHED", DDCAPS_CANCLIPSTRETCHED },
	{ 1, L"DDCAPS_COLORKEY", DDCAPS_COLORKEY },
	{ 1, L"DDCAPS_COLORKEYHWASSIST", DDCAPS_COLORKEYHWASSIST },
	{ 1, L"DDCAPS_GDI", DDCAPS_GDI },
	{ 1, L"DDCAPS_NOHARDWARE", DDCAPS_NOHARDWARE },
	{ 1, L"DDCAPS_OVERLAY", DDCAPS_OVERLAY },
	{ 1, L"DDCAPS_VBI", DDCAPS_VBI },
	{ 1, L"DDCAPS_3D", DDCAPS_3D },
	{ 1, L"DDCAPS_BANKSWITCHED", DDCAPS_BANKSWITCHED },
	{ 1, L"DDCAPS_PALETTE", DDCAPS_PALETTE },
	{ 1, L"DDCAPS_PALETTEVSYNC", DDCAPS_PALETTEVSYNC },
	{ 1, L"DDCAPS_READSCANLINE", DDCAPS_READSCANLINE },
	{ 2, L"DDCAPS2_CERTIFIED", DDCAPS2_CERTIFIED },
	{ 2, L"DDCAPS2_CANRENDERWINDOWED", DDCAPS2_CANRENDERWINDOWED },
	{ 2, L"DDCAPS2_NOPAGELOCKREQUIRED", DDCAPS2_NOPAGELOCKREQUIRED },
	{ 2, L"DDCAPS2_FLIPNOVSYNC", DDCAPS2_FLIPNOVSYNC },
	{ 2, L"DDCAPS2_FLIPINTERVAL", DDCAPS2_FLIPINTERVAL },
	{ 2, L"DDCAPS2_NO2DDURING3DSCENE", DDCAPS2_NO2DDURING3DSCENE },
	{ 2, L"DDCAPS2_NONLOCALVIDMEM", DDCAPS2_NONLOCALVIDMEM },
	{ 2, L"DDCAPS2_NONLOCALVIDMEMCAPS", DDCAPS2_NONLOCALVIDMEMCAPS },
	{ 2, L"DDCAPS2_WIDESURFACES", DDCAPS2_WIDESURFACES },
	{ 3, L"DDCKEYCAPS_DESTBLT", DDCKEYCAPS_DESTBLT },
	{ 3, L"DDCKEYCAPS_DESTBLTCLRSPACE", DDCKEYCAPS_DESTBLTCLRSPACE },
	{ 3, L"DDCKEYCAPS_SRCBLT", DDCKEYCAPS_SRCBLT },
	{ 3, L"DDCKEYCAPS_SRCBLTCLRSPACE", DDCKEYCAPS_SRCBLTCLRSPACE },
	{ 0, NULL }
};

static void showcaps (DDCAPS_DX7 *dc)
{
	int i, out;
	write_log (L"%08x %08x %08x %08x %08x %08x\n",
		dc->dwCaps, dc->dwCaps2, dc->dwCKeyCaps, dc->dwFXCaps, dc->dwFXAlphaCaps, dc->dwPalCaps, dc->ddsCaps);
	out = 0;
	for (i = 0;  dxcapsinfo[i].name; i++) {
		DWORD caps = 0;
		switch (dxcapsinfo[i].num)
		{
		case 1:
			caps = dc->dwCaps;
			break;
		case 2:
			caps = dc->dwCaps2;
			break;
		case 3:
			caps = dc->dwCKeyCaps;
			break;
		}
		if (caps & dxcapsinfo[i].mask) {
			if (out > 0)
				write_log (L",");
			write_log (L"%s", dxcapsinfo[i].name);
			out++;
		}
	}
	if (out > 0)
		write_log (L"\n");
	if ((dc->dwCaps & DDCAPS_COLORKEY) && (dc->dwCKeyCaps & DDCKEYCAPS_SRCBLT))
		dxcaps.cancolorkey = TRUE;
	if (dc->dwCaps2 & DDCAPS2_NONLOCALVIDMEM)
		dxcaps.cannonlocalvidmem = TRUE;
}


static void getcaps (void)
{
	HRESULT hr;
	DDCAPS_DX7 dc, hc;

	memset (&dc, 0, sizeof dc);
	memset (&hc, 0, sizeof hc);
	dc.dwSize = sizeof dc;
	hc.dwSize = sizeof hc;
	hr = IDirectDraw7_GetCaps (dxdata.maindd, &dc, &hc);
	if (FAILED (hr)) {
		write_log (L"IDirectDraw7_GetCaps() failed %s\n", DXError (hr));
		return;
	}
	write_log (L"DriverCaps: ");
	showcaps (&dc);
	write_log (L"HELCaps   : ");
	showcaps (&hc);
}

int DirectDraw_Start (GUID *guid)
{
	static int d3ddone;
	static int first;
	HRESULT ddrval;
	LPDIRECT3D9 d3d;
	D3DCAPS9 d3dCaps;
	HINSTANCE d3dDLL;

	dxdata.islost = 0;
	if (dxdata.ddinit) {
		if (guid == NULL && dxdata.ddzeroguid)
			return -1;
		if (guid && !memcmp (guid, &dxdata.ddguid, sizeof (GUID)))
			return -1;
		DirectDraw_Release ();
	}
#if 0
	LPDIRECTDRAW dd;
	ddrval = DirectDrawCreate (guid, &dd, NULL);
	if (FAILED (ddrval)) {
		write_log (L"DirectDrawCreate() failed, %s\n", DXError (ddrval));
		if (guid != NULL)
			return 0;
		goto oops;
	}
	ddrval = IDirectDraw_QueryInterface (dd, &IID_IDirectDraw7, &dxdata.maindd);
	IDirectDraw_Release (dd);
	if (FAILED (ddrval)) {
		write_log (L"IDirectDraw_QueryInterface() failed, %s\n", DXError (ddrval));
		goto oops;
	}
#else
	ddrval = DirectDrawCreateEx (guid, (void**)&dxdata.maindd, IID_IDirectDraw7, NULL);
	if (FAILED (ddrval)) {
		write_log (L"DirectDrawCreateEx() failed, %s\n", DXError (ddrval));
		if (guid != NULL)
			return 0;
		goto oops;
	}
#endif

	dxdata.statuswidth = 800;
	dxdata.statusheight = TD_TOTAL_HEIGHT;
	if (!d3ddone) {
		d3dDLL = LoadLibrary (L"D3D9.DLL");
		if (d3dDLL) {
			d3d = Direct3DCreate9 (D3D9b_SDK_VERSION);
			if (d3d) {
				if (SUCCEEDED (IDirect3D9_GetDeviceCaps (d3d, 0, D3DDEVTYPE_HAL, &d3dCaps))) {
					dxcaps.maxwidth = d3dCaps.MaxTextureWidth;
					dxcaps.maxheight = d3dCaps.MaxTextureHeight;
					write_log (L"Max hardware surface size: %dx%d\n", dxcaps.maxwidth, dxcaps.maxheight);
				}
				IDirect3D9_Release (d3d);
			}
			FreeLibrary (d3dDLL);
		}
		d3ddone = 1;
	}
	if (dxcaps.maxwidth < 2048)
		dxcaps.maxwidth = 2048;
	if (dxcaps.maxheight < 2048)
		dxcaps.maxheight = 2048;

	if (!first) {
		first = 1;
		getcaps ();
	}

	if (SUCCEEDED (DirectDraw_GetDisplayMode ())) {
		dxdata.ddinit = 1;
		dxdata.ddzeroguid = 1;
		if (guid) {
			dxdata.ddzeroguid = 0;
			memcpy (&dxdata.ddguid, guid, sizeof (GUID));
		}
		return 1;
	}
oops:
	write_log (L"DirectDraw_Start: %s\n", DXError (ddrval));
	DirectDraw_Release ();
	return 0;
}

int dx_islost (void)
{
	return dxdata.islost;
}

void dx_check (void)
{
	dxdata.islost = 0;
	if (dxdata.fsmodeset <= 0 || dxdata.primary == NULL)
		return;
	if (IDirectDrawSurface7_IsLost (dxdata.primary) != DDERR_SURFACELOST)
		return;
	if (IDirectDrawSurface7_Restore (dxdata.primary) != DDERR_WRONGMODE)
		return;
	dxdata.islost = 1;
}

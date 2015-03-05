/************************************************************************ 
 *
 * Stubs for missing (not yet implemeted) functions
 *
 * Copyright 2011 Oliver Brunner - aros<at>oliver-brunner.de
 *
 * This file is part of Janus-UAE.
 *
 * Janus-UAE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Janus-UAE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Janus-UAE. If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 *
 ************************************************************************/

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "fsdb.h"
#include "traps.h"
#include "zfile.h"
#include "archivers/zip/unzip.h"
#include "archivers/dms/pfile.h"
#include "archivers/wrp/warp.h"

#include "7z/xz.h"
#include "7z/lzmadec.h"
#include "7z/7zCrc.h"


#include "od-aros/aros_uaenet.h"
#include "od-aros/threaddep/thread.h"
#include "od-aros/stubs.h"

#define STUBD(x)

volatile int bsd_int_requested;

#warning arosrom/arosrom_len never defined in WinUAE !?
unsigned int arosrom_len;
unsigned char arosrom[10];
#if 0
int debuggable(void) 
{
  TODO();
}
#endif

int init_sound (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if 0
void screenshot (int mode) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void screenshot (int mode, int foo) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif


void sound_mute (int newmute)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void write_dlog (const TCHAR *format, ...)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void close_sound (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int console_get (TCHAR *out, int maxlen)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void console_out (const TCHAR *txt)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int fsdb_exists (TCHAR *nname)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void getfilepart (TCHAR *out, int size, const TCHAR *path)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void getpathpart (TCHAR *outpath, int size, const TCHAR *inpath)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void pause_sound (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void reset_sound (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

/* disable sound for now */
int setup_sound (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
	return 0;
}

void target_quit (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  STUBD(bug("should call shellexecute (currprefs.win32_commandpathend: %s)\n", currprefs.win32_commandpathend));
}

int uaenet_open (void *vsd, struct netdriverdata *tc, void *user, uaenet_gotfunc *gotfunc, uaenet_getfunc *getfunc, int promiscuous)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int uaeser_open (void *vsd, void *user, int unit)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int uaeser_read (void *vsd, uae_u8 *data, uae_u32 len)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void fpux_restore (int *v)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void getgfxoffset (int *dxp, int *dyp, int *mxp, int *myp) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void machdep_free (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

uae_u8 *restore_dmac (uae_u8 *src)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void resume_sound (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void sampler_free (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int sampler_init (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void sound_volume (int dir) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void target_reset (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void uaenet_close (void *vsd)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int uaeser_break (void *vsd, int brklen)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void uaeser_close (void *vsd)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int uaeser_query (void *vsd, uae_u16 *status, uae_u32 *pending)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int uaeser_write (void *vsd, uae_u8 *data, uae_u32 len)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void update_sound (int freq, int longframe, int linetoggle)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void close_console (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void console_flush (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void console_out_f (const TCHAR *format,...)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void sampler_vsync (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  //TODO();
}

void fetch_datapath (TCHAR *out, int size)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

uae_u32 get_byte_ce030 (uaecptr addr)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

uae_u32 get_long_ce030_prefetch (int o)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if 0
void graphics_leave(void) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

#include <exec/execbase.h>
int handle_msgpump (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));

  if(SysBase->TDNestCnt>=0) {
    bug("ERROR: Should not be in Forbid here!!\n");
    while(SysBase->TDNestCnt>=0) {
      bug("ERROR:   => call Permit.\n");
      Permit();
    }
  }
  //TODO();
	return 0;
}

#if (0)
int hdf_dup_target (struct hardfiledata *dhfd, const struct hardfiledata *shfd)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

void put_byte_ce030 (uaecptr addr, uae_u32 v)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void put_long_ce030 (uaecptr addr, uae_u32 v)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void put_word_ce030 (uaecptr addr, uae_u32 v)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

TCHAR *setconsolemode (TCHAR *buffer, int maxlen)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void setmouseactive (int active) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void uaenet_trigger (void *vsd)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void uaeser_trigger (void *vsd)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void debugger_change (int mode)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int get_guid_target (uae_u8 *out) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if (0)
int hdf_open_target (struct hardfiledata *hfd, const TCHAR *pname)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int hdf_read_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

static void picasso96_alloc2 (TrapContext *ctx)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}


USHORT DMS_Process_File(struct zfile *fi, struct zfile *fo, USHORT cmd, USHORT opt, USHORT PCRC, USHORT pwd, int part, struct zfile **extra)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));    
  TODO();
}

void fetch_ripperpath (TCHAR *out, int size)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if (0)
void hdf_close_target (struct hardfiledata *hfd)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int hdf_write_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

uae_u32 next_ilong_030ce (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

uae_u32 next_iword_030ce (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void serial_check_irq (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  //TODO();
}

void serial_uartbreak (int v)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void setmouseactivexy (int x, int y, int dir)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void setup_brkhandler(void) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if 0
void toggle_mousegrab (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

struct netdriverdata *uaenet_enumerate (struct netdriverdata **out, const TCHAR *name)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int uaeser_setparams (void *vsd, int baud, int rbuffer, int bits, int sbits, int rtscts, int parity, uae_u32 xonxoff)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

bool vsync_switchmode (int hz, int oldhz)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if (0)
int hdf_resize_target (struct hardfiledata *hfd, uae_u64 newsize)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

void notify_user_parms (int msg, const TCHAR *parms, ...)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if 0
void toggle_fullscreen (int mode)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

void update_debug_info(void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));    
  TODO();
}

struct zfile *archive_access_lzx (struct znode *zn)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

/* adds name to "recently used" windows menu .. */
void target_addtorecent (const TCHAR *name, int t)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  //TODO();
}

void amiga_clipboard_die (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void fetch_statefilepath (TCHAR *out, int size)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void ahi_finish_sound_buffer (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void master_sound_volume (int dir)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

/*
frame_time_t read_processor_time (void) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
*/

static unsigned int serial_hsynchandler_count;
/* seems to be necessary, calls ... => hsyncstuff ! */
void serial_hsynchandler (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  if((serial_hsynchandler_count % 2000) == 0) {
    STUBD(bug("serial_hsynchandler was called %d times\n", serial_hsynchandler_count));
    //TODO();
  }

  serial_hsynchandler_count++;
}

void uaeser_clearbuffers (void *vsd)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void amiga_clipboard_init (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

struct zvolume *archive_directory_7z (struct zfile *z)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void restart_sound_buffer (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if 0
void restore_a2065_finish (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int target_checkcapslock (int scancode, int *state)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

int uaenet_getdatalenght (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int uaeser_getdatalenght (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}



#if 0
void gfx_set_picasso_state (int on)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

void uaenet_enumerate_free (struct netdriverdata *tcp)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));    
  TODO();
}

int target_get_volume_name (struct uaedev_mount_info *mtinf, const TCHAR *volumepath, TCHAR *volumename, int size, bool inserted, bool fullcheck)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if 0
int check_prefs_changed_gfx (void) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

void amiga_clipboard_got_data (uaecptr data, uae_u32 size, uae_u32 actual)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void bsdsock_fake_int_handler(void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}


uae_u32 emulib_target_getcpurate (uae_u32 v, uae_u32 *low)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

TCHAR *fsdb_create_unique_nname (a_inode *base, const TCHAR *suggestion)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}


int amiga_clipboard_want_data (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int custom_fsdb_used_as_nname (a_inode *base, const TCHAR *nname)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));    
  TODO();
}

int fsdb_mode_representable_p (const a_inode *aino) 
//int fsdb_mode_representable_p (const a_inode *aino, int amigaos_mode)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

uae_u32 amiga_clipboard_proc_start (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void amiga_clipboard_task_start (uaecptr data)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int input_get_default_joystick (struct uae_input_device *uid, int i, int port, int af, int mode) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int input_get_default_lightpen (struct uae_input_device *uid, int i, int port, int af)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

a_inode *custom_fsdb_lookup_aino_aname (a_inode *base, const TCHAR *aname)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

a_inode *custom_fsdb_lookup_aino_nname (a_inode *base, const TCHAR *nname)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int input_get_default_joystick_analog (struct uae_input_device *uid, int i, int port, int af)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void f_out (void *f, const TCHAR *format, ...)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void SERDAT (uae_u16 w)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void SERPER (uae_u16 w)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

struct zfile *unwarp(struct zfile *zf)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

uae_u16 SERDATR (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

TCHAR* buf_out (TCHAR *buffer, int *bufsize, const TCHAR *format, ...)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));    
  TODO();
}

uae_u8 *save_log (int bootlog, int *len)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void to_lower (TCHAR *s, int len)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

static unsigned int ahi_hsync_count;
void ahi_hsync (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));

  if((ahi_hsync_count % 2000) == 0) {
    STUBD(bug("ahi_hsync was called %d times\n", ahi_hsync_count));
    //TODO();
  }

  ahi_hsync_count++;
}

int is_tablet (void) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void jit_abort (const TCHAR *format,...)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

uae_u8 *save_dmac (int *len, uae_u8 *dstptr)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void unlockscr (void) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  //TODO();
}


void CrcGenerateTable(void) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
	TODO();
}

SRes XzUnpacker_Code(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, /* int srcWasFinished, */ int finishMode,
		    ECoderStatus *status) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
	TODO();
}

SRes XzUnpacker_Create(CXzUnpacker *p, ISzAlloc *alloc) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
	TODO();
}
void XzUnpacker_Free(CXzUnpacker *p) 
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
	TODO();
}

char *ua (const TCHAR *s) {
    STUBD(bug("[JUAE:Stub] %s('%s')\n", __PRETTY_FUNCTION__, s));
	  return strdup(s);
}

TCHAR *au (const char *s) {
    STUBD(bug("[JUAE:Stub] %s('%s')\n", __PRETTY_FUNCTION__, s));
	  return strdup(s);
}

void refreshtitle (void)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
	  TODO();
}

void fetch_inputfilepath (TCHAR *out, int size)
{
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
	  TODO();
}

void alloc_colors256 (int (*)(int, int, int, unsigned int*)) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

#if 0
void restore_fram(int a, unsigned intb) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void restore_zram(int a, unsigned int b, int c) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

#if 0
void save_expansion(int* a, unsigned char* b) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

#if 0
void restore_expansion(unsigned char* a) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void save_fram(int* a)       {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void save_zram(int* a, int b)  {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

void clipboard_vsync (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  //TODO();
}

void pause_sound_buffer (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

/******/
int screen_was_picasso;

void update_sound(double foo) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  /* nowhere in WinUAE ..?? */

  TODO();
}

void unlockscr (struct vidbuffer *vb) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  //TODO();
}

int lockscr (struct vidbuffer *vb, bool fullupdate) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  //TODO();
  return 1;
}

void init_scsi (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

bool my_chmod (const TCHAR *name, uae_u32 mode) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return TRUE;
}

int my_issamevolume(const TCHAR *path1, const TCHAR *path2, TCHAR *path) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return FALSE;
}

int input_get_default_joystick_analog (struct uae_input_device *uid, int i, int port, int af, bool gp, bool joymouseswap) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 0;
}

int input_get_default_lightpen (struct uae_input_device *uid, int i, int port, int af, bool gp, bool joymouseswap) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 0;
}

int input_get_default_joystick (struct uae_input_device *uid, int i, int port, int af, int mode, bool gp, bool joymouseswap) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 0;
}

#if 0
int input_get_default_mouse (struct uae_input_device *uid, int i, int port, int af, bool gp, bool wheel, bool joymouseswap) {
  TODO();
  return 0;
}
#endif

void ethernet_enumerate_free (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int ethernet_getdatalenght (struct netdriverdata *ndd) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 0;
}

bool ethernet_enumerate (struct netdriverdata **nddp, const TCHAR *name) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return FALSE;
}

int ethernet_open (struct netdriverdata *ndd, void *vsd, void *user, ethernet_gotfunc *gotfunc, ethernet_getfunc *getfunc, int promiscuous) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 0;
}

void ethernet_close (struct netdriverdata *ndd, void *vsd) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void ethernet_trigger (void *vsd) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

void vsync_busywait_start (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

TCHAR console_getch (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 'X';
}

uae_u32 getlocaltime (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 1;
}

void getgfxoffset (float *dxp, float *dyp, float *mxp, float *myp) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

bool console_isch (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return FALSE;
}

#if (0)
void ncr_io_bput (uaecptr addr, uae_u32 val) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}
#endif

void uae_end_thread (uae_thread_id *tid) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
}

int enumerate_sound_devices (void) {
    STUBD(bug("[JUAE:Stub] %s()\n", __PRETTY_FUNCTION__));
  TODO();
  return 0;
}

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

//#define OLI_DEBUG
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

#if 0
int debuggable(void) 
{
  TODO();
}
#endif

int init_sound (void)
{
  TODO();
}

my_opendir_s *my_opendir (const TCHAR *name, const TCHAR *mask) 
{
  TODO();
}

my_opendir_s *my_opendir (const TCHAR *name) 
{
  TODO();
}



int my_readdir (struct my_opendir_s *mod, TCHAR *name)
{
  TODO();
}

int same_aname (const TCHAR *an1, const TCHAR *an2)
{
  TODO();
}

#if 0
void screenshot (int mode) 
{
  TODO();
}

void screenshot (int mode, int foo) 
{
  TODO();
}
#endif


void sound_mute (int newmute)
{
  TODO();
}

void write_dlog (const TCHAR *format, ...)
{
  TODO();
}

void close_sound (void)
{
  TODO();
}

int console_get (TCHAR *out, int maxlen)
{
  TODO();
}

void console_out (const TCHAR *txt)
{
  TODO();
}

int fsdb_exists (TCHAR *nname)
{
  TODO();
}

void getfilepart (TCHAR *out, int size, const TCHAR *path)
{
  TODO();
}

void getpathpart (TCHAR *outpath, int size, const TCHAR *inpath)
{
  TODO();
}

void my_closedir (struct my_opendir_s *mod)
{
  TODO();
}

int my_truncate (const TCHAR *name, uae_u64 len)
{
  TODO();
}

void pause_sound (void)
{
  TODO();
}

void reset_sound (void)
{
  TODO();
}

/* disable sound for now */
int setup_sound (void)
{
	return 0;
}

void target_quit (void)
{
  TODO();
}

int uaenet_open (void *vsd, struct netdriverdata *tc, void *user, uaenet_gotfunc *gotfunc, uaenet_getfunc *getfunc, int promiscuous)
{
  TODO();
}

int uaeser_open (void *vsd, void *user, int unit)
{
  TODO();
}

int uaeser_read (void *vsd, uae_u8 *data, uae_u32 len)
{
  TODO();
}

void fpux_restore (int *v)
{
  TODO();
}

void getgfxoffset (int *dxp, int *dyp, int *mxp, int *myp) 
{
  TODO();
}

void machdep_free (void)
{
  TODO();
}

int my_existsdir (const TCHAR *name)
{
  TODO();
}

uae_u8 *restore_dmac (uae_u8 *src)
{
  TODO();
}

void resume_sound (void)
{
  TODO();
}

void sampler_free (void)
{
  TODO();
}

int sampler_init (void)
{
  TODO();
}

void sound_volume (int dir) 
{
  TODO();
}

void target_reset (void)
{
  TODO();
}

void uaenet_close (void *vsd)
{
  TODO();
}

int uaeser_break (void *vsd, int brklen)
{
  TODO();
}

void uaeser_close (void *vsd)
{
  TODO();
}

int uaeser_query (void *vsd, uae_u16 *status, uae_u32 *pending)
{
  TODO();
}

int uaeser_write (void *vsd, uae_u8 *data, uae_u32 len)
{
  TODO();
}

void update_sound (int freq, int longframe, int linetoggle)
{
  TODO();
}

void close_console (void)
{
  TODO();
}

void console_flush (void)
{
  TODO();
}

void console_out_f (const TCHAR *format,...)
{
  TODO();
}

void sampler_vsync (void)
{
  TODO();
}

void fetch_datapath (TCHAR *out, int size)
{
  TODO();
}

uae_u32 get_byte_ce030 (uaecptr addr)
{
  TODO();
}

uae_u32 get_long_ce030_prefetch (int o)
{
  TODO();
}

#if 0
void graphics_leave(void) 
{
  TODO();
}
#endif

int handle_msgpump (void)
{
  TODO();
	return 0;
}

int hdf_dup_target (struct hardfiledata *dhfd, const struct hardfiledata *shfd)
{
  TODO();
}

void put_byte_ce030 (uaecptr addr, uae_u32 v)
{
  TODO();
}

void put_long_ce030 (uaecptr addr, uae_u32 v)
{
  TODO();
}

void put_word_ce030 (uaecptr addr, uae_u32 v)
{
  TODO();
}

TCHAR *setconsolemode (TCHAR *buffer, int maxlen)
{
  TODO();
}

void setmouseactive (int active) 
{
  TODO();
}

void uaenet_trigger (void *vsd)
{
  TODO();
}

void uaeser_trigger (void *vsd)
{
  TODO();
}

void consolehook_ret (uaecptr condev, uaecptr oldbeginio)
{
  TODO();
}

void debugger_change (int mode)
{
  TODO();
}

int get_guid_target (uae_u8 *out) 
{
  TODO();
}

int hdf_open_target (struct hardfiledata *hfd, const TCHAR *pname)
{
  TODO();
}

int hdf_read_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
  TODO();
}

bool my_isfilehidden (const TCHAR *path)
{
  TODO();
}

static void picasso96_alloc2 (TrapContext *ctx)
{
  TODO();
}


USHORT DMS_Process_File(struct zfile *fi, struct zfile *fo, USHORT cmd, USHORT opt, USHORT PCRC, USHORT pwd, int part, struct zfile **extra)
{
  TODO();
}

struct zfile *archive_getzfile (struct znode *zn, unsigned int id, int flags)
{
  TODO();
}

void fetch_ripperpath (TCHAR *out, int size)
{
  TODO();
}

void hdf_close_target (struct hardfiledata *hfd)
{
  TODO();
}

int hdf_write_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
  TODO();
}

int my_getvolumeinfo (const TCHAR *root)
{
  TODO();
}

void my_setfilehidden (const TCHAR *path, bool hidden)
{
  TODO();
}

uae_u32 next_ilong_030ce (void)
{
  TODO();
}

uae_u32 next_iword_030ce (void)
{
  TODO();
}

void serial_check_irq (void)
{
  TODO();
}

void serial_uartbreak (int v)
{
  TODO();
}

void setmouseactivexy (int x, int y, int dir)
{
  TODO();
}

void setup_brkhandler(void) 
{
  TODO();
}

#if 0
void toggle_mousegrab (void)
{
  TODO();
}

int uae_start_thread (const TCHAR *name, void *(*f)(void *), void *arg, uae_thread_id *tid)
{
  TODO();
}
#endif

struct netdriverdata *uaenet_enumerate (struct netdriverdata **out, const TCHAR *name)
{
  TODO();
}

int uaeser_setparams (void *vsd, int baud, int rbuffer, int bits, int sbits, int rtscts, int parity, uae_u32 xonxoff)
{
  TODO();
}

bool vsync_switchmode (int hz, int oldhz)
{
  TODO();
}

int fsdb_name_invalid (const char *n) 
{
  TODO();
}

int hdf_resize_target (struct hardfiledata *hfd, uae_u64 newsize)
{
  TODO();
}

void notify_user_parms (int msg, const TCHAR *parms, ...)
{
  TODO();
}

#if 0
void toggle_fullscreen (int mode)
{
  TODO();
}
#endif

void update_debug_info(void)
{
  TODO();
}

void updatedisplayarea (void)
{
  TODO();
}

struct zfile *archive_access_lzx (struct znode *zn)
{
  TODO();
}

void consolehook_config (struct uae_prefs *p)
{
  TODO();
}

void target_addtorecent (const TCHAR *name, int t)
{
  TODO();
}

void amiga_clipboard_die (void)
{
  TODO();
}

void archive_access_scan (struct zfile *zf, zfile_callback zc, void *user, unsigned int id)
{
  TODO();
}

struct zfile *archive_unpackzfile (struct zfile *zf)
{
  TODO();
}

uaecptr consolehook_beginio (uaecptr request)
{
  TODO();
}

void fetch_saveimagepath (TCHAR *out, int size, int dir)
{
  TODO();
}

void fetch_statefilepath (TCHAR *out, int size)
{
  TODO();
}

void ahi_finish_sound_buffer (void)
{
  TODO();
}

int fsdb_mode_supported (const a_inode *aino)
{
  TODO();
}

int fsdb_set_file_attrs (a_inode *aino, int mask) 
{
  TODO();
}

int fsdb_set_file_attrs (a_inode *aino)
{
  TODO();
}

void master_sound_volume (int dir)
{
  TODO();
}

frame_time_t read_processor_time (void) 
{
  TODO();
}

/* seems to be necessary, calls ... => hsyncstuff ! */
void serial_hsynchandler (void)
{
  TODO();
}

void uaeser_clearbuffers (void *vsd)
{
  TODO();
}

void amiga_clipboard_init (void)
{
  TODO();
}

void archive_access_close (void *handle, unsigned int id)
{
  TODO();
}

struct zvolume *archive_directory_7z (struct zfile *z)
{
  TODO();
}

int consolehook_activate (void)
{
  TODO();
}

void restart_sound_buffer (void)
{
  TODO();
}

void restore_a2065_finish (void)
{
  TODO();
}

#if 0
int target_checkcapslock (int scancode, int *state)
{
  TODO();
}
#endif

int uaenet_getdatalenght (void)
{
  TODO();
}

int uaeser_getdatalenght (void)
{
  TODO();
}

struct zfile *archive_access_select (struct znode *parent, struct zfile *zf, unsigned int id, int dodefault, int *retcode, int index)
{
  TODO();
}

struct zvolume *archive_directory_adf (struct znode *parent, struct zfile *z)
{
  TODO();
}

struct zvolume *archive_directory_fat (struct zfile *z)
{
  TODO();
}

struct zvolume *archive_directory_lha (struct zfile *zf)
{
  TODO();
}

struct zvolume *archive_directory_lzx (struct zfile *in_file)
{
  TODO();
}

struct zvolume *archive_directory_rar (struct zfile *z)
{
  TODO();
}

struct zvolume *archive_directory_rdb (struct zfile *z)
{
  TODO();
}

struct zvolume *archive_directory_tar (struct zfile *z)
{
  TODO();
}

struct zvolume *archive_directory_zip (struct zfile *z)
{
  TODO();
}

#if 0
void gfx_set_picasso_state (int on)
{
  TODO();
}
#endif

#if 0
int uae_start_thread_fast (void *(*f)(void *), void *arg, uae_thread_id *tid)
{
  TODO();
}
#endif

void uaenet_enumerate_free (struct netdriverdata *tcp)
{
  TODO();
}

int target_get_volume_name (struct uaedev_mount_info *mtinf, const TCHAR *volumepath, TCHAR *volumename, int size, bool inserted, bool fullcheck)
{
  TODO();
}

struct zvolume *archive_directory_plain (struct zfile *z)
{
  TODO();
}

#if 0
int check_prefs_changed_gfx (void) 
{
  TODO();
}
#endif

void draw_status_line_single (uae_u8 *buf, int bpp, int y, int totalwidth, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha)
{
  TODO();
}

int input_get_default_mouse (struct uae_input_device *uid, int i, int port, int af)
{
  TODO();
}


void uae_set_thread_priority (uae_thread_id *tid, int pri)
{
  TODO();
}

void amiga_clipboard_got_data (uaecptr data, uae_u32 size, uae_u32 actual)
{
  TODO();
}

void bsdsock_fake_int_handler(void)
{
  TODO();
}


uae_u32 emulib_target_getcpurate (uae_u32 v, uae_u32 *low)
{
  TODO();
}

TCHAR *fsdb_create_unique_nname (a_inode *base, const TCHAR *suggestion)
{
  TODO();
}


int amiga_clipboard_want_data (void)
{
  TODO();
}

int custom_fsdb_used_as_nname (a_inode *base, const TCHAR *nname)
{
  TODO();
}

int fsdb_mode_representable_p (const a_inode *aino) 
//int fsdb_mode_representable_p (const a_inode *aino, int amigaos_mode)
{
  TODO();
}

uae_u32 amiga_clipboard_proc_start (void)
{
  TODO();
}

void amiga_clipboard_task_start (uaecptr data)
{
  TODO();
}

int input_get_default_joystick (struct uae_input_device *uid, int i, int port, int af, int mode) 
{
  TODO();
}

int input_get_default_keyboard (int i)
{
  TODO();
}

int input_get_default_lightpen (struct uae_input_device *uid, int i, int port, int af)
{
  TODO();
}

a_inode *custom_fsdb_lookup_aino_aname (a_inode *base, const TCHAR *aname)
{
  TODO();
}

a_inode *custom_fsdb_lookup_aino_nname (a_inode *base, const TCHAR *nname)
{
  TODO();
}

int input_get_default_joystick_analog (struct uae_input_device *uid, int i, int port, int af)
{
  TODO();
}

void f_out (void *f, const TCHAR *format, ...)
{
  TODO();
}

int isfat (uae_u8 *p)
{
  TODO();
}

char *ua_fs (const WCHAR *s, int defchar) 
{
  TODO();
}

void SERDAT (uae_u16 w)
{
  TODO();
}

void SERPER (uae_u16 w)
{
  TODO();
}

struct zfile *unwarp(struct zfile *zf)
{
  TODO();
}

uae_u16 SERDATR (void)
{
  TODO();
}

TCHAR* buf_out (TCHAR *buffer, int *bufsize, const TCHAR *format, ...)
{
  TODO();
}

unsigned int my_read (struct my_openfile_s *mos, void *b, unsigned int size) 
{
  TODO();
}

unsigned int my_write (struct my_openfile_s *mos, void *b, unsigned int size) 
{
  TODO();
}

struct my_openfile_s *my_open (const TCHAR *name, int flags) 
{
  TODO();
}


char *ua_copy (char *dst, int maxlen, const TCHAR *src)
{
  TODO();
}

void my_close (struct my_openfile_s *mos)
{
  TODO();
}

uae_s64 int my_lseek (struct my_openfile_s *mos, uae_s64 int offset, int whence)
{
  TODO();
}

int my_mkdir (const TCHAR *name)
{
  TODO();
}

int my_rmdir (const TCHAR *name)
{
  TODO();
}

uae_u8 *save_log (int bootlog, int *len)
{
  TODO();
}

void to_lower (TCHAR *s, int len)
{
  TODO();
}

void ahi_hsync (void)
{
  TODO();
}

int dos_errno (void)
{
  TODO();
}

int is_tablet (void) 
{
  TODO();
}

void jit_abort (const TCHAR *format,...)
{
  TODO();
}

int my_rename (const TCHAR *oldname, const TCHAR *newname)
{
  TODO();
}

int my_unlink (const TCHAR *name)
{
  TODO();
}

uae_u8 *save_dmac (int *len, uae_u8 *dstptr)
{
  TODO();
}

void unlockscr (void) 
{
  TODO();
}


void CrcGenerateTable(void) 
{
	TODO();
}

SRes XzUnpacker_Code(CXzUnpacker *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, /* int srcWasFinished, */ int finishMode,
		    ECoderStatus *status) 
{
	TODO();
}

SRes XzUnpacker_Create(CXzUnpacker *p, ISzAlloc *alloc) 
{
	TODO();
}
void XzUnpacker_Free(CXzUnpacker *p) 
{
	TODO();
}

int fsdb_fill_file_attrs (a_inode *base, a_inode *aino)
{
	TODO();
}

int fsdb_mode_representable_p (const a_inode *aino, int amigaos_mode)
{
	TODO();
}

char *ua (const TCHAR *s) {
	  return strdup(s);
}

TCHAR *au (const char *s) {
	  return strdup(s);
}

void refreshtitle (void)
{
	  TODO();
}

void fetch_inputfilepath (TCHAR *out, int size)
{
	  TODO();
}

uae_u8 veccode[256];


void my_kbd_handler (int keyboard, int scancode, int newstate) {
  TODO();
}

void alloc_colors256 (int (*)(int, int, int, unsigned int*)) {
  TODO();
}

void setid (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt) {
  TODO();
}

int sdlk2dik (int key) {
  TODO();
}

void setid_af (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, int af) {
  TODO();
}

#if 0
void restore_fram(int a, unsigned intb) {
  TODO();
}

void restore_zram(int a, unsigned int b, int c) {
  TODO();
}
#endif

#if 0
void save_expansion(int* a, unsigned char* b) {
  TODO();
}
#endif

#if 0
void restore_expansion(unsigned char* a) {
  TODO();
}

void save_fram(int* a)       {
  TODO();
}

void save_zram(int* a, int b)  {
  TODO();
}
#endif

void clipboard_vsync (void) {
  TODO();
}

void pause_sound_buffer (void) {
  TODO();
}

/******/
int screen_was_picasso;

void update_sound(double foo) {
  /* nowhere in WinUAE ..?? */

  TODO();
}



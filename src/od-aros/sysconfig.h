/************************************************************************ 
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
#ifndef __SYSCONFIG_H__
#define __SYSCONFIG_H__

#define SUPPORT_THREADS
#define MAX_DPATH 1000
#define MAX_PATH 256

#define GFXFILTER


#ifndef UAE_MINI

//#define USE_GL
#define DEBUGGER
#define FILESYS    /* filesys emulation */
#define UAE_FILESYS_THREADS
#define AUTOCONFIG /* autoconfig support, fast ram, harddrives etc.. */
// NB: (AROS) JIT is passed in from the makefile
#if defined(JIT)
#define USE_JIT_FPU
#endif
#define NATMEM_OFFSET natmem_offset /* j-uae has it not defined */
//#define CATWEASEL /* Catweasel MK2/3 support */
#define ECS_DENISE /* ECS DENISE new features */
#define AGA        /* AGA chipset emulation (ECS_DENISE must be enabled) */
#define CD32
#define CDTV
#define SCSIEMU
#define FPUEMU
#define FPU_UAE
#define MMUEMU /* Aranym 68040 MMU */
#define FULLMMU /* Aranym 68040 MMU */
#define CPUEMU_0 /* generic 680x0 emulation with direct memory access */
#define CPUEMU_11 /* 68000/68010 prefetch emulation */
#define CPUEMU_13 /* 68000/68010 cycle-exact cpu&blitter */
#define CPUEMU_20 /* 68020 prefetch */
#define CPUEMU_21 /* 68020 "cycle-exact" + blitter */
#define CPUEMU_22 /* 68030 prefetch */
#define CPUEMU_23 /* 68030 "cycle-exact" + blitter */
#define CPUEMU_24 /* 68060 "cycle-exact" + blitter */
#define CPUEMU_25 /* 68040 "cycle-exact" + blitter */
#define CPUEMU_31 /* Aranym 68040 MMU */
#define CPUEMU_32 /* Previous 68030 MMU */
#define CPUEMU_33 /* 68060 MMU */
#define CPUEMU_40 /* generic 680x0 with indirect memory access */
#define CPUEMU_50 /* generic 680x0 with indirect memory access */
#define PICASSO96 /* Picasso96 display card emulation */
#define UAEGFX_INTERNAL /* built-in libs:picasso96/uaegfx.card */
//#define WITH_SLIRP
#define ACTION_REPLAY /* Action Replay 1/2/3 support */
#define ARCADIA /* Arcadia arcade system */
#define AMAX /* A-Max ROM adapater emulation */
//#define RETROPLATFORM /* Cloanto RetroPlayer support */
#define SAVESTATE /* State file support */
#define A2091 /* A590/A2091 SCSI */
#define A2065 /* A2065 Ethernet card */
#define GFXBOARD /* Hardware graphics board */
#define NCR /* A4000T/A4091, 53C710/53C770 SCSI */
#define NCR9X /* 53C9X SCSI */
#define WITH_PCI
#define WITH_X86

#define PICASSO96_SUPPORTED

#else

/* #define SINGLEFILE */

#define CUSTOM_SIMPLE /* simplified custom chipset emulation */
#define CPUEMU_0
#define CPUEMU_68000_ONLY /* drop 68010+ commands from CPUEMU_0 */
#define ADDRESS_SPACE_24BIT
#ifndef UAE_NOGUI
#define OPENGL
#endif
#define CAPS
#define CPUEMU_13
#define CPUEMU_11

#endif

/* oli defines */
#define NO_A2091_SCSI
#define NO_WD_SCSI

#define UAE_RAND_MAX RAND_MAX
/* missing types, maybe move somewhere else? */
#define WPARAM IPTR
#define LPARAM IPTR
/* old */

/* Define to 1 if you have the `alarm' function. */
/* #undef HAVE_ALARM */

/* Define to 1 if you have the 'bswap_16' function. */
/* #undef HAVE_BSWAP_16 */

/* Define to 1 if you have the 'bswap_32' function. */
/* #undef HAVE_BSWAP_32 */

/* Define to 1 if you have the <byteswap.h> header file. */
/* #undef HAVE_BYTESWAP_H */

/* Define to 1 if you have the <caps/capsimage.h> header file. */
/* #undef HAVE_CAPS_CAPSIMAGE_H */

/* Define to 1 if you have the `cfmakeraw' function. */
/* #undef HAVE_CFMAKERAW */

/* Define to 1 if you have the <curses.h> header file. */
/* #undef HAVE_CURSES_H */

/* Define to 1 if you have the <cybergraphx/cybergraphics.h> header file. */
#define HAVE_CYBERGRAPHX_CYBERGRAPHICS_H 1

/* Define to 1 if you have the <devices/ahi.h> header file. */
#define HAVE_DEVICES_AHI_H 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* "Define to 1 if you have 'dlopen' function */
/* #undef HAVE_DLOPEN */

/* Define to 1 if you have the <dustat.h> header file. */
/* #undef HAVE_DUSTAT_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `gmtime_r' function. */
/* #undef HAVE_GMTIME_R */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if <inttypes.h> exists, doesn't clash with <sys/types.h>, and
   declares uintmax_t. */
/* #undef HAVE_INTTYPES_H_WITH_UINTMAX */

/* Define to 1 if you have the `isinf' function. */
//#define HAVE_ISINF 1

/* Define to 1 if you have the `isnan' function. */
#define HAVE_ISNAN 1

/* Define to 1 if you have the <libraries/cybergraphics.h> header file. */
#define HAVE_LIBRARIES_CYBERGRAPHICS_H 1

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the `localtime_r' function. */
/* #undef HAVE_LOCALTIME_R */

/* Define to 1 if you have the <machine/joystick.h> header file. */
/* #undef HAVE_MACHINE_JOYSTICK_H */

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkdir' function. */
#define HAVE_MKDIR 1

/* Define to 1 if you have the `nanosleep' function. */
/* #undef HAVE_NANOSLEEP */

/* Define to 1 if you have the <ncurses.h> header file. */
/* #undef HAVE_NCURSES_H */

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define if you have POSIX threads libraries and header files. */
/* #undef HAVE_PTHREAD */

/* Define to 1 if you have the `readdir_r' function. */
/* #undef HAVE_READDIR_R */

/* Define to 1 if you have the `rmdir' function. */
#define HAVE_RMDIR 1

/* Define to 1 if you have the `select' function. */
/* #undef HAVE_SELECT */

/* Define to 1 if you have the `setitimer' function. */
/* #undef HAVE_SETITIMER */

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Define to 1 if you have the `sleep' function. */
#define HAVE_SLEEP 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define if <stdint.h> exists, doesn't clash with <sys/types.h>, and declares
   uintmax_t. */
/* #undef HAVE_STDINT_H_WITH_UINTMAX */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strcmpi' function. */
/* #undef HAVE_STRCMPI */

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `stricmp' function. */
/* #undef HAVE_STRICMP */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if `st_blocks' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLOCKS 1

/* Define to 1 if your `struct stat' has `st_blocks'. Deprecated, use
   `HAVE_STRUCT_STAT_ST_BLOCKS' instead. */
#define HAVE_ST_BLOCKS 1

/* Define to 1 if you have the <sun/audioio.h> header file. */
/* #undef HAVE_SUN_AUDIOIO_H */

/* Define to 1 if you have the `sync' function. */
/* #undef HAVE_SYNC */

/* Define to 1 if you have the <sys/audioio.h> header file. */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/filio.h> header file. */
/* #undef HAVE_SYS_FILIO_H */

/* Define to 1 if you have the <sys/filsys.h> header file. */
/* #undef HAVE_SYS_FILSYS_H */

/* Define to 1 if you have the <sys/fs/s5param.h> header file. */
/* #undef HAVE_SYS_FS_S5PARAM_H */

/* Define to 1 if you have the <sys/fs_types.h> header file. */
#define HAVE_SYS_FS_TYPES_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/ipc.h> header file. */
/* #undef HAVE_SYS_IPC_H */

/* Define to 1 if you have the <sys/mman.h> header file. */
/* #undef HAVE_SYS_MMAN_H */

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/shm.h> header file. */
/* #undef HAVE_SYS_SHM_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/statfs.h> header file. */
/* #undef HAVE_SYS_STATFS_H */

/* Define to 1 if you have the <sys/statvfs.h> header file. */
/* #undef HAVE_SYS_STATVFS_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/termios.h> header file. */
/* #undef HAVE_SYS_TERMIOS_H */

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
/* #undef HAVE_SYS_VFS_H */

/* Define to 1 if you have the `tcgetattr' function. */
/* #undef HAVE_TCGETATTR */

/* Define to 1 if you have the `timegm' function. */
/* #undef HAVE_TIMEGM */

/* Define if you have the 'uintmax_t' type in <stdint.h> or <inttypes.h>. */
/* #undef HAVE_UINTMAX_T */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if you have the 'unsigned long long' type. */
/* #undef HAVE_UNSIGNED_LONG_LONG */

/* Define to 1 if you have the `usleep' function. */
/* #undef HAVE_USLEEP */

/* Define to 1 if you have the <utime.h> header file. */
#define HAVE_UTIME_H 1

/* Define to 1 if `utime(file, NULL)' sets file's timestamp to the present. */
/* #undef HAVE_UTIME_NULL */

/* Define to 1 if you have the <values.h> header file. */
#define HAVE_VALUES_H 1

/* Define to 1 if you have the `vfprintf' function. */
#define HAVE_VFPRINTF 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the `vsprintf' function. */
#define HAVE_VSPRINTF 1

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Define to 1 if your CPU profitably supports multiplication. */
#define MULTIPLICATION_PROFITABLE 1

/* Name of package */
#define PACKAGE "Janus-UAE2"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "Janus-UAE2"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Janus-UAE2 0.3"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "janus-uae"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.3"

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* The number of bytes in a __int64.  */
#define SIZEOF___INT64 8

/* The size of `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8

#ifndef UAE
#define UAE
#endif

/*
 * The size of `void *', as computed by sizeof.
 * NB: (AROS) __WORDSIZE needs to be passed in from
 * the makefile, so that both target and host know.
 */
#if (__WORDSIZE == 64)
#define SIZEOF_VOID_P 8
#else
#define SIZEOF_VOID_P 4
#endif

/* Define if the block counts reported by statfs may be truncated to 2GB and
   the correct values may be stored in the f_spare array. (SunOS 4.1.2, 4.1.3,
   and 4.1.3_U1 are reported to have this problem. SunOS 4.1.1 seems not to be
   affected.) */
/* #undef STATFS_TRUNCATES_BLOCK_COUNTS */

/* Define if there is no specific function for reading filesystems usage
   information and you have the <sys/filsys.h> header file. (SVR2) */
/* #undef STAT_READ_FILSYS */

/* Define if statfs takes 2 args and struct statfs has a field named f_bsize.
   (4.3BSD, SunOS 4, HP-UX, AIX PS/2) */
/* #undef STAT_STATFS2_BSIZE */

/* Define if statfs takes 2 args and struct statfs has a field named f_fsize.
   (4.4BSD, NetBSD) */
/* #undef STAT_STATFS2_FSIZE */

/* Define if statfs takes 2 args and the second argument has type struct
   fs_data. (Ultrix) */
/* #undef STAT_STATFS2_FS_DATA */

/* Define if statfs takes 3 args. (DEC Alpha running OSF/1) */
/* #undef STAT_STATFS3_OSF1 */

/* Define if statfs takes 4 args. (SVR3, Dynix, Irix, Dolphin) */
/* #undef STAT_STATFS4 */

/* Define if there is a function named statvfs. (SVR4) */
/* #undef STAT_STATVFS */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Version number of package */
#define VERSION "0.1"

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Define to 1 if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* # undef _ALL_SOURCE */
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Substitute for socklen_t */
/* #undef socklen_t */

/* Define to unsigned long or unsigned long long if <stdint.h> and
   <inttypes.h> don't define. */
/* #undef uintmax_t */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

#define FSDB_DIR_SEPARATOR      '/'
#define FSDB_DIR_SEPARATOR_S _T("/")

/* build68k.c needs it */
#ifdef __linux__
#include "od-aros/tchar.h"
#define strnicmp strncasecmp 
#endif


#ifdef __AROS__

#include <aros/macros.h>
#define UAE_MAKEID AROS_MAKE_ID

#define _cdecl
#define __cdecl

/* Exception is used both in AROS and in uae :(.. I don't like that! */
#undef Exception
#include <aros/debug.h>

#ifndef UAE_ABI_v0
/* do we need that or not? On ABIV 0 i386 this breaks the build because of c++ redifinitons.. */
#include "tchar.h"
#endif

#if defined(JUAE_DEBUG)
#define DebOut(...) do { bug("[%lx]: %s:%d %s(): ",FindTask(NULL),__FILE__,__LINE__,__func__);bug(__VA_ARGS__); } while(0)
#define TODO() bug("TODO ==> %s:%d: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)
#else
#define DebOut(...)
#define TODO(...)
#endif

#undef Exception
#define REGPARAM
#define REGPARAM3 
extern void REGPARAM3 Exception (int) REGPARAM;
#undef REGPARAM
#undef REGPARAM3 


#define _strdup strdup
#define _stricmp stricmp 
#define _strnicmp strnicmp
#ifndef _strtoui64
#define _strtoui64 strtoull
#endif
#define _tstol atol
#define _tstof atof
#define _tstoi atoi
#define _tfopen fopen
#define _fseeki64 fseek
#define _ftelli64 ftell

/* wide char unlink */
#define _wunlink unlink

/************** Windows data types ****************************/
/* DWORD - 32-bit unsigned integer. */
#define DWORD  uint32_t
#define UINT  uint32_t
#define USHORT uint16_t


typedef struct _RECT {
  long left;
  long top;
  long right;
  long bottom;
} RECT, *PRECT;

#ifdef __AROS__
/*  According to http://msdn.microsoft.com/de-de/library/htb3tdkc.aspx:
    Difference in seconds between coordinated universal time and local time. The default value is 28,800.  
    I somehow doubt, this works. */
#define _timezone 28.8
#endif


#define FILEFLAG_DIR     0x1
#define FILEFLAG_ARCHIVE 0x2
#define FILEFLAG_WRITE   0x4
#define FILEFLAG_READ    0x8
#define FILEFLAG_EXECUTE 0x10
#define FILEFLAG_SCRIPT  0x20
#define FILEFLAG_PURE    0x40


#endif

#endif /* __SYSCONFIG_H__*/

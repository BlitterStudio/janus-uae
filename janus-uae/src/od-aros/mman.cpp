// Implement mprotect() for Win32
// Copyright (C) 2000, Brian King
// GNU Public License

#ifdef WINDOWS
#define _WIN32_WINNT 0x0501
#include <Windows.h>
#endif

#include <float.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "mman_host.h"
#include "memory.h"
#include "options.h"
#include "autoconf.h"

#ifdef WINDOWS

#else

#ifdef __AROS__
#include "aros.h"
#define PVOID void *
#define PULONG_PTR ULONG *
#define PULONG ULONG
#define FSUAE
#define uae_shmat shmat
#define uae_shmdt shmdt
#define uae_shmctl shmctl
#define uae_shmget shmget
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#include <sys/mman.h>

#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_DECOMMIT 0x4000
#define MEM_RELEASE 0x8000
#define MEM_WRITE_WATCH 0x00200000

#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_NOACCESS 0x01
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define PAGE_WRITECOPY 0x08

typedef void * LPVOID;
typedef size_t SIZE_T;

int uae_shmdt (const void *shmaddr);
int uae_shmctl (int shmid, int cmd, struct shmid_ds *buf);

typedef struct {
    int dwPageSize;
} SYSTEM_INFO;

void GetSystemInfo(SYSTEM_INFO *si) {
    //si->dwPageSize = sysconf(_SC_PAGESIZE);
    si->dwPageSize = 1024;
}

void *VirtualAlloc(void *lpAddress, size_t dwSize, int flAllocationType,
        int flProtect)
{
    int prot = 0;
    void *memory = NULL;

    write_log("- VirtualAlloc %p (%zu) %08x %08x\n", lpAddress, dwSize,
            flAllocationType, flProtect);
    if (flAllocationType & MEM_RESERVE) {
        write_log("  MEM_RESERVE\n");
    }
    if (flAllocationType & MEM_COMMIT) {
        write_log("  MEM_COMMIT\n");
    }
    if (flAllocationType & PAGE_READWRITE) {
        write_log("  PAGE_READWRITE\n");
    }

    if (flAllocationType == MEM_COMMIT && lpAddress == NULL) {
        bug("[JUAE:MMAN] %s: (COMMIT) Allocating %d bytes.\n", __PRETTY_FUNCTION__, dwSize);
        memory = malloc(dwSize);
        if (memory == NULL) {
            write_log("memory allocation failed: errno %d\n", errno);
        }
        bug("[JUAE:MMAN] %s: Allocated %d bytes @ 0x%p\n", __PRETTY_FUNCTION__, dwSize, memory);

        return memory;
    }

    if (flAllocationType & MEM_RESERVE) {
        bug("[JUAE:MMAN] %s: (RESERVE) Allocating %d bytes.\n", __PRETTY_FUNCTION__, dwSize);
        memory = malloc(dwSize);
        if (memory == NULL) {
            write_log("memory allocation failed: errno %d\n", errno);
        }
        bug("[JUAE:MMAN] %s: Allocated %d bytes @ 0x%p\n", __PRETTY_FUNCTION__, dwSize, memory);
#if 0
        memory = mmap(lpAddress, dwSize, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        printf("mmap result: %p\n", memory);
        if (memory == (void *) -1) {
            printf("mmap failed errno %d\n", errno);
        }
#endif
    }
    else {
        memory = lpAddress;
    }

    if (flAllocationType & MEM_COMMIT) {
#if 0
        int lockresult = mlock(memory, dwSize);
        if (lockresult != 0) {
            printf("mlock failed errno %d\n", errno);
            perror("mlock failed");
        }
#endif
    }

    bug("[JUAE:MMAN] %s: returning 0x%p\n", __PRETTY_FUNCTION__, memory);

    return memory;
}

bool VirtualFree(void *lpAddress, size_t dwSize, int dwFreeType) {
#if 0
    int result = 0;
    if (dwFreeType == MEM_DECOMMIT) {
        result = munlock(lpAddress, dwSize);
        if (result != 0) {
            printf("munlock failed %d\n", errno);
        }
    }
    else if (dwFreeType == MEM_RELEASE) {
        result = munmap(lpAddress, dwSize);
        if (result != 0) {
            printf("mlock failed\n");
        }
    }
    return result == 0;
#endif
    return true;
}

static int GetLastError() {
    return errno;
}

#ifdef HAVE_POSIX_MEMALIGN
// FIXME: Set HAVE_POSIX_MEMALIGN when available in config.h,
// possibly via autoconf
#define valloc my_valloc

static void *my_valloc(size_t size) {
    size_t alignment = sysconf(_SC_PAGESIZE);
    void *memptr = NULL;
    if (posix_memalign(&memptr, alignment, size) == 0) {
        return memptr;
    }
    return NULL;
}

#endif

static int my_getpagesize (void) {
    //return sysconf(_SC_PAGESIZE);
    return 1024;
}

#define getpagesize my_getpagesize

#endif

uae_u32 max_z3fastmem;

#if defined(NATMEM_OFFSET)

#define VAMODE 1


/* JIT can access few bytes outside of memory block if it executes code at the very end of memory block */
#define BARRIER 32

#define MAXZ3MEM32 0x7F000000
#define MAXZ3MEM64 0xF0000000

static struct shmid_ds shmids[MAX_SHMID];
uae_u8 *natmem_offset, *natmem_offset_end;
static uae_u8 *p96mem_offset;
static int p96mem_size;
static SYSTEM_INFO si;
int maxmem;
uae_u32 natmem_size;

static uae_u8 *virtualallocwithlock (LPVOID addr, SIZE_T size, DWORD allocationtype, DWORD protect)
{
    uae_u8 *p = (uae_u8*)VirtualAlloc (addr, size, allocationtype, protect);
    return p;
}
static void virtualfreewithlock (LPVOID addr, SIZE_T size, DWORD freetype)
{
    VirtualFree(addr, size, freetype);
}

#endif /*NATMEM */

void cache_free (uae_u8 *cache)
{
#ifdef WINDOWS
    virtualfreewithlock (cache, 0, MEM_RELEASE);
#elif defined(__AROS__)
    free(cache);
#else
    // FIXME: Must add (address, size) to a list in cache_alloc, so the memory
    // can be correctly released here...
    write_log("TODO: free memory with munmap\n");
    //munmap(cache, size);
#endif
}

uae_u8 *cache_alloc (int size)
{
    write_log("cache_alloc size = %d\n", size);
#ifdef WINDOWS
    return virtualallocwithlock (NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
    size = size < getpagesize() ? getpagesize() : size;

#ifndef __AROS__
    void *cache = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANON, -1, 0);
#else
    void *cache = malloc(size);
#endif
    if (!cache) {
        write_log ("Cache_Alloc of %d failed. ERR=%d\n", size, errno);
    }
    return (uae_u8 *) cache;
#endif
}

#if defined(NATMEM_OFFSET)
#if 0
static void setworkingset(void)
{
    typedef BOOL (CALLBACK* SETPROCESSWORKINGSETSIZE)(HANDLE,SIZE_T,SIZE_T);
    SETPROCESSWORKINGSETSIZE pSetProcessWorkingSetSize;
    pSetProcessWorkingSetSize = (SETPROCESSWORKINGSETSIZE)GetProcAddress(GetModuleHandle("kernal32.dll", "GetProcessWorkingSetSize");
    if (!pSetProcessWorkingSetSize)
        return;
    pSetProcessWorkingSetSize(GetCurrentProcess (),
        );
}
#endif

static uae_u32 lowmem (void)
{
    uae_u32 change = 0;
    if (currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size >= 8 * 1024 * 1024) {
        if (currprefs.z3fastmem2_size) {
            change = currprefs.z3fastmem2_size;
            currprefs.z3fastmem2_size = 0;
        } else if (currprefs.z3chipmem_size) {
            if (currprefs.z3chipmem_size <= 16 * 1024 * 1024) {
                change = currprefs.z3chipmem_size;
                currprefs.z3chipmem_size = 0;
            } else {
                change = currprefs.z3chipmem_size / 2;
                currprefs.z3chipmem_size /= 2;
            }
        } else {
            change = currprefs.z3fastmem_size - currprefs.z3fastmem_size / 4;
            currprefs.z3fastmem2_size = changed_prefs.z3fastmem2_size = currprefs.z3fastmem_size / 4;
            currprefs.z3fastmem_size /= 2;
            changed_prefs.z3fastmem_size = currprefs.z3fastmem_size;
        }
    } else if (currprefs.rtgmem_type && currprefs.rtgmem_size >= 1 * 1024 * 1024) {
        change = currprefs.rtgmem_size - currprefs.rtgmem_size / 2;
        currprefs.rtgmem_size /= 2;
        changed_prefs.rtgmem_size = currprefs.rtgmem_size;
    }
    if (currprefs.z3fastmem2_size < 128 * 1024 * 1024)
        currprefs.z3fastmem2_size = changed_prefs.z3fastmem2_size = 0;
    return change;
}

#ifdef FSUAE
#else
int mman_GetWriteWatch (PVOID lpBaseAddress, SIZE_T dwRegionSize, PVOID *lpAddresses, PULONG_PTR lpdwCount, PULONG lpdwGranularity)
{
    return GetWriteWatch (0, lpBaseAddress, dwRegionSize, lpAddresses, lpdwCount, lpdwGranularity);
}
void mman_ResetWatch (PVOID lpBaseAddress, SIZE_T dwRegionSize)
{
    if (ResetWriteWatch (lpBaseAddress, dwRegionSize))
        write_log (_T("ResetWriteWatch() failed, %d\n"), GetLastError ());
}
#endif

static uae_u64 size64;
#ifdef WINDOWS
typedef BOOL (CALLBACK* GLOBALMEMORYSTATUSEX)(LPMEMORYSTATUSEX);
#endif

static void clear_shm (void)
{
    write_log("clear_shm\n");

    shm_start = NULL;
    for (int i = 0; i < MAX_SHMID; i++) {
        memset (&shmids[i], 0, sizeof (struct shmid_ds));
        shmids[i].key = -1;
    }
}

bool preinit_shm (void)
{
    write_log("preinit_shm\n");
    uae_u64 total64;
    uae_u64 totalphys64;
#ifdef WINDOWS
    MEMORYSTATUS memstats;
    GLOBALMEMORYSTATUSEX pGlobalMemoryStatusEx;
    MEMORYSTATUSEX memstatsex;
#endif
    uae_u32 max_allowed_mman;

    if (natmem_offset)
#ifdef WINDOWS
        VirtualFree (natmem_offset, 0, MEM_RELEASE);
#else
        free (natmem_offset);
#endif
    natmem_offset = NULL;
    if (p96mem_offset)
#ifdef WINDOWS
        VirtualFree (p96mem_offset, 0, MEM_RELEASE);
#else
        // Don't free p96mem_offset - it is freed as part of natmem_offset
        //free (p96mem_offset);
        ;
#endif
    p96mem_offset = NULL;

    GetSystemInfo (&si);
#if (SIZEOF_VOID_P == 8)
#if defined(WIN64) || defined(__AROS__)
        max_allowed_mman = 3072;
#else
        max_allowed_mman = 2048;
#endif
#else
    max_allowed_mman = 512 + 256;
#endif

#ifdef WINDOWS
    memstats.dwLength = sizeof(memstats);
    GlobalMemoryStatus(&memstats);
    totalphys64 = memstats.dwTotalPhys;
    total64 = (uae_u64)memstats.dwAvailPageFile + (uae_u64)memstats.dwTotalPhys;
#ifdef FSUAE
    pGlobalMemoryStatusEx = GlobalMemoryStatusEx;
#else
    pGlobalMemoryStatusEx = (GLOBALMEMORYSTATUSEX)GetProcAddress (GetModuleHandle (_T("kernel32.dll")), "GlobalMemoryStatusEx");
#endif
    if (pGlobalMemoryStatusEx) {
        memstatsex.dwLength = sizeof (MEMORYSTATUSEX);
        if (pGlobalMemoryStatusEx(&memstatsex)) {
            totalphys64 = memstatsex.ullTotalPhys;
            total64 = memstatsex.ullAvailPageFile + memstatsex.ullTotalPhys;
        }
    }
#elif defined(__APPLE__)
    int mib[2];
    size_t len;

    mib[0] = CTL_HW;
    // FIXME: check 64-bit compat
    mib[1] = HW_MEMSIZE; /* gives a 64 bit int */
    len = sizeof(totalphys64);
    sysctl(mib, 2, &totalphys64, &len, NULL, 0);
    total64 = (uae_u64) totalphys64;
#elif defined(__AROS__)
    totalphys64 = AvailMem(MEMF_ANY);
    total64 = (uae_u64) AvailMem(MEMF_ANY|MEMF_LARGEST);
#else
    totalphys64 = sysconf (_SC_PHYS_PAGES) * getpagesize();
    total64 = (uae_u64)sysconf (_SC_PHYS_PAGES) * (uae_u64)getpagesize();
#endif
    size64 = total64;
#if (SIZEOF_VOID_P == 8)
        if (size64 > MAXZ3MEM64)
            size64 = MAXZ3MEM64;
#else
        if (size64 > MAXZ3MEM32)
            size64 = MAXZ3MEM32;
#endif
    if (maxmem < 0)
        size64 = MAXZ3MEM64;
    else if (maxmem > 0)
        size64 = maxmem * 1024 * 1024;
    if (size64 < 8 * 1024 * 1024)
        size64 = 8 * 1024 * 1024;
    if (max_allowed_mman * 1024 * 1024 > size64)
        max_allowed_mman = size64 / (1024 * 1024);
#if (SIZEOF_VOID_P == 4)
        if (max_allowed_mman * 1024 * 1024 > (totalphys64 / 2))
            max_allowed_mman = (totalphys64 / 2) / (1024 * 1024);
#endif

    natmem_size = (max_allowed_mman + 1) * 1024 * 1024;
#ifndef __AROS__
    if (natmem_size < 256 * 1024 * 1024)
        natmem_size = 256 * 1024 * 1024;
    write_log (_T("Total physical RAM %lluM, all RAM %lluM. Attempting to reserve: %uM.\n"), totalphys64 >> 20, total64 >> 20, natmem_size >> 20);
#else
    uae_u32 natmem_size_max=total64 - 128 * 1024 *1024 ; /* leave 128M memory free, we'll need it */
    //write_log("natmem_size_max: %d %dM\n", natmem_size_max, natmem_size_max>>20);
    natmem_size=1024*1024;
    /* round up nicely */
    while(natmem_size < (natmem_size_max/2)) {
      natmem_size=natmem_size*2;
    }
    write_log (_T("Total free RAM %lluM, largest free RAM %lluM. Attempting to reserve: %uM.\n"), totalphys64 >> 20, total64 >> 20, natmem_size >> 20);
#endif

    natmem_offset = 0;
    if (natmem_size <= 768 * 1024 * 1024) {
        uae_u32 p = 0x78000000 - natmem_size;
        for (;;) {
            natmem_offset = (uae_u8*)VirtualAlloc ((void*)p, natmem_size, MEM_RESERVE | (VAMODE == 1 ? MEM_WRITE_WATCH : 0), PAGE_READWRITE);
            if (natmem_offset)
                break;
            p -= 128 * 1024 * 1024;
            if (p <= 128 * 1024 * 1024)
                break;
        }
    }
    if (!natmem_offset) {
        for (;;) {
            natmem_offset = (uae_u8*)VirtualAlloc (NULL, natmem_size, MEM_RESERVE | (VAMODE == 1 ? MEM_WRITE_WATCH : 0), PAGE_READWRITE);
            if (natmem_offset)
                break;
            natmem_size -= 128 * 1024 * 1024;
            if (!natmem_size) {
                write_log (_T("Can't allocate 256M of virtual address space!?\n"));
                return false;
            }
        }
    }
    max_z3fastmem = natmem_size;
    write_log (_T("Reserved: 0x%p-0x%p (%08x %dM)\n"),
        natmem_offset, (uae_u8*)natmem_offset + natmem_size,
        natmem_size, natmem_size >> 20);

    clear_shm ();

//	write_log (_T("Max Z3FastRAM %dM. Total physical RAM %uM\n"), max_z3fastmem >> 20, totalphys64 >> 20);

    canbang = 1;
    return true;
}

static void resetmem (bool decommit)
{
    int i;

    if (!shm_start)
        return;
    for (i = 0; i < MAX_SHMID; i++) {
        struct shmid_ds *s = &shmids[i];
        int size = s->size;
        uae_u8 *shmaddr;
        uae_u8 *result;

        if (!s->attached)
            continue;
        if (!s->natmembase)
            continue;
        if (s->fake)
            continue;
        if (!decommit && ((uae_u8*)s->attached - (uae_u8*)s->natmembase) >= 0x10000000)
            continue;
        shmaddr = natmem_offset + ((uae_u8*)s->attached - (uae_u8*)s->natmembase);
        if (decommit) {
            VirtualFree (shmaddr, size, MEM_DECOMMIT);
        } else {
            result = virtualallocwithlock (shmaddr, size, decommit ? MEM_DECOMMIT : MEM_COMMIT, PAGE_READWRITE);
            if (result != shmaddr)
                write_log (_T("NATMEM: realloc(%p-%p,%d,%d,%s) failed, err=%d\n"), shmaddr, shmaddr + size, size, s->mode, s->name, GetLastError ());
            else
                write_log (_T("NATMEM: rellocated(%p-%p,%d,%s)\n"), shmaddr, shmaddr + size, size, s->name);
        }
    }
}

static ULONG getz2rtgaddr (void)
{
    ULONG start;
    start = changed_prefs.fastmem_size;
    while (start & (changed_prefs.rtgmem_size - 1) && start < 4 * 1024 * 1024)
        start += 1024 * 1024;
    return start + 2 * 1024 * 1024;
}

#if 0
int init_shm (void)
{
	uae_u32 size, totalsize, z3size, natmemsize;
	uae_u32 rtgbarrier, z3chipbarrier, rtgextra;
	int rounds = 0;
	ULONG z3rtgmem_size = currprefs.rtgmem_type ? currprefs.rtgmem_size : 0;

restart:
	for (;;) {
		int lowround = 0;
		uae_u8 *blah = NULL;
		if (rounds > 0)
			write_log (_T("NATMEM: retrying %d..\n"), rounds);
		rounds++;
		if (natmem_offset)
			VirtualFree (natmem_offset, 0, MEM_RELEASE);
		natmem_offset = NULL;
		natmem_offset_end = NULL;
		canbang = 0;

		z3size = 0;
		size = 0x1000000;
		rtgextra = 0;
		z3chipbarrier = 0;
		rtgbarrier = si.dwPageSize;
		if (currprefs.cpu_model >= 68020)
			size = 0x10000000;
		if (currprefs.z3fastmem_size || currprefs.z3fastmem2_size || currprefs.z3chipmem_size) {
			z3size = currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size + (currprefs.z3fastmem_start - 0x10000000);
			if (z3rtgmem_size) {
				rtgbarrier = 16 * 1024 * 1024 - ((currprefs.z3fastmem_size + currprefs.z3fastmem2_size) & 0x00ffffff);
			}
			if (currprefs.z3chipmem_size && (currprefs.z3fastmem_size || currprefs.z3fastmem2_size))
				z3chipbarrier = 16 * 1024 * 1024;
		} else {
			rtgbarrier = 0;
		}
		totalsize = size + z3size + z3rtgmem_size;
		while (totalsize > size64) {
			int change = lowmem ();
			if (!change)
				return 0;
			write_log (_T("NATMEM: %d, %dM > %dM = %dM\n"), ++lowround, totalsize >> 20, size64 >> 20, (totalsize - change) >> 20);
			totalsize -= change;
		}
		if ((rounds > 1 && totalsize < 0x10000000) || rounds > 20) {
			write_log (_T("NATMEM: No special area could be allocated (3)!\n"));
			return 0;
		}
		natmemsize = size + z3size;

		if (z3rtgmem_size) {
			rtgextra = si.dwPageSize;
		} else {
			rtgbarrier = 0;
			rtgextra = 0;
		}
		natmem_size = natmemsize + rtgbarrier + z3chipbarrier + z3rtgmem_size + rtgextra + 16 * si.dwPageSize;
		blah = (uae_u8*)VirtualAlloc (NULL, natmem_size, MEM_RESERVE, PAGE_READWRITE);
		if (blah) {
			natmem_offset = blah;
			break;
		}
		natmem_size = 0;
		write_log (_T("NATMEM: %dM area failed to allocate, err=%d (Z3=%dM,RTG=%dM)\n"),
			natmemsize >> 20, GetLastError (), (currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size) >> 20, z3rtgmem_size >> 20);
		if (!lowmem ()) {
			write_log (_T("NATMEM: No special area could be allocated (2)!\n"));
			return 0;
		}
	}
	p96mem_size = z3rtgmem_size;
	if (currprefs.rtgmem_size && currprefs.rtgmem_type) {
		VirtualFree (natmem_offset, 0, MEM_RELEASE);
		if (!VirtualAlloc (natmem_offset, natmemsize + rtgbarrier + z3chipbarrier, MEM_RESERVE, PAGE_READWRITE)) {
			write_log (_T("VirtualAlloc() part 2 error %d. RTG disabled.\n"), GetLastError ());
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			rtgbarrier = si.dwPageSize;
			rtgextra = 0;
			goto restart;
		}
		p96mem_offset = (uae_u8*)VirtualAlloc (natmem_offset + natmemsize + rtgbarrier + z3chipbarrier, p96mem_size + rtgextra,
			MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
		if (!p96mem_offset) {
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			z3rtgmem_size = 0;
			write_log (_T("NATMEM: failed to allocate special Picasso96 GFX RAM, err=%d\n"), GetLastError ());
		}
	} else if (currprefs.rtgmem_size && !currprefs.rtgmem_type) {
		// This so annoying..
		VirtualFree (natmem_offset, 0, MEM_RELEASE);
		// Chip + Z2Fast
		if (!VirtualAlloc (natmem_offset, 2 * 1024 * 1024 + currprefs.fastmem_size, MEM_RESERVE, PAGE_READWRITE)) {
			write_log (_T("VirtualAlloc() part 2 error %d. RTG disabled.\n"), GetLastError ());
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			rtgbarrier = si.dwPageSize;
			rtgextra = 0;
			goto restart;
		}
		// After RTG
		if (!VirtualAlloc (natmem_offset + 2 * 1024 * 1024 + 8 * 1024 * 1024,
			natmemsize + rtgbarrier + z3chipbarrier - (2 * 1024 * 1024 + 8 * 1024 * 1024) + si.dwPageSize, MEM_RESERVE, PAGE_READWRITE)) {
			write_log (_T("VirtualAlloc() part 2 error %d. RTG disabled.\n"), GetLastError ());
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			rtgbarrier = si.dwPageSize;
			rtgextra = 0;
			goto restart;
		}
		// RTG
		p96mem_offset = (uae_u8*)VirtualAlloc (natmem_offset + getz2rtgaddr (), 10 * 1024 * 1024 - getz2rtgaddr (),
			MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
		if (!p96mem_offset) {
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			write_log (_T("NATMEM: failed to allocate special Picasso96 GFX RAM, err=%d\n"), GetLastError ());
		}
	}

	if (!natmem_offset) {
		write_log (_T("NATMEM: No special area could be allocated! (1) err=%d\n"), GetLastError ());
	} else {
		write_log (_T("NATMEM: Our special area: 0x%p-0x%p (%08x %dM)\n"),
			natmem_offset, (uae_u8*)natmem_offset + natmemsize,
			natmemsize, natmemsize >> 20);
		if (currprefs.rtgmem_size)
			write_log (_T("NATMEM: P96 special area: 0x%p-0x%p (%08x %dM)\n"),
			p96mem_offset, (uae_u8*)p96mem_offset + currprefs.rtgmem_size,
			currprefs.rtgmem_size, currprefs.rtgmem_size >> 20);
		canbang = 1;
		if (p96mem_size)
			natmem_offset_end = p96mem_offset + p96mem_size;
		else
			natmem_offset_end = natmem_offset + natmemsize;
	}

	resetmem (false);

	return canbang;
}
#endif

static uae_u8 *va (uae_u32 offset, uae_u32 len, DWORD alloc, DWORD protect)
{
    uae_u8 *addr;

    addr = (uae_u8*)VirtualAlloc (natmem_offset + offset, len, alloc, protect);
    if (addr) {
        write_log (_T("VA(%p - %p, %4uM, %s)\n"),
            natmem_offset + offset, natmem_offset + offset + len, len >> 20, (alloc & MEM_WRITE_WATCH) ? _T("WATCH") : _T("RESERVED"));
        return addr;
    }
    write_log (_T("VA(%p - %p, %4uM, %s) failed %d\n"),
        natmem_offset + offset, natmem_offset + offset + len, len >> 20, (alloc & MEM_WRITE_WATCH) ? _T("WATCH") : _T("RESERVED"), GetLastError ());
    return NULL;
}

static int doinit_shm (void)
{
    write_log("doinit_shm\n");
    uae_u32 size, totalsize, z3size, natmemsize;
    uae_u32 rtgbarrier, z3chipbarrier, rtgextra;
    int rounds = 0;
    ULONG z3rtgmem_size;

    for (;;) {
        int lowround = 0;
        uae_u8 *blah = NULL;
        if (rounds > 0)
            write_log (_T("NATMEM: retrying %d..\n"), rounds);
        rounds++;

        z3size = 0;
        size = 0x1000000;
        rtgextra = 0;
        z3chipbarrier = 0;
        rtgbarrier = si.dwPageSize;
        z3rtgmem_size = changed_prefs.rtgmem_type ? changed_prefs.rtgmem_size : 0;
        if (changed_prefs.cpu_model >= 68020)
            size = 0x10000000;
        if (changed_prefs.z3fastmem_size || changed_prefs.z3fastmem2_size || changed_prefs.z3chipmem_size) {
            z3size = changed_prefs.z3fastmem_size + changed_prefs.z3fastmem2_size + changed_prefs.z3chipmem_size + (changed_prefs.z3fastmem_start - 0x10000000);
            if (z3rtgmem_size) {
                rtgbarrier = 16 * 1024 * 1024 - ((changed_prefs.z3fastmem_size + changed_prefs.z3fastmem2_size) & 0x00ffffff);
            }
            if (changed_prefs.z3chipmem_size && (changed_prefs.z3fastmem_size || changed_prefs.z3fastmem2_size))
                z3chipbarrier = 16 * 1024 * 1024;
        } else {
            rtgbarrier = 0;
        }
        totalsize = size + z3size + z3rtgmem_size;
        while (totalsize > size64) {
            int change = lowmem ();
            if (!change)
                return 0;
            write_log (_T("NATMEM: %d, %dM > %dM = %dM\n"), ++lowround, totalsize >> 20, size64 >> 20, (totalsize - change) >> 20);
            totalsize -= change;
        }
        if ((rounds > 1 && totalsize < 0x10000000) || rounds > 20) {
            write_log (_T("NATMEM: No special area could be allocated (3)!\n"));
            return 0;
        }
        natmemsize = size + z3size;

        if (z3rtgmem_size) {
            rtgextra = si.dwPageSize;
        } else {
            rtgbarrier = 0;
            rtgextra = 0;
        }
        if (natmemsize + rtgbarrier + z3chipbarrier + z3rtgmem_size + rtgextra + 16 * si.dwPageSize <= natmem_size)
            break;
        write_log (_T("NATMEM: %dM area failed to allocate, err=%d (Z3=%dM,RTG=%dM)\n"),
            natmemsize >> 20, GetLastError (), (changed_prefs.z3fastmem_size + changed_prefs.z3fastmem2_size + changed_prefs.z3chipmem_size) >> 20, z3rtgmem_size >> 20);
        if (!lowmem ()) {
            write_log (_T("NATMEM: No special area could be allocated (2)!\n"));
            return 0;
        }
    }

#if VAMODE == 1

    p96mem_offset = NULL;
    p96mem_size = z3rtgmem_size;
    if (changed_prefs.rtgmem_size && changed_prefs.rtgmem_type) {
        p96mem_offset = natmem_offset + natmemsize + rtgbarrier + z3chipbarrier;
    } else if (changed_prefs.rtgmem_size && !changed_prefs.rtgmem_type) {
        p96mem_offset = natmem_offset + getz2rtgaddr ();
    }

#else

    if (p96mem_offset)
        VirtualFree (p96mem_offset, 0, MEM_RELEASE);
    p96mem_offset = NULL;
    p96mem_size = z3rtgmem_size;
    if (changed_prefs.rtgmem_size && changed_prefs.rtgmem_type) {
        uae_u32 s, l;
        VirtualFree (natmem_offset, 0, MEM_RELEASE);

        s = 0;
        l = natmemsize + rtgbarrier + z3chipbarrier;
        if (!va (s, l, MEM_RESERVE, PAGE_READWRITE))
            return 0;

        s = natmemsize + rtgbarrier + z3chipbarrier;
        l = p96mem_size + rtgextra;
        p96mem_offset = va (s, l, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
        if (!p96mem_offset) {
            currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
            z3rtgmem_size = 0;
            write_log (_T("NATMEM: failed to allocate special Picasso96 GFX RAM, err=%d\n"), GetLastError ());
        }

#if 0
        s = natmemsize + rtgbarrier + z3chipbarrier + p96mem_size + rtgextra + 4096;
        l = natmem_size - s - 4096;
        if (natmem_size > l) {
            if (!va (s, l, 	MEM_RESERVE, PAGE_READWRITE))
                return 0;
        }
#endif

    } else if (changed_prefs.rtgmem_size && !changed_prefs.rtgmem_type) {

        uae_u32 s, l;
        VirtualFree (natmem_offset, 0, MEM_RELEASE);
        // Chip + Z2Fast
        s = 0;
        l = 2 * 1024 * 1024 + changed_prefs.fastmem_size;
        if (!va (s, l, MEM_RESERVE, PAGE_READWRITE)) {
            currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
        }
        // After RTG
        s = 2 * 1024 * 1024 + 8 * 1024 * 1024;
        l = natmem_size - (2 * 1024 * 1024 + 8 * 1024 * 1024) + si.dwPageSize;
        if (!va (s, l, MEM_RESERVE, PAGE_READWRITE)) {
            currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
        }
        // RTG
        s = getz2rtgaddr ();
        l = 10 * 1024 * 1024 - getz2rtgaddr ();
        p96mem_offset = va (s, l, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
        if (!p96mem_offset) {
            currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
        }

    } else {

        VirtualFree (natmem_offset, 0, MEM_RELEASE);
        if (!VirtualAlloc (natmem_offset, natmem_size, MEM_RESERVE, PAGE_READWRITE)) {
            write_log (_T("NATMEM: No special area could be reallocated! (1) err=%d\n"), GetLastError ());
            return 0;
        }
    }
#endif
    if (!natmem_offset) {
        write_log (_T("NATMEM: No special area could be allocated! err=%d\n"), GetLastError ());
    } else {
        write_log (_T("NATMEM: Our special area: 0x%p-0x%p (%08x %dM)\n"),
            natmem_offset, (uae_u8*)natmem_offset + natmemsize,
            natmemsize, natmemsize >> 20);
        if (changed_prefs.rtgmem_size)
            write_log (_T("NATMEM: P96 special area: 0x%p-0x%p (%08x %dM)\n"),
            p96mem_offset, (uae_u8*)p96mem_offset + changed_prefs.rtgmem_size,
            changed_prefs.rtgmem_size, changed_prefs.rtgmem_size >> 20);
        canbang = 1;
        if (p96mem_size)
            natmem_offset_end = p96mem_offset + p96mem_size;
        else
            natmem_offset_end = natmem_offset + natmemsize;
    }

    return canbang;
}

bool init_shm (void)
{
    write_log("init_shm\n");
    static uae_u32 oz3fastmem_size, oz3fastmem2_size;
    static uae_u32 oz3chipmem_size;
    static uae_u32 ortgmem_size;
    static int ortgmem_type;

    if (
        oz3fastmem_size == changed_prefs.z3fastmem_size &&
        oz3fastmem2_size == changed_prefs.z3fastmem2_size &&
        oz3chipmem_size == changed_prefs.z3chipmem_size &&
        ortgmem_size == changed_prefs.rtgmem_size &&
        ortgmem_type == changed_prefs.rtgmem_type)
        return false;

    oz3fastmem_size = changed_prefs.z3fastmem_size;
    oz3fastmem2_size = changed_prefs.z3fastmem2_size;
    oz3chipmem_size = changed_prefs.z3chipmem_size;;
    ortgmem_size = changed_prefs.rtgmem_size;
    ortgmem_type = changed_prefs.rtgmem_type;

    doinit_shm ();

    resetmem (false);
    clear_shm ();

    memory_hardreset (2);
    return true;
}

void free_shm (void)
{
    resetmem (true);
    clear_shm ();
}

void mapped_free (uae_u8 *mem)
{
    shmpiece *x = shm_start;

    if (mem == filesysory) {
        while(x) {
            if (mem == x->native_address) {
                int shmid = x->id;
                shmids[shmid].key = -1;
                shmids[shmid].name[0] = '\0';
                shmids[shmid].size = 0;
                shmids[shmid].attached = 0;
                shmids[shmid].mode = 0;
                shmids[shmid].natmembase = 0;
            }
            x = x->next;
        }
        return;
    }

    while(x) {
        if(mem == x->native_address)
            uae_shmdt (x->native_address);
        x = x->next;
    }
    x = shm_start;
    while(x) {
        struct shmid_ds blah;
        if (mem == x->native_address) {
            if (uae_shmctl (x->id, IPC_STAT, &blah) == 0)
                uae_shmctl (x->id, IPC_RMID, &blah);
        }
        x = x->next;
    }
}

static uae_key_t get_next_shmkey (void)
{
    uae_key_t result = -1;
    int i;
    for (i = 0; i < MAX_SHMID; i++) {
        if (shmids[i].key == -1) {
            shmids[i].key = i;
            result = i;
            break;
        }
    }
    return result;
}

STATIC_INLINE uae_key_t find_shmkey (uae_key_t key)
{
    int result = -1;
    if(shmids[key].key == key) {
        result = key;
    }
    return result;
}

int mprotect (void *addr, size_t len, int prot)
#ifdef PANDORA    
    __THROW
#endif
{
    int result = 0;
    return result;
}

void *uae_shmat (int shmid, void *shmaddr, int shmflg)
{
    write_log("uae_shmat shmid %d shmaddr %p, shmflg %d natmem_offset = %p\n",
            shmid, shmaddr, shmflg, natmem_offset);
    void *result = (void *)-1;
    BOOL got = FALSE, readonly = FALSE, maprom = FALSE;
    int p96special = FALSE;

#ifdef NATMEM_OFFSET
    unsigned int size = shmids[shmid].size;
    unsigned int readonlysize = size;

    if (shmids[shmid].attached)
        return shmids[shmid].attached;

    if ((uae_u8*)shmaddr < natmem_offset) {
        if(!_tcscmp (shmids[shmid].name, _T("chip"))) {
            shmaddr=natmem_offset;
            got = TRUE;
            if (getz2endaddr () <= 2 * 1024 * 1024 || currprefs.chipmem_size < 2 * 1024 * 1024)
                size += BARRIER;
        } else if(!_tcscmp (shmids[shmid].name, _T("kick"))) {
            shmaddr=natmem_offset + 0xf80000;
            got = TRUE;
            size += BARRIER;
            readonly = TRUE;
            maprom = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("rom_a8"))) {
            shmaddr=natmem_offset + 0xa80000;
            got = TRUE;
            readonly = TRUE;
            maprom = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("rom_e0"))) {
            shmaddr=natmem_offset + 0xe00000;
            got = TRUE;
            readonly = TRUE;
            maprom = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("rom_f0"))) {
            shmaddr=natmem_offset + 0xf00000;
            got = TRUE;
            readonly = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("rtarea"))) {
            shmaddr=natmem_offset + rtarea_base;
            got = TRUE;
            readonly = TRUE;
            readonlysize = RTAREA_TRAPS;
        } else if(!_tcscmp (shmids[shmid].name, _T("fast"))) {
            shmaddr=natmem_offset + 0x200000;
            got = TRUE;
            if (!(currprefs.rtgmem_size && !currprefs.rtgmem_type))
                size += BARRIER;
        } else if(!_tcscmp (shmids[shmid].name, _T("z2_gfx"))) {
            ULONG start = getz2rtgaddr ();
            got = TRUE;
            p96special = TRUE;
            shmaddr = natmem_offset + start;
            p96ram_start = start;
            if (start + currprefs.rtgmem_size < 10 * 1024 * 1024)
                size += BARRIER;
        } else if(!_tcscmp (shmids[shmid].name, _T("ramsey_low"))) {
            shmaddr=natmem_offset + a3000lmem_bank.start;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("ramsey_high"))) {
            shmaddr=natmem_offset + a3000hmem_bank.start;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("z3"))) {
            shmaddr=natmem_offset + z3fastmem_bank.start;
            if (!currprefs.z3fastmem2_size)
                size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("z3_2"))) {
            shmaddr=natmem_offset + z3fastmem_bank.start + currprefs.z3fastmem_size;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("z3_chip"))) {
            shmaddr=natmem_offset + z3chipmem_bank.start;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("z3_gfx"))) {
            got = TRUE;
            p96special = TRUE;
            p96ram_start = p96mem_offset - natmem_offset;
            shmaddr = natmem_offset + p96ram_start;
            size += BARRIER;
        } else if(!_tcscmp (shmids[shmid].name, _T("bogo"))) {
            shmaddr=natmem_offset+0x00C00000;
            got = TRUE;
            if (currprefs.bogomem_size <= 0x100000)
                size += BARRIER;
        } else if(!_tcscmp (shmids[shmid].name, _T("filesys"))) {
            static uae_u8 *filesysptr;
            if (filesysptr == NULL)
                filesysptr = xcalloc (uae_u8, size);
            result = filesysptr;
            shmids[shmid].attached = result;
            shmids[shmid].fake = true;
            return result;
        } else if(!_tcscmp (shmids[shmid].name, _T("custmem1"))) {
            shmaddr=natmem_offset + currprefs.custom_memory_addrs[0];
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("custmem2"))) {
            shmaddr=natmem_offset + currprefs.custom_memory_addrs[1];
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("hrtmem"))) {
            shmaddr=natmem_offset + 0x00a10000;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("arhrtmon"))) {
            shmaddr=natmem_offset + 0x00800000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("xpower_e2"))) {
            shmaddr=natmem_offset + 0x00e20000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("xpower_f2"))) {
            shmaddr=natmem_offset + 0x00f20000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("nordic_f0"))) {
            shmaddr=natmem_offset + 0x00f00000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("nordic_f4"))) {
            shmaddr=natmem_offset + 0x00f40000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("nordic_f6"))) {
            shmaddr=natmem_offset + 0x00f60000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp(shmids[shmid].name, _T("superiv_b0"))) {
            shmaddr=natmem_offset + 0x00b00000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("superiv_d0"))) {
            shmaddr=natmem_offset + 0x00d00000;
            size += BARRIER;
            got = TRUE;
        } else if(!_tcscmp (shmids[shmid].name, _T("superiv_e0"))) {
            shmaddr=natmem_offset + 0x00e00000;
            size += BARRIER;
            got = TRUE;
        }
    }
#endif

    if (shmids[shmid].key == shmid && shmids[shmid].size) {
        DWORD protect = readonly ? PAGE_READONLY : PAGE_READWRITE;
        shmids[shmid].mode = protect;
        shmids[shmid].rosize = readonlysize;
        shmids[shmid].natmembase = natmem_offset;
        shmids[shmid].maprom = maprom ? 1 : 0;
        if (shmaddr)
            virtualfreewithlock (shmaddr, size, MEM_DECOMMIT);
        result = virtualallocwithlock (shmaddr, size, MEM_COMMIT, PAGE_READWRITE);
        if (result == NULL)
            virtualfreewithlock (shmaddr, 0, MEM_DECOMMIT);
        result = virtualallocwithlock (shmaddr, size, MEM_COMMIT, PAGE_READWRITE);
        if (result == NULL) {
            result = (void*)-1;
            write_log (_T("VA %08X - %08X %x (%dk) failed %d\n"),
                (uae_u8*)shmaddr - natmem_offset, (uae_u8*)shmaddr - natmem_offset + size,
                size, size >> 10, GetLastError ());
        } else {
            shmids[shmid].attached = result;
            write_log (_T("VA %08X - %08X %x (%dk) ok (%08X)%s\n"),
                (uae_u8*)shmaddr - natmem_offset, (uae_u8*)shmaddr - natmem_offset + size,
                size, size >> 10, shmaddr, p96special ? _T(" P96") : _T(""));
        }
    }
    return result;
}

void unprotect_maprom (void)
{
    bool protect = false;
    for (int i = 0; i < MAX_SHMID; i++) {
        DWORD old;
        struct shmid_ds *shm = &shmids[i];
        if (shm->mode != PAGE_READONLY)
            continue;
        if (!shm->attached || !shm->rosize)
            continue;
        if (shm->maprom <= 0)
            continue;
        shm->maprom = -1;
#ifdef WINDOWS
        if (!VirtualProtect (shm->attached, shm->rosize, protect ? PAGE_READONLY : PAGE_READWRITE, &old)) {
            write_log (_T("VP %08X - %08X %x (%dk) failed %d\n"),
                (uae_u8*)shm->attached - natmem_offset, (uae_u8*)shm->attached - natmem_offset + shm->size,
                shm->size, shm->size >> 10, GetLastError ());
        }
#endif
    }
}

void protect_roms (bool protect)
{
    if (protect) {
        // protect only if JIT enabled, always allow unprotect
        if (!currprefs.cachesize || currprefs.comptrustbyte || currprefs.comptrustword || currprefs.comptrustlong)
            return;
    }
    for (int i = 0; i < MAX_SHMID; i++) {
        DWORD old;
        struct shmid_ds *shm = &shmids[i];
        if (shm->mode != PAGE_READONLY)
            continue;
        if (!shm->attached || !shm->rosize)
            continue;
        if (shm->maprom < 0 && protect)
            continue;
#ifdef WINDOWS
        if (!VirtualProtect (shm->attached, shm->rosize, protect ? PAGE_READONLY : PAGE_READWRITE, &old)) {
            write_log (_T("VP %08X - %08X %x (%dk) failed %d\n"),
                (uae_u8*)shm->attached - natmem_offset, (uae_u8*)shm->attached - natmem_offset + shm->size,
                shm->size, shm->size >> 10, GetLastError ());
        }
#endif
    }
}

int uae_shmdt (const void *shmaddr)
{
    return 0;
}

int uae_shmget (uae_key_t key, size_t size, int shmflg, const TCHAR *name)
{
    int result = -1;

    if((key == IPC_PRIVATE) || ((shmflg & IPC_CREAT) && (find_shmkey (key) == -1))) {
        write_log (_T("shmget of size %d (%dk) for %s\n"), size, size >> 10, name);
        if ((result = get_next_shmkey ()) != -1) {
            shmids[result].size = size;
            _tcscpy (shmids[result].name, name);
        } else {
            result = -1;
        }
    }
    return result;
}

int uae_shmctl (int shmid, int cmd, struct shmid_ds *buf)
{
    int result = -1;

    if ((find_shmkey (shmid) != -1) && buf) {
        switch (cmd)
        {
        case IPC_STAT:
            *buf = shmids[shmid];
            result = 0;
            break;
        case IPC_RMID:
            VirtualFree (shmids[shmid].attached, shmids[shmid].size, MEM_DECOMMIT);
            shmids[shmid].key = -1;
            shmids[shmid].name[0] = '\0';
            shmids[shmid].size = 0;
            shmids[shmid].attached = 0;
            shmids[shmid].mode = 0;
            result = 0;
            break;
        }
    }
    return result;
}

#endif

#ifdef FSUAE
#else
int isinf (double x)
{
    const int nClass = _fpclass (x);
    int result;
    if (nClass == _FPCLASS_NINF || nClass == _FPCLASS_PINF)
        result = 1;
    else
        result = 0;
    return result;
}
#endif



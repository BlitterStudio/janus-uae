/**
  * UAE - The Un*x Amiga Emulator
  *
  * Hardfile emulation for *nix systems
  *
  * Copyright 2003-2006 Richard Drummond
  * Based on hardfile_win32.c
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "filesys.h"

//#define HDF_DEBUG
#ifdef  HDF_DEBUG
#define DEBUG_LOG write_log( "%s: ", __func__); write_log
#else
#define DEBUG_LOG(...) do ; while(0)
#endif


static int hdf_seek (struct hardfiledata *hfd, uae_u64 offset)
{
    off_t ret;

    DEBUG_LOG ("called with offset=0x%llx\n", offset);

    if (hfd->offset != hfd->offset2 || hfd->size != hfd->size2) {
	gui_message ("hd: memory corruption detected in seek");
	abort ();
    }

    if (offset >= hfd->size) {
	gui_message ("hd: tried to seek out of bounds! (0x%llx >= 0x%llx)\n", offset, hfd->size);
	abort ();
    }

    offset += hfd->offset;
    if (offset & (hfd->blocksize - 1)) {
	gui_message ("hd: poscheck failed, offset not aligned to blocksize! (0x%llx & 0x%04.4x = 0x%04.4x)\n", offset, hfd->blocksize, offset & (hfd->blocksize - 1));
	abort ();
    }

  if (offset >= 0x80000000 && sizeof (off_t) < sizeof (uae_u64)) {
	DEBUG_LOG ("Failed to seek passed 2GB limit (0x%llx)\n", offset);
	return -1;
    }

    ret = lseek ((int)hfd->handle, offset, SEEK_SET);

    if (ret == -1) {
	DEBUG_LOG ("seek failed\n");
	return -1;
    }

    DEBUG_LOG ("seek okay\n");
    return 0;
}

static void poscheck (struct hardfiledata *hfd, int len)
{
    off_t pos;

    if (hfd->offset != hfd->offset2 || hfd->size != hfd->size2) {
	gui_message ("hd: memory corruption detected in poscheck");
	abort ();
    }

    pos = lseek ((int)hfd->handle, 0, SEEK_CUR);

    if (pos == -1 ) {
	gui_message ("hd: poscheck failed. seek failure, error %d", errno);
	abort ();
    }
    if (len < 0) {
	gui_message ("hd: poscheck failed, negative length! (%d)", len);
	abort ();
    }

    if ((uae_u64)pos < hfd->offset) {
	gui_message ("hd: poscheck failed, offset out of bounds! (0x%llx < 0x%llx)", pos, hfd->offset);
	abort ();
    }
    if ((uae_u64)pos >= hfd->offset + hfd->size || (uae_u64)pos >= hfd->offset + hfd->size + len) {
	gui_message ("hd: poscheck failed, offset out of bounds! (0x%llx >= 0x%llx, LEN=%d)", pos, hfd->offset + hfd->size, len);
	abort ();
    }
    if (pos & (hfd->blocksize - 1)) {
	gui_message ("hd: poscheck failed, offset not aligned to blocksize! (0x%llx & 0x%04.4x = 0x%04.4x\n", pos, hfd->blocksize, pos & hfd->blocksize);
	abort ();
    }
}

int hdf_open (struct hardfiledata *hfd, const char *name)
{
    int handle;

    DEBUG_LOG ("called with name=%s\n",name);

   if ((handle = open (name, hfd->readonly ? O_RDONLY : O_RDWR)) != -1) {
	int i;

	hfd->handle = (void *) handle;
	hfd->cache = 0;

	i = strlen (name) - 1;
	while (i >= 0) {
	    if ((i > 0 && (name[i - 1] == '/' || name[i - 1] == '\\')) || i == 0) {
		strcpy (hfd->vendor_id, "UAE");
		strncpy (hfd->product_id, name + i, 15);
		strcpy (hfd->product_rev, "0.2");
		break;
	    }
	    i--;
	}

	hfd->size = hfd->size2 = lseek (handle, 0, SEEK_END);
	hfd->blocksize = 512;
	lseek (handle, 0, SEEK_SET);

	DEBUG_LOG ("okay\n");
	return 1;
    }

    DEBUG_LOG ("failed\n");
    return 0;
}

extern int hdf_dup (struct hardfiledata *dhfd, const struct hardfiledata *shfd)
{
    DEBUG_LOG ("called\n");

    if ((int)shfd->handle >= 0) {
	dhfd->handle = (void *)dup ((int)shfd->handle);
    }

    return 0;
}

void hdf_close (struct hardfiledata *hfd)
{
    DEBUG_LOG ("called\n");

    close ((int)hfd->handle);
    hfd->handle = (void *)-1;
}

int hdf_read (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
    int n;

    DEBUG_LOG ("called with offset=0x%llx len=%d\n", offset, len);

    hfd->cache_valid = 0;
    hdf_seek (hfd, offset);
    poscheck (hfd, len);
    n = read ((int)hfd->handle, buffer, len);

    DEBUG_LOG ("read %d bytes\n", n);

    return n;
}

int hdf_write (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
    int n;

    DEBUG_LOG ("called with offset=0x%llx len=%d\n", offset, len);

    hfd->cache_valid = 0;
    hdf_seek (hfd, offset);
    poscheck (hfd, len);
    n = write ((int)hfd->handle, buffer, len);

    DEBUG_LOG ("Wrote %d bytes\n", n);

    return n;
}

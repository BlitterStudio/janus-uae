 /*
  * UAE - The Un*x Amiga Emulator
  *
  * OS-specific memory support functions
  *
  * Copyright 2004 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <proto/exec.h>
#include <exec/exec.h>

#include "memory_uae.h"

uae_u32 max_z3fastmem=0;

#ifdef JIT


/*
 * Allocate executable memory for JIT cache
 */
void *cache_alloc (int size)
{
   return malloc (size);
}

void cache_free (void *cache)
{
    xfree (cache);
}

bool init_mem() {
  LONG biggest;
  LONG all;

  biggest=AvailMem(MEMF_ANY|MEMF_LARGEST);
  all=AvailMem(MEMF_ANY);
  DebOut("free mem     : %d\n", all);
  DebOut("biggest chunk: %d\n", biggest);

  /* ensure at least 5MB free ram.. */
  if(all-biggest < 5*1024*1024) {
    biggest=biggest-( 5*1024*1024-(all-biggest));
  }
  max_z3fastmem=biggest;
  DebOut("max_z3fastmem: %d\n", max_z3fastmem);

  p96memstart();
}

#endif

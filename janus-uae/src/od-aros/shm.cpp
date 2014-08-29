/************************************************************************ 
 *
 * SHM functins
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

#include "sysconfig.h"
#include "sysdeps.h"

#include <proto/exec.h>
#include <exec/exec.h>

#include "include/memory.h"
#include "include/options.h"

#ifdef JIT

#ifdef NATMEM_OFFSET
void init_shm (void) {

  canbang = 1;
}
#endif

#endif /* JIT */

void p96memstart (void) {
  //uae_u32 rtgbarrier, z3chipbarrier, rtgextra;

  if(!currprefs.gfxmem_size) {
    DebOut("WARNING: currprefs.gfxmem_size is 0\n");
    return;
  }

  /* taken from puae */
	p96ram_start = currprefs.z3fastmem_start + ((currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size + 0xffffff) & ~0xffffff);
	if (p96ram_start == currprefs.z3fastmem_start + currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size &&
		(currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size < 512 * 1024 * 1024 || currprefs.gfxmem_size < 128 * 1024 * 1024)) {

    p96ram_start += 0x1000000;
  }

  DebOut("p96ram_start: %lx\n", p96ram_start);

  //rtgbarrier = 16 * 1024 * 1024 - ((currprefs.z3fastmem_size + currprefs.z3fastmem2_size) & 0x00ffffff);
  

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

#warning  p96memstart();
}


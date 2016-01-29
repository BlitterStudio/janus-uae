/*
 * AROS virtual memory functions for UAE.
 * Copyright (C) 2015 Oliver Brunner 
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#define JUAE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/vm.h"
#include "uae/log.h"
#include "memory.h"

static APTR vm_pool=NULL;

int uae_vm_page_size(void)
{
	return 4096;
}

void *uae_vm_alloc(uae_u32 size, int flags, int protect)
{
  APTR mem;

  DebOut("size: %lx, flags: %d, protect: %d\n", size, flags, protect);
  mem=AllocPooled(vm_pool, size);
  DebOut("allocated %d bytes at 0x%lx\n", size, mem);

	return (void *)mem;
}

bool uae_vm_protect(void *address, int size, int protect) {
	return FALSE;
}

static bool do_free(void *address, int size) {

  if(!vm_pool) return false;

  FreePooled(vm_pool, address, size);

	return true;
}

bool uae_vm_free(void *address, int size)
{
	uae_log("VM: Free     0x%-8x bytes at %p\n", size, address);
	return do_free(address, size);
}

void *uae_vm_reserve(uae_u32 size, int flags)
{
  APTR mem;

  mem=AllocPooled(vm_pool, size);

  if(!mem) {
    uae_log("VM: Reserve  0x%-8x bytes failed!\n", size);
    DebOut("no reserve memory (request was for %d bytes) on AROS\n", size);
  }
	return mem;
}

void *uae_vm_reserve_fixed(void *want_addr, uae_u32 size, int flags)
{
  TODO();
  uae_log("VM: Reserve  0x%-8x bytes at %p failed!\n", size, want_addr);
  DebOut("no reserve memory (request was for %d bytes at %p) on AROS\n", size, want_addr);
	return NULL;
}

void *uae_vm_commit(void *address, uae_u32 size, int protect)
{
  DebOut("ignored: uae_vm_commit for address %p (ok?)\n", address);
  //uae_log("VM: ERROR: uae_vm_commit is not available on AROS!!\n");
	return address;
}

bool uae_vm_decommit(void *address, uae_u32 size)
{
	//uae_log("VM: Decommit 0x%-8x bytes at %p faild\n", size, address);
  DebOut("ignored: uae_vm_decommit 0x%-8x bytes at %p (ok?)\n", size, address);

  return TRUE;
}

/***************************************************************
 *
 * vm_prep/vm_free are called by AROS on
 * startup/exit via ADD2INIT/ADD2EXIT
 *
 ***************************************************************/
void vm_prep(void) {

  vm_pool=CreatePool(MEMF_CLEAR, 16384, 8192);
  DebOut("vm_pool=%lx\n", vm_pool);

}
#ifndef UAE_ABI_v0
ADD2INIT(vm_prep, 0);
#endif

void vm_free(void) {

  DebOut("valloc_pool: %lx\n", vm_pool);
  if(vm_pool) {
    DeletePool(vm_pool);
    vm_pool=NULL;
  }

}
#ifndef UAE_ABI_v0
ADD2EXIT(vm_free, 0);
#endif



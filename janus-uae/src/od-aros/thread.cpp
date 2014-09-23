/************************************************************************ 
 *
 * thread.cpp
 *
 * Semaphores, Threads etc for AROS
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

#include <proto/exec.h>

#include "threaddep/thread.h"

#define AROS_MAX_SEM 1024

typedef struct {
  struct SignalSemaphore sigSem;
  bool init_done;
} aros_sem;

/* we use a static lookup table here for speed reasons! */
/* TODO: protect access with a semaphore! */
static aros_sem sem_table[AROS_MAX_SEM+1];

/* sem is a ULONG * to store number in it! */
void uae_sem_init (uae_sem_t *sem, int manual_reset, int initial_state) {
  struct SignalSemaphore *s;
  ULONG nr=1; /* leave first slot empty */

	DebOut("uae_sem_init(%lx)\n", sem);

  /* test for NULL pointer */
	if(!sem) {
		DebOut("WARNING: sem==NULL!\n");
		return;
	}

  while((nr<AROS_MAX_SEM) && (sem_table[nr].init_done!=FALSE)) {
    nr++;
  }

  if(nr==AROS_MAX_SEM) {
    write_log("FATAL ERROR in thread.cpp: Too many Semaphores used!\n");
    exit(1);
  }

  /* now we have found a slot */

  DebOut("Init aros_sem nr %d\n", nr);

  DebOut("InitSemaphore(%lx)\n", &sem_table[nr].sigSem);
	InitSemaphore(&sem_table[nr].sigSem);
  sem_table[nr].init_done=TRUE;

	if(initial_state) {
		ObtainSemaphore(&sem_table[nr].sigSem);
	}

  /* return new semaphore handle */
  *sem=nr;



#if 0
	if(*event) {
		if (initial_state)
			SetEvent (*event);
		else
			ResetEvent (*event);
	} else {
		*event = CreateEvent (NULL, manual_reset, initial_state, NULL);
	}
#endif
}

void uae_sem_wait (uae_sem_t *sem) {
  ULONG nr=*sem;

  DebOut("uae_sem_wait for nr %d\n", nr);

	if(!sem || !(sem_table[nr].init_done)) {
		DebOut("WARNING: sem==NULL!\n");
		return;
	}

	DebOut("ObtainSemaphore(%lx)\n", &sem_table[nr].sigSem);

	ObtainSemaphore(&sem_table[nr].sigSem);
#if 0
	WaitForSingleObject (*event, INFINITE);
#endif
}

void uae_sem_post (uae_sem_t *sem) {
  ULONG nr=*sem;

	if(!sem || !(sem_table[nr].init_done)) {
		DebOut("WARNING: sem==NULL!\n");
		return;
	}


#if 0
	SetEvent (*event);
#endif

	/* is it legal, to release a sempahore more than once !? */
	DebOut("ReleaseSemaphore(nr %d)\n", nr);
	ReleaseSemaphore(&sem_table[nr].sigSem);
}

int uae_sem_trywait (uae_sem_t *sem) {
  ULONG nr=*sem;
	ULONG result;

	if(!sem || !(sem_table[nr].init_done)) {
		DebOut("WARNING: sem==NULL!\n");
		return 0;
	}

	result=AttemptSemaphore(&sem_table[nr].sigSem);

	DebOut("AttemptSemaphore(nr %d)=%d\n", nr, result);

	return result;

#if 0
	return WaitForSingleObject (*event, 0) == WAIT_OBJECT_0 ? 0 : -1;
#endif
}

void uae_sem_destroy (uae_sem_t *sem) {
  ULONG nr=*sem;

	DebOut("uae_sem_destroy(nr %d)\n", nr);

	if(!sem) {
		DebOut("WARNING: sem==NULL!\n");
		return;
	}

  sem_table[nr].init_done=FALSE;

	/* nothing to do here */
#if 0
	if (*event) {
		CloseHandle (*event);
		*event = NULL;
	}
#endif
}

/* Windows API */
void InitializeCriticalSection(CRITICAL_SECTION *section) {
  DebOut("InitializeCriticalSection(%lx)\n", section);

  InitSemaphore(section);
}

void EnterCriticalSection(CRITICAL_SECTION *section) {
  DebOut("EnterCriticalSection(%lx)\n", section);

  ObtainSemaphore(section);
}

void LeaveCriticalSection(CRITICAL_SECTION *section) {
  DebOut("LeaveCriticalSection(%lx)\n", section);

  ReleaseSemaphore(section);
}




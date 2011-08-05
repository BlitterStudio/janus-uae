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

#include "sysconfig.h"
#include "sysdeps.h"

#include <proto/exec.h>

#include "threaddep/thread.h"

void uae_sem_init (uae_sem_t *sem, int manual_reset, int initial_state) {

	if(!sem) {
		DebOut("WARNING: sem==NULL!\n");
		return;
	}

	DebOut("init sem %lx\n", sem);
	InitSemaphore(sem);

	if(initial_state) {
		ObtainSemaphore(sem);
	}

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

	if(!sem) {
		DebOut("WARNING: sem==NULL!\n");
		return;
	}

	DebOut("ObtainSemaphore(%lx)\n", sem);

	ObtainSemaphore(sem);
#if 0
	WaitForSingleObject (*event, INFINITE);
#endif
}

void uae_sem_post (uae_sem_t *sem) {
#if 0
	SetEvent (*event);
#endif

	/* is it legal, to release a sempahore more than once !? */
	ReleaseSemaphore(sem);
}

int uae_sem_trywait (uae_sem_t *sem) {
	ULONG result;

	result=AttemptSemaphore(sem);

	DebOut("AttemptSemaphore(%lx)=%d\n", sem, result);

	return result;

#if 0
	return WaitForSingleObject (*event, 0) == WAIT_OBJECT_0 ? 0 : -1;
#endif
}

void uae_sem_destroy (uae_sem_t *sem) {

	DebOut("uae_sem_destroy(%lx)\n", sem);

	/* nothing to do here */
#if 0
	if (*event) {
		CloseHandle (*event);
		*event = NULL;
	}
#endif
}


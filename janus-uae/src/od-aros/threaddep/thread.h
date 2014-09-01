/************************************************************************ 
 *
 * thread.h
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

#warning ==== sem_handle is TODO ====
#if 0
typedef struct {
	int foo;
	int foo2;
} uae_sem_t;

typedef struct {
	int foo;
	int foo2;
} uae_thread_id;
#endif


typedef struct SignalSemaphore *uae_sem_t;
typedef void *uae_thread_id;

extern void uae_sem_destroy (uae_sem_t*);
extern int uae_sem_trywait (uae_sem_t*);
extern void uae_sem_post (uae_sem_t*);
extern void uae_sem_wait (uae_sem_t*t);
extern void uae_sem_init (uae_sem_t*, int manual_reset, int initial_state);
extern int uae_start_thread (const TCHAR *name, void *(*f)(void *), void *arg, uae_thread_id *thread);
extern int uae_start_thread_fast (void *(*f)(void *), void *arg, uae_thread_id *thread);
extern void uae_end_thread (uae_thread_id *thread);
extern void uae_set_thread_priority (uae_thread_id *, int);

#include "commpipe.h"

STATIC_INLINE void uae_wait_thread (uae_thread_id tid)
{
	TODO();
#if 0
    WaitForSingleObject (tid, INFINITE);
    CloseHandle (tid);
#endif
}

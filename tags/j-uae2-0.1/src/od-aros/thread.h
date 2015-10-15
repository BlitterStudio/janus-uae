/************************************************************************ 
 *
 * thread.h
 *
 * Semaphores, Threads etc for AROS
 *
 * Copyright 2015 Oliver Brunner - aros<at>oliver-brunner.de
 *           and many others..
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
#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdint.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "tchar.h"

#define CRITICAL_SECTION struct SignalSemaphore

void InitializeCriticalSection(CRITICAL_SECTION *section);
void EnterCriticalSection(CRITICAL_SECTION *section);
void LeaveCriticalSection(CRITICAL_SECTION *section);

typedef void *uae_thread_id;

extern void uae_end_thread (uae_thread_id *thread);
extern int uae_start_thread (const TCHAR *name, void *(*f)(void *), void *arg, uae_thread_id *thread);
extern int uae_start_thread_fast (void *(*f)(void *), void *arg, uae_thread_id *thread);
extern void uae_set_thread_priority (uae_thread_id *, int);


#ifdef __cplusplus
extern "C" {
#endif

struct fs_mutex;
typedef struct fs_mutex fs_mutex;

fs_mutex *fs_mutex_create(void);
void      fs_mutex_destroy(fs_mutex *mutex);
int       fs_mutex_lock(fs_mutex *mutex);
int       fs_mutex_unlock(fs_mutex *mutex);

struct fs_condition;
typedef struct fs_condition fs_condition;

fs_condition *fs_condition_create(void);
void          fs_condition_destroy(fs_condition *condition);
int           fs_condition_signal(fs_condition *condition);
int           fs_condition_wait(fs_condition *condition, fs_mutex *mutex);
int64_t       fs_condition_get_wait_end_time(int period);
int           fs_condition_wait_until(fs_condition *condition, fs_mutex *mutex, int64_t real_time);

struct fs_semaphore;
typedef struct fs_semaphore fs_semaphore;

fs_semaphore *fs_semaphore_create(int value);
void          fs_semaphore_destroy(fs_semaphore *semaphore);
int           fs_semaphore_post(fs_semaphore *semaphore);
int           fs_semaphore_wait(fs_semaphore *semaphore);
int           fs_semaphore_try_wait(fs_semaphore *semaphore);

#ifdef __cplusplus
}
#endif

typedef fs_semaphore *uae_sem_t;


static inline int uae_sem_init(uae_sem_t *sem, int dummy, int init) {
   *sem = fs_semaphore_create(init);
   return (*sem == 0);
}

static inline void uae_sem_destroy(uae_sem_t *sem) {
	return fs_semaphore_destroy(*sem);
}

static inline int uae_sem_post(uae_sem_t *sem) {
	return fs_semaphore_post(*sem);
}

static inline int uae_sem_wait(uae_sem_t *sem) {
	return fs_semaphore_wait(*sem);
}

static inline int uae_sem_trywait(uae_sem_t *sem) {
	return fs_semaphore_try_wait(*sem);
}

#include "commpipe.h"

STATIC_INLINE void uae_wait_thread (uae_thread_id tid) {
  TODO();
}
#endif // __THREAD_H__

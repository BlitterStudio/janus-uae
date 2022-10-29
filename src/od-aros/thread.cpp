/************************************************************************ 
 *
 * Thread support
 *
 * Copyright 2015 Oliver Brunner <aros<at>oliver-brunner.de>
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

//#define JUAE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"

#include "thread.h"

#include <proto/exec.h>

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

/* 
 * UAE sets task priorities only to "1", no other values seem to
 * be used. As for AmigaOS uncommon values may cause severe problems,
 * we only accept NULL/1, everything else needs to be checked!
 *
 * Increasing the thread priority is important for the filesystem 
 * task, otherwise virtual filesystem access is *very* slow.
 */
void uae_set_thread_priority (uae_thread_id *tid, int pri) {

  DebOut("uae_thread_id %lx, pri %d\n", tid, pri);

  if(tid!=NULL || pri>1) {
    bug("ERROR: uae_set_thread_priority with unsupported values: uae_thread_id %lx, pri %d!\n", tid, pri);
    return;
  }

  SetTaskPri(FindTask(NULL), pri);
}

/*
 * taken from fs-uae (fs-uae-2.5.23dev)
 *
 * we don't use the thread implementaion, but we use the semaphore implementations in
 * winuae for AROS
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __AROS__
#define USE_SDL
//#define USE_PTHREADS
#define g_malloc malloc
#define g_free free
#endif

#include <thread.h>
//#include <fs/base.h>
#include <stdlib.h>

#ifdef USE_GLIB
#include <glib.h>
#endif

#ifdef USE_SDL2
#define USE_SDL
#endif

#ifdef USE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#endif

#ifdef USE_PTHREADS
#include <pthread.h>
#define USE_PSEM
#endif

#ifdef USE_PSEM
#ifndef __AROS__
#include <semaphore.h>
#else
#undef USE_PSEM
#endif
#endif

struct fs_mutex {
#if defined(USE_PTHREADS)
    pthread_mutex_t mutex;
#elif defined(USE_GLIB)
    GMutex mutex;
#elif defined(USE_SDL)
    SDL_mutex* mutex;
#endif
};

struct fs_condition {
#if defined(USE_PTHREADS)
    pthread_cond_t condition;
#elif defined(USE_GLIB)
    GCond condition;
#elif defined(USE_SDL)
    SDL_cond* condition;
#endif
};

struct fs_semaphore {
#if defined(USE_PSEM)
    sem_t semaphore;
#elif defined(USE_SDL)
    SDL_sem* semaphore;
#endif
};

fs_mutex *fs_mutex_create() {
    fs_mutex *mutex = (fs_mutex *) g_malloc(sizeof(fs_mutex));
#if defined(USE_PTHREADS)
    pthread_mutex_init(&mutex->mutex, NULL);
#elif defined(USE_GLIB)
    g_mutex_init(&mutex->mutex);
#elif defined(USE_SDL)
    mutex->mutex = SDL_CreateMutex();
#else
#error no thread support
#endif
    return mutex;
}

void fs_mutex_destroy(fs_mutex *mutex) {

  DebOut("mutex %d\n", mutex);
#if defined(USE_PTHREADS)
    pthread_mutex_destroy(&mutex->mutex);
#elif defined(USE_GLIB)
    g_mutex_clear(&mutex->mutex);
#elif defined(USE_SDL)
    SDL_DestroyMutex(mutex->mutex);
#else
#error no thread support
#endif
    g_free(mutex);
}

int fs_mutex_lock(fs_mutex *mutex) {
#if defined(USE_PTHREADS)
    return pthread_mutex_lock(&mutex->mutex);
#elif defined(USE_GLIB)
    g_mutex_lock(&mutex->mutex);
    return 0;
#elif defined(USE_SDL)
    return SDL_mutexP(mutex->mutex);
#else
#error no thread support
#endif
}

int fs_mutex_unlock(fs_mutex *mutex) {
#if defined(USE_PTHREADS)
    return pthread_mutex_unlock(&mutex->mutex);
#elif defined(USE_GLIB)
    g_mutex_unlock(&mutex->mutex);
    return 0;
#elif defined(USE_SDL)
    return SDL_mutexV(mutex->mutex);
#else
#error no thread support
#endif
}

fs_condition *fs_condition_create(void) {
    fs_condition *condition = (fs_condition *) g_malloc(sizeof(fs_condition));
#if defined(USE_PTHREADS)
    pthread_cond_init(&condition->condition, NULL);
#elif defined(USE_GLIB)
    g_cond_init(&condition->condition);
#elif defined(USE_SDL)
    condition->condition = SDL_CreateCond();
#else
#error no thread support
#endif
    return condition;
}

void fs_condition_destroy(fs_condition *condition) {
#if defined(USE_PTHREADS)
    pthread_cond_destroy(&condition->condition);
#elif defined(USE_GLIB)
    g_cond_clear(&condition->condition);
#elif defined(USE_SDL)
    SDL_DestroyCond(condition->condition);
#else
#error no thread support
#endif
    g_free(condition);
}

int fs_condition_wait (fs_condition *condition, fs_mutex *mutex) {
#if defined(USE_PTHREADS)
    return pthread_cond_wait(&condition->condition, &mutex->mutex);
#elif defined(USE_GLIB)
    g_cond_wait(&condition->condition, &mutex->mutex);
    return 0;
#elif defined(USE_SDL)
    return SDL_CondWait(condition->condition, mutex->mutex);
#else
#error no thread support
#endif
}

#ifndef __AROS__
int64_t fs_condition_get_wait_end_time(int period) {
#if defined(USE_GLIB)
    return fs_get_monotonic_time() + period;
#else
    return fs_get_current_time() + period;
#endif
}
#endif

int fs_condition_wait_until(fs_condition *condition, fs_mutex *mutex,
        int64_t end_time) {
#if defined(USE_PTHREADS)
    struct timespec tv;
    tv.tv_sec = end_time / 1000000;
    tv.tv_nsec = (end_time % 1000000) * 1000;
    return pthread_cond_timedwait(&condition->condition, &mutex->mutex, &tv);
#elif defined(USE_GLIB)
    // GTimeVal tv;
    // tv.tv_sec = end_time / 1000000;
    // tv.tv_usec = end_time % 1000000;
    // gboolean result = g_cond_timed_wait(
    //         &condition->condition, &mutex->mutex, &tv);
    gboolean result = g_cond_wait_until(
             &condition->condition, &mutex->mutex, end_time);
    return !result;
#elif defined(USE_SDL)
    Uint32 ms=(Uint32) (end_time / 1000);
    // FIXME: no timed wait...
    //return SDL_CondWait(condition->condition, mutex->mutex);
    //WARNING: never tested (Timeout) oli
    return SDL_CondWaitTimeout(condition->condition, mutex->mutex, ms);
#else
#error no thread support
#endif
}

int fs_condition_signal(fs_condition *condition) {
#if defined(USE_PTHREADS)
    return pthread_cond_signal(&condition->condition);
#elif defined(USE_GLIB)
    g_cond_signal(&condition->condition);
    return 0;
#elif defined(USE_SDL)
    return SDL_CondSignal(condition->condition);
#else
#error no thread support
#endif
}

fs_semaphore *fs_semaphore_create(int value) {
    fs_semaphore *semaphore = (fs_semaphore *) g_malloc(sizeof(fs_semaphore));
#if defined(USE_PSEM)
    sem_init(&semaphore->semaphore, 1, value);
#elif defined(USE_SDL)
    semaphore->semaphore = SDL_CreateSemaphore(value);
#else
#error no thread support
#endif
    return semaphore;
}

void fs_semaphore_destroy(fs_semaphore *semaphore) {

  DebOut("semaphore: %lx\n", semaphore);
  if(!semaphore) {
    DebOut("ERROR: semaphore is NULL!? This would crash WinUAE..\n");
    return;
  }
#if defined(USE_PSEM)
    sem_destroy(&semaphore->semaphore);
#elif defined(USE_SDL)
    SDL_DestroySemaphore(semaphore->semaphore);
#else
#error no thread support
#endif
    g_free(semaphore);
}

int fs_semaphore_post(fs_semaphore *semaphore) {
#if defined(USE_PSEM)
    return sem_post(&semaphore->semaphore);
#elif defined(USE_SDL)
    return SDL_SemPost(semaphore->semaphore);
#else
#error no thread support
#endif
}

int fs_semaphore_wait(fs_semaphore *semaphore) {
#if defined(USE_PSEM)
    return sem_wait(&semaphore->semaphore);
#elif defined(USE_SDL)
    return SDL_SemWait(semaphore->semaphore);
#else
#error no thread support
#endif
}

int fs_semaphore_try_wait(fs_semaphore *semaphore) {
#if defined(USE_PSEM)
    return sem_trywait(&semaphore->semaphore);
#elif defined(USE_SDL)
    return SDL_SemTryWait(semaphore->semaphore);
#else
#error no thread support
#endif
}


 /*
  * E-UAE - The portable Amiga Emulator
  *
  * Thread and semaphore support using AmigaOS processes
  *
  * Copyright 2003-2005 Richard Drummond
  */

#include <exec/semaphores.h>
#include <exec/lists.h>

/*
 * Emulation of POSIX sempahores.
 */

struct PSemaphore {
   struct SignalSemaphore mutex;
   struct MinList         wait_queue;
   int                    value;
   int                    live;
};

typedef struct PSemaphore uae_sem_t;

int uae_sem_init (uae_sem_t *sem, int pshared, unsigned int value);

int uae_sem_destroy (uae_sem_t *sem);

int uae_sem_post (uae_sem_t *sem);

int uae_sem_wait (uae_sem_t *sem);

int uae_sem_trywait (uae_sem_t *sem);

int uae_sem_get_value (uae_sem_t *sem);

/*
 * Handle CreateNewProc() differences between AmigaOS-like systems 
 */
#ifdef __MORPHOS__
/* CreateNewProc() on MorphOS needs to be told that code is PPC */
# define myCreateNewProcTags(...) CreateNewProcTags(NP_CodeType, CODETYPE_PPC, __VA_ARGS__)
#else
# ifdef __amigaos4__
/* On OS4, we assert that the threads we create are our children */
#  define myCreateNewProcTags(...) CreateNewProcTags(NP_Child, TRUE, __VA_ARGS__)
# else
#  define myCreateNewProcTags CreateNewProcTags
# endif
#endif


#include "commpipe.h"

/*
 * Threading using AmigaOS processes
 */
typedef struct Task *uae_thread_id;
#define BAD_THREAD NULL

extern int uae_start_thread (void *(*f) (void *), void *arg, uae_thread_id *foo);

extern void uae_set_thread_priority (int pri);

/* Do nothing; thread exits if thread function returns.  */
#define UAE_THREAD_EXIT do {} while (0)

#define uae_wait_thread(id)
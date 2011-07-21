
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


typedef void *uae_sem_t;
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

#ifndef LOCK_IMPL_H
#define LOCK_IMPL_H

#ifdef LOCAL_SEM
#include <l4/semaphore/semaphore.h>

#define LOCK_TYPE l4semaphore_t
#define LOCK_SIZE sizeof(l4semaphore_t)
#define LOCK_DOWN(sem) l4semaphore_down(sem)
#define LOCK_UP(sem) l4semaphore_up(sem)
#define LOCK_INIT(sem) *sem = L4SEMAPHORE_INIT(1)
static inline void lock_map_to(l4_threadid_t t, LOCK_TYPE *l) {}
static inline void lock_rcv_map(LOCK_TYPE *l) {}
static inline void free_lock(LOCK_TYPE *l) {}

#elif L4UTIL_LOCK
#include <l4/util/lock.h>

#define LOCK_TYPE l4util_simple_lock_t
#define LOCK_SIZE sizeof(l4util_simple_lock_t)
#define LOCK_INIT(lock) l4_simple_unlock(lock)
#define LOCK_DOWN(lock) l4_simple_lock(lock)
#define LOCK_UP(lock) l4_simple_unlock(lock)
static inline void lock_map_to(l4_threadid_t t, LOCK_TYPE *l) {}
static inline void lock_rcv_map(LOCK_TYPE *l) {}
static inline void free_lock(LOCK_TYPE *l) {}

#elif FIASCO_ULOCK
#include <l4/sys/user_locks.h>
#include <l4/sys/syscalls.h>

static int last_lock = 1;


#define LOCK_TYPE unsigned long
#define LOCK_SIZE sizeof(unsigned long)
#define LOCK_INIT(lock) do { *lock = last_lock++; l4_ulock_new(*(lock)); } while (0)
#define LOCK_DOWN(lock) l4_ulock_lock(*(lock))
#define LOCK_UP(lock) l4_ulock_unlock(*(lock))
static inline
void lock_map_to(l4_threadid_t t, LOCK_TYPE *l)
{
    l4_fpage_t fp = l4_iofpage(*(l), 0, 0); 
    fp.iofp.zero2 = 2; 
    l4_msgdope_t res; 
    l4_ipc_send(t, L4_IPC_SHORT_FPAGE, 0, fp.raw, L4_IPC_NEVER, &res); 
} 

static inline
void lock_rcv_map(LOCK_TYPE *l)
{
    l4_threadid_t t;
    l4_umword_t d1,d2;
    l4_fpage_t fp = l4_iofpage(*(l), 0, 0);
    fp.iofp.zero2 = 2;
    l4_msgdope_t res;
    l4_ipc_wait(&t, (void*)(fp.raw | 2), &d1, &d2, L4_IPC_NEVER, &res);
}

static inline void free_lock(LOCK_TYPE *l) 
{
    l4_fpage_t fp = l4_iofpage(*(l), 0, 0);
    fp.iofp.zero2 = 2;
    l4_fpage_unmap(fp, L4_FP_ALL_SPACES);
}

#elif FIASCO_USEM
#include <l4/sys/user_locks.h>

static int last_lock = 1;
typedef struct sem_t { l4_u_semaphore_t s; unsigned long ksem; } sem_t;

#define LOCK_TYPE sem_t
#define LOCK_SIZE sizeof(sem_t)
#define LOCK_INIT(_lock) do { _lock->ksem = last_lock++; l4_usem_new(_lock->ksem, 1, &(_lock->s)); } while (0)
#define LOCK_DOWN(_lock) l4_usem_down(_lock->ksem, &(_lock->s))
#define LOCK_UP(_lock) l4_usem_up(_lock->ksem, &(_lock->s))
static inline
void lock_map_to(l4_threadid_t t, LOCK_TYPE *l)
{
    l4_fpage_t fp = l4_iofpage((l)->ksem, 0, 0); 
    fp.iofp.zero2 = 2; 
    l4_msgdope_t res; 
    l4_ipc_send(t, L4_IPC_SHORT_FPAGE, 0, fp.raw, L4_IPC_NEVER, &res); 
} 

static inline
void lock_rcv_map(LOCK_TYPE *l)
{
    l4_threadid_t t;
    l4_umword_t d1,d2;
    l4_fpage_t fp = l4_iofpage(l->ksem, 0, 0);
    fp.iofp.zero2 = 2;
    l4_msgdope_t res;
    l4_ipc_wait(&t, (void*)(fp.raw | 2), &d1, &d2, L4_IPC_NEVER, &res);
}

static inline void free_lock(LOCK_TYPE *l) 
{
    l4_fpage_t fp = l4_iofpage(l->ksem, 0, 0);
    fp.iofp.zero2 = 2;
    l4_fpage_unmap(fp, L4_FP_ALL_SPACES);
}

#endif

#ifndef LOCK_TYPE
#error LOCK_* defines are not set
#endif

#endif

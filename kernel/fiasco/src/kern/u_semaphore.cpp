INTERFACE:

#include "mapping_tree.h"
#include "mappable.h"
#include "kmem_slab.h"
#include "l4_types.h"
#include "prio_list.h"
#include "thread.h"
#include "slab_cache_anon.h"

class Ram_quota;

class U_semaphore : public Mappable
{
private:
  typedef slab_cache_anon Allocator;

  Ram_quota *_q;
  unsigned long _cnt;
  Prio_list _queue;
  bool _valid;
public:
  enum Result { Ok, Retry, Timeout, Invalid };

public:
  virtual ~U_semaphore();
};


IMPLEMENTATION:

#include "cpu_lock.h"
#include "ipc_timeout.h"
#include "mem_space.h"
#include "timer.h"


PUBLIC inline
U_semaphore::U_semaphore(Ram_quota *q) 
  : _q(q), _cnt(0), _valid(true)  
{}


PRIVATE inline NOEXPORT
void
U_semaphore::set_queued(L4_semaphore *sem, bool q)
{
  current_mem_space()->poke_user(&(sem->flags), (Mword)q);
}


PRIVATE inline NOEXPORT
bool
U_semaphore::pagein_set_queued(Thread *c, L4_semaphore *sem, bool q)
{
  jmp_buf pf_recovery;
  int err;
  if (EXPECT_TRUE ((err = setjmp(pf_recovery)) == 0))
    {
      c->recover_jmp_buf(&pf_recovery);
      // we are preemptible here, in case of a page fault
      current_mem_space()->poke_user(&(sem->flags), (Mword)q);
    }

  c->recover_jmp_buf(0);
  return err == 0;
}


PRIVATE inline NOEXPORT
bool
U_semaphore::add_counter(L4_semaphore *sem, long v)
{
  Smword cnt = current_mem_space()->peek_user(&(sem->counter)) + v;
  current_mem_space()->poke_user(&(sem->counter), cnt);
  return cnt;
}


PRIVATE inline NOEXPORT
bool
U_semaphore::valid_semaphore(L4_semaphore *s)
{
  if (EXPECT_FALSE(((unsigned long)s & (sizeof(L4_semaphore)-1)) != 0))
    return false;

  if (EXPECT_FALSE((unsigned long)s >= Mem_layout::User_max))
    return false;

  return true;
}


PUBLIC
U_semaphore::Result
U_semaphore::block_locked(L4_timeout const &to, L4_semaphore *sem)
{
  if (EXPECT_FALSE(!valid_semaphore(sem)))
    return Invalid;

  Thread *c = current_thread();
  if (EXPECT_FALSE (!pagein_set_queued(c, sem, true)))
    // unhandled page fault semaphore is considered invalid
    return Invalid;

  // *counter is now paged in writable

  if (add_counter(sem, 1) > 0) 
    {
      if (!_queue.head())
	set_queued(sem, false);

      add_counter(sem, -1);
      return Ok;
    }

  c->wait_queue(&_queue);
  c->sender_enqueue(&_queue, c->sched()->prio());
  c->state_del_dirty(Thread_ready);

  IPC_timeout timeout;

  if (!to.is_never())
    {
      Unsigned64 t = to.microsecs(Timer::system_clock());
      if (t)
	{
	  timeout.set(t);
	  c->set_timeout(&timeout);
	}
      else
	return Timeout;
    }

  c->schedule();
  // We go here by: (a) a wakeup, (b) a timeout, (c) wait_queue delete,
  // (d) ex_regs
  
  // The wait_queue was destroyed
  if (EXPECT_FALSE(!_valid))
    return Invalid;

  // Two cases:
  // 1. c is not in the queue, then the wakeup already occured
  // 2. c is in the sender list an the timeout has hit a timeout is flagged
  if (EXPECT_FALSE(c->in_sender_list() && timeout.has_hit()))
    {
      // The timeout really hit so remove c from the queue
      c->sender_dequeue(&_queue);
      return Timeout;
    }

  return Retry;
}


PUBLIC
void
U_semaphore::wakeup_locked(L4_semaphore *sem)
{
  if (EXPECT_FALSE(!valid_semaphore(sem)))
    return;

  Thread *c = current_thread();

  // basically make queued flag writable
  if (EXPECT_FALSE (!pagein_set_queued(c, sem, true)))
    // semaphore is invalid
    return;

  Prio_list_elem *h = _queue.head();
  if (!h)
    {
      set_queued(sem, false); // queue is empty
      return;
    }

  Thread *w = static_cast<Thread*>(Sender::cast(h));
  w->sender_dequeue(&_queue);
  w->state_add_dirty(Thread_ready);

  w->reset_timeout();
  w->wait_queue(0);

  if (!_queue.head())
    set_queued(sem, false); // dequeued the last thread

  if (Context::schedule_in_progress())
    return;

  if (Context::can_preempt_current(w->sched()))
    current()->switch_to_locked(w);
  else
    w->ready_enqueue();
}


IMPLEMENT
U_semaphore::~U_semaphore()
{
  _valid = false;

  while (Prio_list_elem *h = _queue.head())
    {
      Thread *w = static_cast<Thread*>(Sender::cast(h));
      w->sender_dequeue(&_queue);
      w->state_add(Thread_ready);
      w->ready_enqueue();
      w->reset_timeout();
    }

  Lock_guard<Cpu_lock> guard(&cpu_lock);

  if (!Context::schedule_in_progress())
    current()->schedule();
}


PUBLIC inline
unsigned
U_semaphore::dec_ref_cnt()
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);
  --_cnt;
  if (_cnt == 0 && Mappable::no_mappings())
    return 0;
  else
    return 1;
}


PUBLIC inline
void
U_semaphore::inc_ref_cnt()
{ ++_cnt; }


PUBLIC inline
bool 
U_semaphore::no_mappings() const
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);
  if (Mappable::no_mappings())
    return !_cnt;
  
  return false;
}



PUBLIC static
U_semaphore*
U_semaphore::alloc(Ram_quota *q)
{
  void *nq;
  if (q->alloc(sizeof(U_semaphore)) && (nq = allocator()->alloc()))
    return new (nq) U_semaphore(q);

  return 0;
}

PUBLIC
void *
U_semaphore::operator new (size_t, void *p)
{ return p; }

PUBLIC 
void
U_semaphore::operator delete (void *_l)
{
  U_semaphore *l = reinterpret_cast<U_semaphore*>(_l);
  if (l->_q)
    l->_q->free(sizeof(U_semaphore));

  allocator()->free(l);
}

PRIVATE static inline NOEXPORT NEEDS["kmem_slab.h"]
U_semaphore::Allocator *
U_semaphore::allocator()
{
  static Allocator* slabs = 
    new Kmem_slab_simple (sizeof (U_semaphore), sizeof (Mword), "U_semaphore");

  return slabs;
}


#if 0 // Promela model of the lock
#define MAX_THREADS    4
#define MAX_WQ_ENTRIES MAX_THREADS
#define MAX 1
#define LOOPS 1

#define LOCKED 0
#define RETRY  1
#define ERROR  2

bit thread_state[MAX_THREADS];
hidden byte loops[MAX_THREADS];
unsigned in_critical : 4 = 0;
hidden byte temp;

typedef sem_t
{
  short counter;
  bit queued;
  bit queue[MAX_WQ_ENTRIES];
}

sem_t sem;  /*  maybe move init. to init*/


inline init_globals()
{
  d_step
  {
    temp = 0;
    sem.counter = MAX;
    do
      ::
        sem.queue[temp] = 0;
	temp++;
	if
	  :: (temp >= MAX_WQ_ENTRIES) -> break;
	  :: else;
	fi
    od;
  }
}

inline enqueue_thread(t)
{
  sem.queue[t] = 1;
}

inline dequeue_thread(t)
{
  sem.queue[t] = 0;
}

inline queue_head(head)
{
  local_temp = 0;
  do
    ::
      if
        :: (sem.queue[local_temp]) -> head = local_temp; break;
	:: else;
      fi;
      local_temp++;
      if
        :: (local_temp >= MAX_WQ_ENTRIES) -> head = local_temp; break;
	:: else;
      fi;
  od
}

inline block(ret, thread)
{
  do ::
  atomic
  {
    sem.counter++;
    if
      :: (sem.counter > 0) -> sem.counter--; ret = LOCKED; break;
      :: else
    fi;

    sem.queued = 1;
    enqueue_thread(thread);
    thread_state[thread] = 0;

    if
      :: (thread_state[thread] == 1) -> skip
    fi;

    if
      :: (sem.queue[thread] == 0) -> ret = RETRY; break;
      :: else;
    fi;

    dequeue_thread(thread);
    ret = ERROR;
    break;
  }
  od
}

inline wakeup()
{
  do ::
  atomic
  {
    queue_head(pf);
    if
      :: (pf == MAX_THREADS) -> sem.queued = 0; break;
      :: else;
    fi;

    dequeue_thread(pf);
    thread_state[pf] = 1;
    queue_head(pf);
    if :: (pf == MAX_THREADS) -> sem.queued = 0;
       :: else;
    fi;
    break;
  }
  od
}

inline down(ret)
{
  do
    ::
      atomic
      {
	sem.counter--;
	if
	  :: (sem.counter >= 0) -> break;
	  :: else;
	fi
      }

      block(ret, thread);

      if
        :: (ret == LOCKED || ret == ERROR) -> break;
	:: (ret == RETRY);
	:: else assert(false);
      fi
  od
}


inline up()
{
  do
    ::
      sem.counter++;
      if
        :: (!sem.queued) -> break;
        :: else;
      fi;

      wakeup();
      break;
  od
}


proctype Killer()
{
  end_k:
  do
    ::
      if
        :: (thread_state[0] == 0) -> thread_state[0] = 1;
        :: (thread_state[1] == 0) -> thread_state[1] = 1;
        :: (thread_state[2] == 0) -> thread_state[2] = 1;
      fi
  od
}

proctype Thread(byte thread)
{
  unsigned pf : 4;
  unsigned ret : 4;
  unsigned local_temp : 4;

before_down:
L1:  do
    ::
      down(ret);
      if
        :: (ret == ERROR) -> goto L1;
        :: else;
      fi;
      atomic {
      in_critical++;
      assert (in_critical <= MAX);
      }

progress1:
      in_critical--;
      up();

      if
        :: (loops[thread] == 0) -> break;
	:: else;
      fi;
      loops[thread]--;
  od
}

hidden byte threads = 0;
init
{
  threads = 0;
  in_critical = 0;
  init_globals();
  run Killer();
  do
    ::
       loops[threads] = LOOPS - 1;
       run Thread(threads);
       threads++;
       if
         :: (threads >= MAX_THREADS) -> break;
         :: else;
       fi
  od
}

/*
never
{
  do
    :: (in_critical == MAX) -> assert(false)
    :: else;
  od
}
*/

#endif 

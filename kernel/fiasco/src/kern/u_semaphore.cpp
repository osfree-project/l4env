INTERFACE:

#include "mapping_tree.h"
#include "mappable.h"
#include "kmem_slab.h"
#include "l4_types.h"
#include "prio_list.h"
#include "thread.h"

class Ram_quota;

class U_semaphore : public Mappable
{
private:
  typedef slab_cache_anon Allocator;

  Ram_quota *_q;
  unsigned long _cnt;
  Prio_list _queue;
  unsigned _wakeup_pending;
  bool _valid;
public:
  enum Result { Ok, Timeout, Invalid };

public:
  virtual ~U_semaphore();
};


IMPLEMENTATION:

#include "cpu_lock.h"
#include "ipc_timeout.h"
#include "timer.h"


PUBLIC inline
U_semaphore::U_semaphore(Ram_quota *q) 
  : _q(q), _cnt(0), _wakeup_pending(0), _valid(true)  
{}


PUBLIC
U_semaphore::Result
U_semaphore::block_locked(L4_timeout const &to)
{ 
  if (_wakeup_pending)
  {
    --_wakeup_pending;
    return Ok;
  }

  Thread *c = current_thread();
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


  current()->schedule();

  if (EXPECT_FALSE(!_valid))
    return Invalid;

  if (EXPECT_FALSE(timeout.has_hit()))
    return Timeout;

  return Ok;
}


PUBLIC inline
void
U_semaphore::wakeup_locked()
{
  Prio_list_elem *h = _queue.head();
  if (!h)
    {
      ++_wakeup_pending;
      return;
    }

  Thread *w = static_cast<Thread*>(Sender::cast(h));
  w->sender_dequeue(&_queue);
  w->state_add_dirty(Thread_ready);

  w->reset_timeout();
  w->wait_queue(0);

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


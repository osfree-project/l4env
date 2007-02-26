INTERFACE:


#include "lock_guard.h"
#include "cpu_lock.h"

#include "sched.h"

void check_sched_state(void);

class Scheduler
{
  // CLASS DATA
protected:
  static sched_t *_prio_first[256] asm ("SCHEDULER_PRIO_FIRST");
  static sched_t *_prio_next[256]  asm ("SCHEDULER_PRIO_NEXT");
  static unsigned _prio_highest    asm ("SCHEDULER_PRIO_HIGHEST");
  static int      _timeslice_ticks_left;
  static sched_t *_timeslice_owner;


  friend void check_sched_state(void);
};

extern Scheduler *classic_scheduler;

IMPLEMENTATION:

#include <cassert>

sched_t *Scheduler::_prio_first[256];
sched_t *Scheduler::_prio_next[256];
unsigned Scheduler::_prio_highest;
int Scheduler::_timeslice_ticks_left;
sched_t *Scheduler::_timeslice_owner;

Scheduler scheduler_singleton;
Scheduler *classic_scheduler = &scheduler_singleton;

PUBLIC inline
void
Scheduler::save_ticks()
{
  _timeslice_owner->set_ticks_left (_timeslice_ticks_left 
		      ? _timeslice_ticks_left 
		      : _timeslice_owner->timeslice());
}


PUBLIC inline
int
Scheduler::get_ticks_left(void)
{
  return _timeslice_ticks_left;
}

PUBLIC inline
sched_t *
Scheduler::next_to_run(void)
{
  return _prio_next[_prio_highest];
}

PUBLIC inline
void
Scheduler::set_timeslice_owner(sched_t *next_to_run)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  assert(guard.was_set());

  _timeslice_owner = next_to_run;
  _timeslice_ticks_left = next_to_run->ticks_left();
}

PUBLIC inline
void
Scheduler::exhaust_timeslice()
{
  _timeslice_ticks_left = 0;
}

PUBLIC inline
void
Scheduler::next_if_timeslice_expired(void)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  assert(guard.was_set());

  if (! _timeslice_ticks_left
      && _timeslice_owner->next())
    {
      _prio_next[_timeslice_owner->prio()] = 
	_timeslice_owner->next()->prio() 
          == _timeslice_owner->prio() 
	? _timeslice_owner->next()
	: _prio_first[_timeslice_owner->prio()];
    }
}

PUBLIC inline NEEDS[<cassert>]
void
Scheduler::dequeue(sched_t *lnk)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  assert(guard.was_set());

  if (_prio_first[lnk->prio()] == lnk)
    {
      _prio_first[lnk->prio()] = lnk->next()->prio() == lnk->prio() ? 
	lnk->next() : 0;
    }
  if (_prio_next[lnk->prio()] == lnk)
    {
      _prio_next[lnk->prio()] = lnk->next()->prio() == lnk->prio() ?
	lnk->next()  : _prio_first[lnk->prio()];
    }
  if ((_prio_first[lnk->prio()] == 0) && (lnk->prio() == _prio_highest))
    {
      assert(lnk->next()->prio() < lnk->prio());
      _prio_highest = lnk->prev()->prio();
    }      

  lnk->dequeue();
}

PUBLIC void
Scheduler::enqueue(sched_t *lnk)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  sched_t *sibling = 0;
  unsigned i;

  unsigned prio = lnk->prio();

  if (! _prio_first[prio])
    _prio_first[prio] = lnk;
  if (! _prio_next[prio])
    _prio_next[prio] = lnk;
  
  if (prio > _prio_highest)
    _prio_highest = prio;

  // enqueue as the last tcb of this prio, i.e., just before the
  // first tcb of the next prio
  i = prio;
  do 
    {
      if (++i == 256) i = 0;
      sibling = _prio_first[i]; // find 1st tcb of next prio
    }
  while (! sibling);	// loop terminates at least at kernel_thread

  assert (! lnk->next());

  lnk->enqueue_before(sibling);
  

  assert(lnk->prev()->prio() <= prio
	 || prio == 0);

}

PUBLIC void
Scheduler::init(int time_slice, int prio, sched_t *idle)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  assert(guard.was_set());

  for (int i = 0; i < 256; i++) 
    {
      _prio_next[i] = _prio_first[i] = 0;
    }
  _prio_next[prio] = _prio_first[prio] = idle;
  _prio_highest = prio;

  _timeslice_ticks_left = time_slice;
  _timeslice_owner = idle;
}


PUBLIC bool
Scheduler::timeslice_ticks_left()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  assert(guard.was_set());

  return --_timeslice_ticks_left <= 0;
}

PUBLIC void
Scheduler::set_timeslice_ticks_left(int ticks)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  assert(guard.was_set());

  _timeslice_ticks_left = ticks;
}



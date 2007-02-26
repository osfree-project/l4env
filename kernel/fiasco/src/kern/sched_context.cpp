INTERFACE:

#include "types.h"

class slab_cache_anon;
class Context;

// Scheduling parameters
class Sched_context
{
public:

  // Constructor
  Sched_context		(Context *owner,
                         unsigned short id,
                         unsigned short prio,
                         unsigned short timeslice);

  void *		operator new	(size_t);
  void			operator delete	(void *);

  Unsigned64		get_total_cputime() const;

private:
  Context * const	_owner;
  unsigned short const	_id;
  unsigned short	_prio;
  unsigned short	_timeslice;
  unsigned short	_ticks_left;
  unsigned long		_total_ticks;
  Cpu_time		_preemption_time;
  unsigned long		_start_cputime;
  unsigned long long	_total_cputime;

  /* Slab allocator for Sched_context slabs */
  static slab_cache_anon * _slabs;

protected:
  Sched_context *	_prev;
  Sched_context *	_next;
};

IMPLEMENTATION:

#include <cassert>
#include "config.h"
#include "cpu.h"
#include "kdb_ke.h"
#include "slab_cache_anon.h"

IMPLEMENT
Sched_context::Sched_context (Context *owner,
                              unsigned short id,
                              unsigned short prio,
                              unsigned short timeslice)
             : _owner (owner),
               _id (id),
               _prio (prio),
               _timeslice (timeslice),
               _ticks_left (timeslice),
               _preemption_time (0),
               _total_cputime (0),
               _prev (this),
               _next (this)
{}

IMPLEMENT inline NEEDS ["kdb_ke.h", "slab_cache_anon.h"]
void *
Sched_context::operator new (size_t)
{
  void *ptr = _slabs->alloc();
  
  if (!ptr)
    kdb_ke ("Sched_context: Out of slab memory!");
        
  return ptr;
}

IMPLEMENT inline
void  
Sched_context::operator delete (void *ptr)
{
  _slabs->free (ptr);
}

PUBLIC inline
Sched_context *
Sched_context::next()
{
  return _next;
}

PUBLIC inline
Sched_context *
Sched_context::prev()
{
  return _prev;
}

PUBLIC inline
void
Sched_context::set_next (Sched_context *next)
{
  _next = next;
}

PUBLIC inline
void
Sched_context::set_prev (Sched_context *prev)
{
  _prev = prev;
}

PUBLIC inline NEEDS [<cassert>]
void
Sched_context::enqueue_before (Sched_context *sibling)
{
  _next = sibling;
  _prev = sibling->prev();

  assert (_prev);

  _prev->set_next (this);
  sibling->set_prev (this);
}

PUBLIC inline
void
Sched_context::dequeue()
{
  _prev->set_next (_next);
  _next->set_prev (_prev);
}

PUBLIC inline
Context *
Sched_context::owner()
{
  return _owner;
}

PUBLIC inline
unsigned short
Sched_context::id()
{
  return _id;
}

PUBLIC inline 
unsigned short 
Sched_context::prio() const
{ 
  return _prio; 
}

PUBLIC inline 
void
Sched_context::set_prio (unsigned short prio)
{ 
  _prio = prio; 
}

PUBLIC inline 
unsigned short 
Sched_context::timeslice() const
{ 
  return _timeslice; 
}

PUBLIC inline 
void
Sched_context::set_timeslice (unsigned short timeslice)
{ 
  _timeslice = timeslice; 
}

PUBLIC inline 
unsigned short 
Sched_context::ticks_left() const
{ 
  return _ticks_left; 
}

PUBLIC inline 
void
Sched_context::set_ticks_left (unsigned short ticks_left)
{ 
  _ticks_left = ticks_left; 
}

PUBLIC inline
Cpu_time
Sched_context::preemption_time()
{
  return _preemption_time;
}

PUBLIC inline
void
Sched_context::set_preemption_time (Cpu_time clock)
{
  _preemption_time = clock;
}

PUBLIC inline
Sched_context *
Sched_context::find_next_preemption()
{
  // This Sched_context has been dealt with already
  set_preemption_time(0);

  // Now find the next Sched_context with a pending preemption event
  for (Sched_context *tmp = next(); tmp != this; tmp = tmp->next())
    if (tmp->preemption_time())
      return tmp;
      
  return 0;
}

PUBLIC inline NEEDS ["config.h"]
void
Sched_context::reset_cputime()
{
  if (Config::fine_grained_cputime)
    _total_cputime = 0;
  else
    _total_ticks = 0;
}

PUBLIC inline
void
Sched_context::update_cputime()
{
  _total_ticks++;
}

PUBLIC inline
void
Sched_context::start_thread(unsigned long tsc_32)
{
  _start_cputime = tsc_32;
}

PUBLIC inline
void
Sched_context::stop_thread(unsigned long tsc_32)
{
  _total_cputime += tsc_32 - _start_cputime;
}

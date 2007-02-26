
/*
 * Timeslice infrastructure
 */

INTERFACE:

#include "types.h"

class Context;
class slab_cache_anon;

class Sched_context
{
  friend class Jdb_list_timeouts;

public:
  /**
   * Definition of different preemption types
   */
  enum Preemption_type {
    None,
    Timeslice_overrun,
    Deadline_miss
  };

private:
  Context * const	_owner;
  unsigned short const	_id;
  unsigned short	_prio;
  Unsigned64		_quantum;
  Unsigned64		_left;
  Preemption_type	_preemption_type;
  Cpu_time		_preemption_time;
  unsigned		_preemption_count;
  Sched_context *	_prev;
  Sched_context *	_next;

  /**
   * Slab allocator for Sched_context slabs
   */
  static slab_cache_anon * _slabs;
  friend void allocator_init();
};

IMPLEMENTATION:

#include <cassert>
#include "cpu_lock.h"
#include "kdb_ke.h"
#include "lock_guard.h"
#include "slab_cache_anon.h"

slab_cache_anon *Sched_context::_slabs;

/**
 * Constructor
 */
PUBLIC
Sched_context::Sched_context (Context * const owner,
                              unsigned short const id,
                              unsigned short prio,
                              Unsigned64 quantum)
             : _owner            (owner),
               _id               (id),
               _prio             (prio),
               _quantum          (quantum),
               _left             (quantum),
               _preemption_type  (None),
               _preemption_time  (0),
               _preemption_count (0),
               _prev             (this),
               _next             (this)
{}

/**
 * Allocator
 */
PUBLIC inline NEEDS ["kdb_ke.h", "slab_cache_anon.h"]
void *
Sched_context::operator new (size_t)
{
  void *ptr = _slabs->alloc();

  if (!ptr)
    kdb_ke ("Sched_context: Out of slab memory!");

  return ptr;
}

/**
 * Deallocator
 */
PUBLIC inline
void
Sched_context::operator delete (void * const ptr)
{
  _slabs->free (ptr);
}

/**
 * Return the next Sched_context in list
 */
PUBLIC inline
Sched_context *
Sched_context::next() const
{
  return _next;
}

/**
 * Return the previous Sched_context in list
 */
PUBLIC inline
Sched_context *
Sched_context::prev() const
{
  return _prev;
}

/**
 * Enqueue a Sched_context in list before sibling
 */
PUBLIC inline NEEDS [<cassert>, "cpu_lock.h", "lock_guard.h"]
void
Sched_context::enqueue_before (Sched_context * const sibling)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  _next = sibling;
  _prev = sibling->prev();

  assert (_prev);

  _prev->_next = sibling->_prev = this;
}

/**
 * Dequeue a Sched_context from list
 */
PUBLIC inline NEEDS ["cpu_lock.h", "lock_guard.h"]
void
Sched_context::dequeue()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  _prev->_next = _next;
  _next->_prev = _prev;
}

/**
 * Return owner of Sched_context
 */
PUBLIC inline
Context *
Sched_context::owner() const
{
  return _owner;
}

/**
 * Return ID of Sched_context
 */
PUBLIC inline
unsigned short
Sched_context::id() const
{
  return _id;
}

/**
 * Return priority of Sched_context
 */
PUBLIC inline
unsigned short
Sched_context::prio() const
{
  return _prio;
}

/**
 * Set priority of Sched_context
 */
PUBLIC inline
void
Sched_context::set_prio (unsigned short const prio)
{
  _prio = prio;
}

/**
 * Return full time quantum of Sched_context
 */
PUBLIC inline
Unsigned64
Sched_context::quantum() const
{
  return _quantum;
}

/**
 * Set full time quantum of Sched_context
 */
PUBLIC inline
void
Sched_context::set_quantum (Unsigned64 const quantum)
{
  _quantum = quantum;
}

/**
 * Return remaining time quantum of Sched_context
 */
PUBLIC inline
Unsigned64
Sched_context::left() const
{
  return _left;
}

/**
 * Set remaining time quantum of Sched_context
 */
PUBLIC inline
void
Sched_context::set_left (Unsigned64 const left)
{
  _left = left;
}

/**
 * Return type of last preemption event on this Sched_context
 */
PUBLIC inline
Sched_context::Preemption_type
Sched_context::preemption_type() const
{
  return _preemption_type;
}

/**
 * Return time of last preemption event on this Sched_context
 */
PUBLIC inline
Cpu_time
Sched_context::preemption_time() const
{
  return _preemption_time;
}

/**
 * Return number of unsent preemption events on this Sched_context
 */
PUBLIC inline
unsigned
Sched_context::preemption_count() const
{
  return _preemption_count;
}

/**
 * Set time of last preemption event on this Sched_context
 */
PUBLIC inline
void
Sched_context::set_preemption_event (Preemption_type const type,
                                     Cpu_time const clock)
{
  _preemption_type = type;
  _preemption_time = clock;
  _preemption_count++;
}

PUBLIC inline
void
Sched_context::clear_preemption_event()
{
  _preemption_type  = None;
  _preemption_count = 0;
}

/**
 * Return next Sched_context with a pending preemption event
 */
PUBLIC inline
Sched_context *
Sched_context::find_next_preemption()
{
  // Clear preemption event for this Sched_context
  clear_preemption_event();

  // Now find the next Sched_context with a pending preemption event
  for (Sched_context *tmp = next(); tmp != this; tmp = tmp->next())
    if (tmp->preemption_type() != None)
      return tmp;

  return 0;
}

IMPLEMENTATION:

#include "cpu_lock.h"
#include "lock_guard.h"

PUBLIC inline
Thread *
Thread::next_present() const
{
  return present_next;
}

/** Is this thread a member of the present list?.
    @return true if thread is in present list
 */
PROTECTED inline
bool 
Thread::in_present_list()
{
  return present_next;
}

/** Enqueue in present list.
    @param sibling present-list member after which we should be enqueued
    @pre sibling->in_present_list() == true
 */
PROTECTED inline NEEDS["cpu_lock.h", "lock_guard.h"]
void
Thread::present_enqueue(Thread *sibling)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (! in_present_list())
    {
      present_next = sibling->present_next;
      present_prev = sibling;
      sibling->present_next = this;
      present_next->present_prev = this;
    }
}

/** Dequeue from present list.
    Remove this thread from present list.
 */
PROTECTED inline NEEDS["cpu_lock.h", "lock_guard.h"]
void
Thread::present_dequeue()
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  if (in_present_list())
    {
      present_prev->present_next = present_next;
      present_next->present_prev = present_prev;
      present_next /* = present_prev */ = 0;
    }
}

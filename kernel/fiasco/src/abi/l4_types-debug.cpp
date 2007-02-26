IMPLEMENTATION [debug]:

#include <cstdio>
#include "simpleio.h"

IMPLEMENTATION [debug]:

PUBLIC Task_num		L4_uid::d_task()		{ return task(); }
PUBLIC void		L4_uid::d_task (Mword num)	{ task (num); }
PUBLIC LThread_num	L4_uid::d_thread()		{ return lthread(); }
PUBLIC void		L4_uid::d_thread (Mword num)	{ lthread (num); }

PUBLIC static inline
Unsigned32
L4_uid::lthread_from_gthread (GThread_num g)
{
  return g % threads_per_task();
}

PUBLIC static inline
Unsigned32
L4_uid::task_from_gthread (GThread_num g)
{
  return g / threads_per_task();
}

PUBLIC void L4_uid::print (int task_format = 0) const
{
  if(is_invalid())
    putstr("---.--");
  else if(is_irq())
    printf("IRQ %02lx",irq());
  else
    printf("%*x.%02x", task_format, task(), lthread());
}

// If compiling w/o JDB, these methods should be available but do nothing.
IMPLEMENTATION [!debug]:

PUBLIC inline void L4_uid::print (int = 0) const {}

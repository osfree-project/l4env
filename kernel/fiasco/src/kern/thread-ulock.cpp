INTERFACE [ulock]:

#include "prio_list.h"

EXTENSION class Thread
{
private:
  Prio_list *_wait_queue;
};


//------------------------------------------------------------------------
IMPLEMENTATION [ulock]:

PUBLIC inline
Prio_list *
Thread::wait_queue() const
{ return _wait_queue; }


PUBLIC inline
void
Thread::wait_queue(Prio_list *wq)
{ _wait_queue = wq; }


PRIVATE inline NEEDS[Thread::wait_queue]
void
Thread::wait_queue_kill()
{
  if (wait_queue())
    sender_dequeue(wait_queue());
}

//------------------------------------------------------------------------
IMPLEMENTATION [!ulock]:

PRIVATE inline
void
Thread::wait_queue_kill()
{}

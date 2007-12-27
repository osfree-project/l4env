INTERFACE:

#include "irq.h"
#include "initcalls.h"
#include "types.h"

class Receiver;

class Dirq : public Irq
{
public:
  static void init() FIASCO_INIT;
  void *operator new (size_t);

private:
  Dirq(Dirq&) : Irq() {}
};


IMPLEMENTATION:

#include "cpu_lock.h"
#include "initcalls.h"
#include "pic.h"
#include "receiver.h"
#include "std_macros.h"
#include "vkey.h"

static char dirq_storage[sizeof(Dirq) * Config::Max_num_dirqs];

IMPLEMENT 
void *Dirq::operator new (size_t) 
{
  static unsigned first = 0;
  assert(first < Config::Max_num_dirqs);
  return reinterpret_cast<Dirq*>(dirq_storage)+(first++);
}

IMPLEMENT FIASCO_INIT
void
Dirq::init()
{
  for( unsigned i = 0; i<Config::Max_num_dirqs; ++i )
    new Dirq(i);
}


PUBLIC inline
explicit
Dirq::Dirq(unsigned irqnum) : Irq(irqnum)
{}


PUBLIC inline NEEDS["pic.h", "cpu_lock.h"]
void
Dirq::mask()
{
  assert (cpu_lock.test());
  unsigned irq = id().irq();
  Pic::disable_locked(irq);
}


PUBLIC inline NEEDS["pic.h", "cpu_lock.h"]
void
Dirq::mask_and_ack()
{
  assert (cpu_lock.test());
  unsigned irq = id().irq();
  Pic::disable_locked(irq);
  Pic::acknowledge_locked(irq);
}


PUBLIC inline NEEDS["pic.h", "receiver.h"]
void
Dirq::unmask()
{
  assert (cpu_lock.test());

  unsigned long prio;

  if (EXPECT_FALSE(!owner()))
    return;
  if (owner() == (Receiver*)-1)
    prio = ~0UL; // highes prio for JDB IRQs
  else
    prio = owner()->sched()->prio();

  Pic::enable_locked(id().irq(), prio);
}


PUBLIC
bool
Dirq::check_debug_irq()
{
  return !Vkey::check_(id().irq());
}





INTERFACE:

#include "sender.h"
#include "irq_alloc.h"

/** Hardware interrupts.  This class encapsulates handware IRQs.  Also,
    it provides a registry that ensures that only one receiver can sign up
    to receive interrupt IPC messages.
 */
class Irq : public Irq_alloc, public Sender
{

 
private:
  Irq();			// default constructors remain undefined
  Irq(Irq&);
 
protected:
  Smword _queued;

public:

  static Irq *lookup( unsigned irq );

};

extern unsigned pic_mask_master;
extern unsigned pic_mask_slave;

extern "C" void fast_ret_from_irq(void);

IMPLEMENTATION:

#include "atomic.h"
#include "globals.h"

IMPLEMENT inline
Irq *Irq::lookup( unsigned irq )
{
  return static_cast<Irq*>(Irq_alloc::lookup(irq));
}


/** Constructor.
    @param irqnum number of hardware interrupt this instance will be bound to
 */
PUBLIC 
explicit 
Irq::Irq(unsigned irqnum)
  : Irq_alloc (irqnum), Sender (Global_id::irq (irqnum)), _queued (0)
{}


/** Consume one interrupt.  
    @return number of IRQs that are still pending.
 */
PRIVATE inline NEEDS ["atomic.h"]
Smword
Irq::consume ()
{
  Smword old;

  do 
    {
      old = _queued;
    }
  while (!cas (&_queued, old, old - 1));

  return old - 1;
}

PUBLIC inline
int
Irq::queued ()
{
  return _queued;
}

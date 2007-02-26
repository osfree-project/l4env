INTERFACE:

#include "sender.h"
#include "irq_alloc.h"

/** Hardware interrupts.  This class encapsulates handware IRQs.  Also,
    it provides a registry that ensures that only one receiver can sign up
    to receive interrupt IPC messages.
 */
class irq_t : public Irq_alloc, public Sender
{

 
private:
  irq_t();			// default constructors remain undefined
  irq_t(irq_t&);
 
protected:
  int _queued;

public:

  static irq_t *lookup( unsigned irq );

};

extern unsigned pic_mask_master;
extern unsigned pic_mask_slave;

extern "C" void fast_ret_from_irq(void);

IMPLEMENTATION:

#include "atomic.h"
#include "globals.h"

IMPLEMENT inline
irq_t *irq_t::lookup( unsigned irq )
{
  return static_cast<irq_t*>(Irq_alloc::lookup(irq));
}


/** Constructor.
    @param irqnum number of hardware interrupt this instance will be bound to
 */
PUBLIC 
explicit 
irq_t::irq_t(unsigned irqnum)
  : Irq_alloc(irqnum), Sender (L4_uid::irq(irqnum)), _queued (0)
{}



/** Consume one interrupt.  
    @return number of IRQs that are still pending.
 */
PRIVATE inline NEEDS ["atomic.h"]
int
irq_t::consume ()
{
  int o;

  do {
    o = _queued;
  } while (! smp_cas(&_queued, o, o - 1));

  return o - 1;
}



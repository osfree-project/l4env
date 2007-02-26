INTERFACE:

#include "irq.h"
#include "types.h"

class Receiver;

class Dirq : public Irq
{
public:

  static void init();
  void *operator new ( size_t );

  explicit Dirq( unsigned irq );
  Receiver *owner() const;
  bool alloc(Receiver *t, bool ack_in_kernel);
  bool free(Receiver *t);

private:
  Dirq();
  Dirq(Dirq&);
};


IMPLEMENTATION:

IMPLEMENT 
Dirq::Dirq(unsigned irqnum) : Irq(irqnum)
{}

IMPLEMENT
Receiver *Dirq::owner() const
{
  return _irq_thread;
}

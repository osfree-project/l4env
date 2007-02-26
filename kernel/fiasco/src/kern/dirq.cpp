INTERFACE:

#include "irq.h"
#include "types.h"

class Receiver;

class dirq_t : public irq_t
{
public:

  static void init();
  void *operator new ( size_t );

  explicit dirq_t( unsigned irq );
  Receiver *owner() const;
  bool alloc(Receiver *t, bool ack_in_kernel);
  bool free(Receiver *t);

private:
  dirq_t();
  dirq_t(dirq_t&);
};


IMPLEMENTATION:

IMPLEMENT 
dirq_t::dirq_t(unsigned irqnum) : irq_t(irqnum)
{}

IMPLEMENT
Receiver *dirq_t::owner() const
{
  return _irq_thread;
}

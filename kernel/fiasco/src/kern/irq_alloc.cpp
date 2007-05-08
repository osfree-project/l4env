INTERFACE:

class Receiver;

class Irq_alloc
{

public:
  virtual bool alloc(Receiver *t) = 0;
  virtual bool free(Receiver *t) = 0;

  static void init();

  Irq_alloc( unsigned irq );

protected:
  Receiver *_irq_thread;

private:
  static Irq_alloc *irqs[];

public:

  static Irq_alloc *lookup( unsigned irq );
  static void register_irq( unsigned irq, Irq_alloc *i );


};

IMPLEMENTATION:

#include "config.h"
#include "initcalls.h"
#include "std_macros.h"

Irq_alloc::Irq_alloc *Irq_alloc::irqs[Config::Max_num_irqs];

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE 
void 
Irq_alloc::init()
{
  for ( unsigned i = 0; i<Config::Max_num_irqs; ++i )
    irqs[i] = 0;
};

PUBLIC inline
Receiver *
Irq_alloc::owner() const
{ return _irq_thread; }

PUBLIC static
void 
Irq_alloc::free_all(Receiver *rcv)
{
  for (unsigned i = 0; i < Config::Max_num_irqs; ++i)
    if (irqs[i] && irqs[i]->owner() == rcv)
      irqs[i]->free(rcv);
}


IMPLEMENT inline NEEDS["config.h"]
Irq_alloc *Irq_alloc::lookup( unsigned irq )
{
  if(irq < Config::Max_num_irqs)
    return irqs[irq];
  else
    return 0;
}

IMPLEMENT inline NEEDS["config.h"]
void Irq_alloc::register_irq( unsigned irq, Irq_alloc *i )
{
  if(irq < Config::Max_num_irqs)
    irqs[irq] = i;
}

IMPLEMENT inline
Irq_alloc::Irq_alloc(unsigned irqnum)
  : _irq_thread (0)
{
  register_irq(irqnum, this);
}


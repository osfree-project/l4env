INTERFACE:

class Receiver;

class Irq_alloc
{

public:
  virtual bool alloc(Receiver *t, bool ack_in_kernel) = 0;
  virtual bool free(Receiver *t) = 0;
  virtual Receiver *owner() const = 0;

  static void init();

  Irq_alloc( unsigned irq );

protected:
  Receiver *_irq_thread;
  bool _ack_in_kernel;

private:
  static Irq_alloc *irqs[];

public:

  static Irq_alloc *lookup( unsigned irq );
  static void register_irq( unsigned irq, Irq_alloc *i );


};

IMPLEMENTATION:

#include "config.h"
#include "initcalls.h"

Irq_alloc::Irq_alloc *Irq_alloc::irqs[Config::MAX_NUM_IRQ];

IMPLEMENT FIASCO_INIT 
void Irq_alloc::init()
{
  for ( unsigned i = 0; i<Config::MAX_NUM_IRQ; ++i )
    irqs[i] = 0;
};


IMPLEMENT inline NEEDS["config.h"]
Irq_alloc *Irq_alloc::lookup( unsigned irq )
{
  if(irq < Config::MAX_NUM_IRQ) 
    return irqs[irq];    
  else
    return 0;
}

IMPLEMENT inline NEEDS["config.h"]
void Irq_alloc::register_irq( unsigned irq, Irq_alloc *i )
{
  if(irq < Config::MAX_NUM_IRQ) {
    irqs[irq] = i;
  }
}

IMPLEMENT inline
Irq_alloc::Irq_alloc(unsigned irqnum)
  : _irq_thread (0)
{
  register_irq(irqnum, this);
}


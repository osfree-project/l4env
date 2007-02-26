#ifdef CPUTYPE_pxa
#include <l4/arm_drivers/uart.h>

namespace Proc
{
  typedef unsigned Status;
  
  inline Status cli_save()
  { return 0; }

  inline void sti_restore(Status)
  {}

};

Uart::Uart()
 : port((unsigned)-1), _irq(-1)
{}

Uart::~Uart()
{}

bool Uart::valid()
{
  char scratch, scratch2, scratch3;

  scratch = ier();
  ier(0x00);

  scratch2 = ier();
  ier(0x0f);

  scratch3 = ier();
  ier(scratch);

  return (scratch2 == 0x00 && scratch3 == 0x0f);
}


bool Uart::startup(l4_addr_t _port, int __irq)
{
  port = _port;
  _irq  = __irq;

  if (!valid())
    return false;

  Proc::Status o = Proc::cli_save();
  ier(Base_ier_bits);/* disable all rs-232 interrupts */
  mcr(0x0b);         /* out2, rts, and dtr enabled */
  fcr(1);            /* enable fifo */
  fcr(0x07);         /* clear rcv xmit fifo */
  fcr(1);            /* enable fifo */
  lcr(0);            /* clear line control register */

  /* clearall interrupts */
  /*read*/ msr(); /* IRQID 0*/
  /*read*/ iir(); /* IRQID 1*/
  /*read*/ trb(); /* IRQID 2*/
  /*read*/ lsr(); /* IRQID 3*/

  while(lsr() & 1/*DATA READY*/)
    /*read*/ trb();
  Proc::sti_restore(o);
  return true;
}


void Uart::shutdown()
{
  Proc::Status o = Proc::cli_save();
  mcr( 0x06 );
  fcr( 0 );
  lcr( 0 );
  ier( 0 );
  Proc::sti_restore(o);
}
  
bool Uart::change_mode(TransferMode m, BaudRate r)
{
  Proc::Status o = Proc::cli_save();
  char old_lcr = lcr();
  if(r != BAUD_NC) {
    lcr(old_lcr | 0x80/*DLAB*/);
    l4_uint16_t divisor = Base_rate/r;
    trb( divisor & 0x0ff );        /* BRD_LOW  */
    ier( (divisor >> 8) & 0x0ff ); /* BRD_HIGH */
    lcr(old_lcr);
  }
  if( m != MODE_NC ) {
    lcr( m & 0x07f );
  }

  Proc::sti_restore(o);
  return true;
}

Uart::TransferMode Uart::get_mode()
{
  return lcr() & 0x7f;
}

int Uart::write( char const *s, unsigned count )
{
  /* disable uart irqs */
  char old_ier;
  old_ier = ier();
  ier(old_ier & ~0x0f);

  /* transmission */
  for(unsigned i =0; i<count; i++) {
    while(!(lsr() & 0x20 /* THRE */)) {}
    if (s[i] == '\346')
      trb('\265');
    else
      trb(s[i]);
    if( s[i]=='\n' ) {
      while(!(lsr() &0x20 /* THRE */)) {}
      trb('\r');
    }	  
  }

  /* wait till everything is transmitted */
  while(!(lsr() & 0x40 /* TSRE */)) {}


  ier(old_ier);
  return 1;

}

int Uart::getchar( bool blocking )
{
  if(!blocking && !(lsr() & 1 /* DATA READY */))
    return -1;

  char old_ier, ch;
  old_ier = ier();
  ier(old_ier & ~0x0f);
  while(!(lsr() & 1 /* DATA READY */));
  ch = trb();
  ier(old_ier);
  return ch;
}

int Uart::char_avail() const
{
  if(lsr() & 1 /* DATA READY */)
    {
      return 1;
    }
  else 
    return 0;
}

#endif

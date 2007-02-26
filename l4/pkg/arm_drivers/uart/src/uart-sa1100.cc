#ifdef CPUTYPE_sa

#include <l4/arm_drivers/uart.h>

namespace Proc
{
  typedef l4_umword_t Status;
  
  inline
  Status cli_save()
  {
    Status ret;
    asm volatile ( "    mrs    r6, cpsr    \n"
		   "    mov    %0, r6      \n"
		   "    orr    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
		   : "=r"(ret) : : "r6" 
		   );
    return ret;
  }

  inline
  void sti_restore( Status st )
  {
    asm volatile ( "    tst    %0, #128    \n"
		   "    bne    1f          \n"
                   "    mrs    r6, cpsr    \n"
		   "    bic    r6,r6,#128  \n"
		   "    msr    cpsr_c, r6  \n"
                   "1:                     \n"
		   : : "r"(st) : "r6" 
		   );
  }

};

Uart::Uart()
{
  address = (unsigned)-1;
  _irq = (unsigned)-1;
}

Uart::~Uart()
{
  utcr3(0);
}


bool Uart::startup( l4_addr_t _address, unsigned irq ) 

{
  address =_address;
  _irq = irq;
  utsr0((unsigned)-1); //clear pending status bits
  utcr3(UTCR3_RXE | UTCR3_TXE); //enable transmitter and receiver
  return true;
}

void Uart::shutdown()
{
  utcr3(0);
}


bool Uart::change_mode(TransferMode m, BaudRate baud)
{
  unsigned old_utcr3, quot;
  Proc::Status st;
  if(baud == (BaudRate)-1) 
    return false;
  if(baud != BAUD_NC && (baud>115200 || baud<96)) 
    return false;
  if(m == (TransferMode)-1)
    return false;

  st = Proc::cli_save();
  old_utcr3 = utcr3();
  utcr3(old_utcr3 & ~(UTCR3_RIE|UTCR3_TIE));
  Proc::sti_restore(st);

  while(utsr1() & UTSR1_TBY);

  /* disable all */
  utcr3(0);

  /* set parity, data size, and stop bits */
  if(m!=MODE_NC)
    utcr0(m & 0x0ff); 

  /* set baud rate */
  if(baud!=BAUD_NC) 
    {
      quot = (UARTCLK / (16*baud)) -1;
      utcr1((quot & 0xf00) >> 8);
      utcr2(quot & 0x0ff);
    }

  utsr0((unsigned)-1);

  utcr3(old_utcr3);
  return true;
}


int Uart::write( const char *s, unsigned count )
{
  unsigned old_utcr3;
  Proc::Status st;
  st = Proc::cli_save();
  old_utcr3 = utcr3();
  utcr3( (old_utcr3 & ~(UTCR3_RIE|UTCR3_TIE)) | UTCR3_TXE );

  /* transmission */
  for(unsigned i =0; i<count; i++) 
    {
      while(!(utsr1() & UTSR1_TNF)) ;
      utdr(s[i]);
      if( s[i]=='\n' ) 
	{
	  while(!(utsr1() & UTSR1_TNF)) ;
	  utdr('\r');
	}	  
    }

  /* wait till everything is transmitted */
  while(utsr1() & UTSR1_TBY) ;

  utcr3(old_utcr3);
  Proc::sti_restore(st);
  return 1;
}

int Uart::getchar( bool blocking )
{
  unsigned old_utcr3, ch;

  old_utcr3 = utcr3();
  utcr3( old_utcr3 & ~(UTCR3_RIE|UTCR3_TIE) );
  while(!(utsr1() & UTSR1_RNE))
    if(!blocking)
      return -1;

  ch = utdr();
  utcr3(old_utcr3);
  return ch;
}


int Uart::char_avail() const
{
  if((utsr1() & UTSR1_RNE))
    {
      return 1;
    }
  else 
    return 0;
}
#endif

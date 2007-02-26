// The following stuff is included within the class Uart definition

#if !defined(IN_CLASS_UART__)
# error This file must be included only from uart.h
#endif

public:

  bool startup(l4_addr_t address, unsigned irq);

  enum {
    PAR_NONE = 0x00,
    PAR_EVEN = 0x03,
    PAR_ODD  = 0x01,
    DAT_5    = (unsigned)-1,
    DAT_6    = (unsigned)-1,
    DAT_7    = 0x00,
    DAT_8    = 0x08,
    STOP_1   = 0x00,
    STOP_2   = 0x04,

    MODE_8N1 = PAR_NONE | DAT_8 | STOP_1,
    MODE_7E1 = PAR_EVEN | DAT_7 | STOP_1,

  // these two values are to leave either mode
  // or baud rate unchanged on a call to change_mode
    MODE_NC  = 0x1000000,
    BAUD_NC  = 0x1000000,

  };

private:
  l4_addr_t address;
  unsigned _irq;
  enum {
    UTCR0_PE  = 0x01,
    UTCR0_OES = 0x02,
    UTCR0_SBS = 0x04,
    UTCR0_DSS = 0x08,
    UTCR0_SCE = 0x10,
    UTCR0_RCE = 0x20,
    UTCR0_TCE = 0x40,

    UTCR3_RXE = 0x01,
    UTCR3_TXE = 0x02,
    UTCR3_BRK = 0x04,
    UTCR3_RIE = 0x08,
    UTCR3_TIE = 0x10,
    UTCR3_LBM = 0x20,


    UTSR0_TFS = 0x01,
    UTSR0_RFS = 0x02,
    UTSR0_RID = 0x04,
    UTSR0_RBB = 0x08,
    UTSR0_REB = 0x10,
    UTSR0_EIF = 0x20,
    
    UTSR1_TBY = 0x01,
    UTSR1_RNE = 0x02,
    UTSR1_TNF = 0x04,
    UTSR1_PRE = 0x08,
    UTSR1_FRE = 0x10,
    UTSR1_ROR = 0x20,
    
    UARTCLK = 3686400,
  };


inline static l4_addr_t _utcr0( l4_addr_t a ) 
{ return a; }

inline static l4_addr_t _utcr1( l4_addr_t a ) 
{ return (a+0x04); }

inline static l4_addr_t _utcr2( l4_addr_t a ) 
{ return (a+0x08); }

inline static l4_addr_t _utcr3( l4_addr_t a ) 
{ return (a+0x0c); }

inline static l4_addr_t _utcr4( l4_addr_t a ) 
{ return (a+0x10); }

inline static l4_addr_t _utdr( l4_addr_t a ) 
{ return (a+0x14); }

inline static l4_addr_t _utsr0( l4_addr_t a )
{ return (a+0x1c); }

inline static l4_addr_t _utsr1( l4_addr_t a )
{ return (a+0x20); }


inline 
unsigned utcr0()  const 
{ return *((volatile unsigned*)(_utcr0(address))); }

inline 
unsigned utcr1()  const
{ return *((volatile unsigned*)(_utcr1(address))); }

inline 
unsigned utcr2()  const
{ return *((volatile unsigned*)(_utcr2(address))); }

inline 
unsigned utcr3()  const
{ return *((volatile unsigned*)(_utcr3(address))); }

inline 
unsigned utcr4()  const
{ return *((volatile unsigned*)(_utcr4(address))); }

inline 
unsigned utdr()  const
{ return *((volatile unsigned*)(_utdr(address))); }

inline 
unsigned utsr0()  const
{ return *((volatile unsigned*)(_utsr0(address))); }

inline 
unsigned utsr1()  const
{ return *((volatile unsigned*)(_utsr1(address))); }


inline 
void utcr0(unsigned v) 
{ *((volatile unsigned*)(_utcr0(address)))= v; }

inline 
void utcr1(unsigned v) 
{ *((volatile unsigned*)(_utcr1(address)))= v; }

inline 
void utcr2(unsigned v) 
{ *((volatile unsigned*)(_utcr2(address)))= v; }

inline 
void utcr3(unsigned v) 
{ *((volatile unsigned*)(_utcr3(address)))= v; }

inline 
void utcr4(unsigned v) 
{ *((volatile unsigned*)(_utcr4(address)))= v; }

inline 
void utdr(unsigned v) 
{ *((volatile unsigned*)(_utdr(address)))= v; }

inline 
void utsr0(unsigned v) 
{ *((volatile unsigned*)(_utsr0(address)))= v; }

inline 
void utsr1(unsigned v) 
{ *((volatile unsigned*)(_utsr1(address)))= v; }

inline
bool tx_empty()
{
  return !(utsr1() & UTSR1_TBY);
}

#if !defined(IN_CLASS_UART__)
# error This fime must be included only from uart.h
#endif

public:
  enum {
    Base_rate     = 921600,
    Base_ier_bits = 1 << 6,
  };

  /**
   * Start this serial port for I/O.
   * @param port the I/O port base address.
   * @param irq the IRQ assigned to this port, -1 if none.
   */
  bool startup(unsigned port, int irq);

  enum {
    PAR_NONE = 0x00,
    PAR_EVEN = 0x18,
    PAR_ODD  = 0x08,
    DAT_5    = 0x00,
    DAT_6    = 0x01,
    DAT_7    = 0x02,
    DAT_8    = 0x03,
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

  enum Registers {
    TRB      = 0, // Transmit/Receive Buffer  (read/write)
    BRD_LOW  = 0, // Baud Rate Divisor LSB if bit 7 of LCR is set  (read/write)
    IER      = 1, // Interrupt Enable Register  (read/write)
    BRD_HIGH = 1, // Baud Rate Divisor MSB if bit 7 of LCR is set  (read/write)
    IIR      = 2, // Interrupt Identification Register  (read only)
    FCR      = 2, // 16550 FIFO Control Register  (write only)
    LCR      = 3, // Line Control Register  (read/write)
    MCR      = 4, // Modem Control Register  (read/write)
    LSR      = 5, // Line Status Register  (read only)
    MSR      = 6, // Modem Status Register  (read only)
    SPR      = 7, // Scratch Pad Register  (read/write)
  };

  unsigned port;
  int _irq;

  bool valid();
  
inline 
void outb( char b, Registers reg )
{
  *(volatile char *)((port+reg)*4) = b;
}

inline 
char inb( Registers reg ) const
{
  return *(volatile char *)((port+reg)*4);
}


inline 
void mcr( char b )
{
  outb(b, MCR);
}

inline 
char mcr() const
{
  return inb(MCR);
}

inline 
void fcr( char b )
{
  outb(b, FCR);
}

inline 
void lcr( char b )
{
  outb(b, LCR);
}

inline 
char lcr() const
{
  return inb(LCR);
}

inline 
void ier( char b )
{
  outb(b, IER);
}

inline 
char ier() const
{
  return inb(IER);
}

inline 
char iir() const
{
  return inb(IIR);
}

inline 
char msr() const
{
  return inb(MSR);
}

inline 
char lsr() const
{
  return inb(LSR);
}

inline 
void trb( char b )
{
  outb(b, TRB);
}

inline 
char trb() const
{
  return inb(TRB);
}




#include <l4/sys/l4int.h>

/**
 * Platform independent UART stub.
 */
class Uart 
{
public:
  /**
   * Type UART transfer mode (Bits, Stopbits etc.).
   */
  typedef unsigned TransferMode;

  /**
   * Type for baud rate.
   */
  typedef unsigned BaudRate;
# define IN_CLASS_UART__
# if defined(CPUTYPE_sa)
#   include "uart-sa1100.h"
# elif defined(CPUTYPE_pxa)
#   include "uart-pxa.h"
# else
#   error Unknown CPUTYPE
# endif
# undef IN_CLASS_UART__
  
  /* These constants must be defined in the 
     arch part of the uart. To define them there
     has the advantage of most efficent definition
     for the hardware. 

  static unsigned const PAR_NONE = xxx;
  static unsigned const PAR_EVEN = xxx;
  static unsigned const PAR_ODD  = xxx;
  static unsigned const DAT_5    = xxx;
  static unsigned const DAT_6    = xxx;
  static unsigned const DAT_7    = xxx;
  static unsigned const DAT_8    = xxx;
  static unsigned const STOP_1   = xxx;
  static unsigned const STOP_2   = xxx;

  static unsigned const MODE_8N1 = PAR_NONE | DAT_8 | STOP_1;
  static unsigned const MODE_7E1 = PAR_EVEN | DAT_7 | STOP_1;

  // these two values are to leave either mode
  // or baud rate unchanged on a call to change_mode
  static unsigned const MODE_NC  = xxx;
  static unsigned const BAUD_NC  = xxx;

  */


public:
  /* Interface definition - implemented in the arch part */
  /// ctor
  Uart();

  /// dtor
  ~Uart();

  /**
   * (abstract) Shutdown the serial port.
   */
  void shutdown();

  /**
   * (abstract) Get the IRQ assigned to the port.
   */
  int const irq() const;

  /**
   * (abstract) Enable rcv IRQ in UART.
   */
  void enable_rcv_irq();

  /**
   * (abstract) Disable rcv IRQ in UART.
   */
  void disable_rcv_irq();
  
  /**
   * (abstract) Change transfer mode or speed.
   * @param m the new mode for the transfer, or MODE_NC for no mode change.
   * @param r the new baud rate, or BAUD_NC, for no speed change.
   */
  bool change_mode(TransferMode m, BaudRate r);

  /**
   * (abstract) Get the current transfer mode.
   */
  TransferMode get_mode();

  /**
   * (abstract) Write str.
   */
  int write( char const *str, unsigned len );

  /**
   * (abstract) Read a character.
   */
  int getchar( bool blocking = true );

  /**
   * (abstract) Is there anything to read?
   */
  int char_avail() const;
  
};

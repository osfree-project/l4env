INTERFACE:

#include "console.h"

/**
 * @brief Platform independent UART stub.
 */
class Uart 
  : public Console
{
public:
  /**
   * @brief Type UART transfer mode (Bits, Stopbits etc.).
   */
  typedef unsigned TransferMode;

  /**
   * @brief Type for baud rate.
   */
  typedef unsigned BaudRate;

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
   * @brief (abstract) Shutdown the serial port.
   */
  void shutdown();

  /**
   * @brief (abstract) Get the IRQ assigned to the port.
   */
  int const irq() const;

  /**
   * @brief (abstract) Enable rcv IRQ in UART.
   */
  void enable_rcv_irq();

  /**
   * @brief (abstract) Disable rcv IRQ in UART.
   */
  void disable_rcv_irq();
  
  /**
   * @brief (abstract) Change transfer mode or speed.
   * @param m the new mode for the transfer, or MODE_NC for no mode change.
   * @param r the new baud rate, or BAUD_NC, for no speed change.
   */
  bool change_mode(TransferMode m, BaudRate r);

  /**
   * @brief (abstract) Get the current transfer mode.
   */
  TransferMode get_mode();

  /**
   * @brief (abstract) Write str.
   */
  int write( char const *str, size_t len );

  /**
   * @brief (abstract) Read a character.
   */
  int getchar( bool blocking = true );

  /**
   * @brief (abstract) Is there anything to read?
   */
  int char_avail() const;
  
  char const *next_attribute( bool restart = false ) const;
  
};


IMPLEMENTATION:

IMPLEMENT
char const *Uart::next_attribute( bool restart ) const
{
  static unsigned current = 0;
  static char const *attribs[] = { "uart", "in", "out", 0 };
  if(restart)
    current = 0;
  if(current>=4)
    return 0;
  else
    return attribs[current++];
}



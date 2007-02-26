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
   * @brief Shutdown the serial port.
   */
  void shutdown();

  /**
   * @brief Get the IRQ assigned to the port.
   */
  int const irq() const;

  /**
   * @brief Enable rcv IRQ in UART.
   */
  void enable_rcv_irq();

  /**
   * @brief Disable rcv IRQ in UART.
   */
  void disable_rcv_irq();
  
  /**
   * @brief Change transfer mode or speed.
   * @param m the new mode for the transfer, or MODE_NC for no mode change.
   * @param r the new baud rate, or BAUD_NC, for no speed change.
   */
  bool change_mode(TransferMode m, BaudRate r);

  /**
   * @brief Get the current transfer mode.
   */
  TransferMode get_mode();

  int write( char const *str, size_t len );
  int getchar( bool blocking = true );
  int char_avail() const;

  
};


IMPLEMENTATION:

//-


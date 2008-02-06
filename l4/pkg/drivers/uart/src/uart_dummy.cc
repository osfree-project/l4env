#include "uart_dummy.h"

namespace L4
{
  bool Uart_dummy::startup(unsigned long base)
  { return true; }

  void Uart_dummy::shutdown()
  {}

  bool Uart_dummy::enable_rx_irq(bool enable) 
  { return true; }
  bool Uart_dummy::enable_tx_irq(bool /*enable*/) { return false; }
  bool Uart_dummy::change_mode(Transfer_mode, Baud_rate r)
  { return true; }

  int Uart_dummy::get_char(bool blocking) const
  { return 0; }

  int Uart_dummy::char_avail() const 
  { return false; }

  void Uart_dummy::out_char(char c) const
  { (void)c; }

  int Uart_dummy::write(char const *s, unsigned long count) const
  { (void)s; (void)count; return 0; }
};

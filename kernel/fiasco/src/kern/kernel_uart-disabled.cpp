INTERFACE:

/**
 * @brief Glue between kernel and UART driver.
 */
class Kernel_uart
{
public:
  Kernel_uart();

  static inline void enable_rcv_irq();
};


IMPLEMENTATION[disabled]:

IMPLEMENT
Kernel_uart::Kernel_uart()
{
}

IMPLEMENT inline void
Kernel_uart::enable_rcv_irq()
{
}


/* ARM specific */

INTERFACE:

EXTENSION class Config
{
public:

  enum {
    PAGE_SHIFT = 12,
    PAGE_SIZE  = 1 << PAGE_SHIFT,
    PAGE_MASK  = ~((1 << PAGE_SHIFT) -1),
  
    SUPERPAGE_SIZE = 1024*1024,
    SUPERPAGE_MASK = ~(SUPERPAGE_SIZE -1),

    backward_compatibility = 0,
    hlt_works_ok = 0,

    microsec_per_tick = 10000LL,
    MAX_NUM_IRQ = 64,
  };

  // the default uart to use for serial console
  static unsigned const default_console_uart = 3;
  static unsigned const default_console_uart_baudrate = 115200;

  static const bool getchar_does_hlt = false;

#if (CONFIG_VMEM_ALLOC_TEST,1)
  static bool const VMEM_ALLOC_TEST = true;
#else
  static bool const VMEM_ALLOC_TEST = false;
#endif
  
};


IMPLEMENTATION[arch]:
//-

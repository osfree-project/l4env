/* ARM specific */
INTERFACE [arm]:

EXTENSION class Config
{
public:

  enum {
    PAGE_SHIFT = 12,
    PAGE_SIZE  = 1 << PAGE_SHIFT,
    PAGE_MASK  = ~(PAGE_SIZE - 1),

    SUPERPAGE_SHIFT = 20,
    SUPERPAGE_SIZE  = 1 << SUPERPAGE_SHIFT,
    SUPERPAGE_MASK  = ~(SUPERPAGE_SIZE -1),

    backward_compatibility = 1,
    hlt_works_ok = 1,
    Irq_shortcut = 1,
  };

  enum 
  {
#ifdef CONFIG_ONE_SHOT
    scheduler_one_shot		= 1,
    scheduler_granularity	= 1UL,
    default_time_slice	        = 10000 * scheduler_granularity,
#else
    scheduler_one_shot		= 0,
    scheduler_granularity	= 1000UL,
    default_time_slice	        = 10 * scheduler_granularity,
#endif
  };

  // the default uart to use for serial console
  static unsigned const default_console_uart	= 3;
  static unsigned const default_console_uart_baudrate = 115200;

  static const bool getchar_does_hlt = false;
  static const bool getchar_does_hlt_works_ok = true;
  static const char char_micro;
  static const bool enable_io_protection = false;

#if (CONFIG_VMEM_ALLOC_TEST,1)
  static bool const VMEM_ALLOC_TEST = true;
#else
  static bool const VMEM_ALLOC_TEST = false;
#endif
  
};

INTERFACE [arm && (pxa || sa1100)]:

EXTENSION class Config
{
public:
  enum
  {
    Scheduling_irq       = 26,
    scheduler_irq_vector = Scheduling_irq,
    Max_num_irqs         = 64,
    Max_num_dirqs        = 32,

    Vkey_irq             = 27,
  };
};

INTERFACE [arm && integrator]:

EXTENSION class Config
{
public:
  enum
  {
    Scheduling_irq       = 6,
    scheduler_irq_vector = Scheduling_irq,
    Max_num_irqs         = 49,
    Max_num_dirqs        = 48,

    Vkey_irq             = 48,
  };
};

INTERFACE [arm && realview && !mpcore]:

EXTENSION class Config
{
public:
  enum
  {
    Scheduling_irq       = 36,
    scheduler_irq_vector = Scheduling_irq,
    Max_num_irqs         = 97,
    Max_num_dirqs        = 96,

    Vkey_irq             = 96,
  };
};

INTERFACE [arm && realview && mpcore]:

EXTENSION class Config
{
public:
  enum
  {
    Scheduling_irq       = 33,
    scheduler_irq_vector = Scheduling_irq,
    Max_num_irqs         = 65,
    Max_num_dirqs        = 64,

    Vkey_irq             = 64,
  };
};

INTERFACE[arm && pxa]:
#define TARGET_NAME "XScale"

INTERFACE[arm && sa1100]:
#define TARGET_NAME "StrongARM"

INTERFACE[arm && integrator]:
#define TARGET_NAME "Integrator"

INTERFACE[arm && realview]:
#define TARGET_NAME "Realview"

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

char const Config::char_micro = '\346';
const char *const Config::kernel_warn_config_string = 0;

IMPLEMENT FIASCO_INIT
void Config::init()
{
  serial_esc = SERIAL_ESC_IRQ;
}


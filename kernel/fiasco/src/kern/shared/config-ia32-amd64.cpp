/* IA32/AMD64 specific */
INTERFACE[ia32,amd64]:

EXTENSION class Config
{
public:

  enum
  {
    Tbuf_irq		= 16,
    Vkey_irq		= 17,
  };

  static const bool backward_compatibility	= true;

#ifdef CONFIG_IO_PROT
  static const bool enable_io_protection	= true;
#else
  static const bool enable_io_protection	= false;
#endif

#ifdef CONFIG_SCHED_PIT
  static const unsigned scheduler_mode		= SCHED_PIT;
  static const unsigned scheduler_irq_vector	= 0x20U;
  static const unsigned scheduler_granularity	= 1000U;
  static const unsigned default_time_slice	= 10 * scheduler_granularity;
#endif

#ifdef CONFIG_SCHED_RTC
  static const unsigned scheduler_mode		= SCHED_RTC;
  static const unsigned scheduler_irq_vector	= 0x28U;
#  ifdef CONFIG_SLOW_RTC
  static const unsigned scheduler_granularity	= 15625U;
  static const unsigned default_time_slice	= 10 * scheduler_granularity;
#  else
  static const unsigned scheduler_granularity	= 976U;
  static const unsigned default_time_slice	= 10 * scheduler_granularity;
#  endif
#endif

#ifdef CONFIG_ONE_SHOT
  static const bool scheduler_one_shot          = true;
#else
  static const bool scheduler_one_shot          = false;
#endif

#ifdef CONFIG_SCHED_APIC
  static const unsigned scheduler_mode		= SCHED_APIC;
  static const unsigned scheduler_irq_vector	= 0x3dU;
#  ifdef CONFIG_ONE_SHOT
  static const unsigned scheduler_granularity	= 1U;
  static const unsigned default_time_slice	= 10000 * scheduler_granularity;
#  else
  static const unsigned scheduler_granularity	= 1000U;
  static const unsigned default_time_slice	= 10 * scheduler_granularity;
#  endif
#endif

#ifdef CONFIG_POWERSAVE_GETCHAR
  static const bool getchar_does_hlt = true;
#else
  static const bool getchar_does_hlt = false;
#endif

  static bool getchar_does_hlt_works_ok;
  static bool apic;

#ifdef CONFIG_WATCHDOG
  static bool watchdog;
#else
  static const bool watchdog = false;
#endif

  static const bool hlt_works_ok = true;
  static const bool pic_prio_modify = true;
#ifdef CONFIG_SYNC_TSC
  static const bool kinfo_timer_uses_rdtsc = true;
#else
  static const bool kinfo_timer_uses_rdtsc = false;
#endif

  static const bool old_sigma0_adapter_hack = false;

  // the default uart to use for serial console
  static const unsigned default_console_uart = 1;
  static const unsigned default_console_uart_baudrate = 115200;

  static char const char_micro;

  static bool found_vmware;
};

IMPLEMENTATION[ia32,amd64]:

#include <cstring>
#include "cmdline.h"

bool Config::found_vmware = false;
char const Config::char_micro = '\346';
bool Config::apic = false;
bool Config::getchar_does_hlt_works_ok = false;

#ifdef CONFIG_WATCHDOG
bool Config::watchdog = false;
#endif

const char *const Config::kernel_warn_config_string =
#ifdef CONFIG_SCHED_RTC
  "  CONFIG_SCHED_RTC is on\n"
#endif
#ifndef CONFIG_ASSEMBLER_IPC_SHORTCUT
  "  CONFIG_ASSEMBLER_IPC_SHORTCUT is off\n"
#endif
#ifndef CONFIG_INLINE
  "  CONFIG_INLINE is off\n"
#endif
#ifndef CONFIG_NDEBUG
  "  CONFIG_NDEBUG is off\n"
#endif
#ifdef CONFIG_PROFILE
  "  CONFIG_PROFILE is on\n"
#endif
#ifndef CONFIG_NO_FRAME_PTR
  "  CONFIG_NO_FRAME_PTR is off\n"
#endif
#ifdef CONFIG_LIST_ALLOC_SANITY
  "  CONFIG_LIST_ALLOC_SANITY is on\n"
#endif
#ifdef CONFIG_BEFORE_IRET_SANITY
  "  CONFIG_BEFORE_IRET_SANITY is on\n"
#endif
#ifdef CONFIG_FINE_GRAINED_CPUTIME
  "  CONFIG_FINE_GRAINED_CPUTIME is on\n"
#endif
#ifdef CONFIG_JDB_ACCOUNTING
  "  CONFIG_JDB_ACCOUNTING is on\n"
#endif
  "";

IMPLEMENT FIASCO_INIT
void
Config::init_arch()
{
  char const *cmdline = Cmdline::cmdline();

#ifdef CONFIG_WATCHDOG
  if (strstr(cmdline, " -watchdog"))
    {
      watchdog = true;
      apic = true;
    }
#endif
  
  if (strstr(cmdline, " -apic"))
    apic = true;

  if (scheduler_mode == SCHED_APIC)
    apic = true;
}

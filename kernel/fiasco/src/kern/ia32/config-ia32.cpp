/* IA32 specific */
INTERFACE:

EXTENSION class Config
{
public:

  enum {
    TBUF_IRQ = 16,
  };

  static const bool backward_compatibility = true;

#ifdef CONFIG_SCHED_PIT
  static const bool scheduling_using_pit = true;
#else
  static const bool scheduling_using_pit = false;
#endif

#ifndef BOCHS
  static const bool hlt_works_ok = true;
#ifdef CONFIG_POWERSAVE_GETCHAR
  static const bool getchar_does_hlt = true;
#else
  static const bool getchar_does_hlt = false;
#endif
  static const bool pic_prio_modify = true;
#else
  static bool hlt_works_ok;
  static bool getchar_does_hlt;
  static bool pic_prio_modify;
#endif /* BOCHS */

  static const bool enable_io_protection = false;

#ifdef CONFIG_SCHED_PIT
  static const unsigned microsec_per_tick = 1000LL;
#else
# ifdef CONFIG_SLOW_RTC
  static const unsigned microsec_per_tick = 15625LL;
# else
  static const unsigned microsec_per_tick = 976LL;
# endif
#endif
#ifdef CONFIG_SYNC_TSC
  static const bool kinfo_timer_uses_rdtsc = true;
#else
  static const bool kinfo_timer_uses_rdtsc = false;
#endif

  // the default uart to use for serial console
  static unsigned const default_console_uart = 2;
  static unsigned const default_console_uart_baudrate = 115200;

  static bool found_vmware;
  
};

IMPLEMENTATION[ia32]:

bool Config::found_vmware = false;
//-

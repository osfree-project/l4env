/*
 * Global kernel configuration
 */

INTERFACE:

#include <globalconfig.h>
#include "config_tcbsize.h"
#include "l4_types.h"

#include "undef_oskit.h"

// special magic to allow old compilers to inline constants

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#if defined(__GNUC__)
# if defined(__GNUC_PATCHLEVEL__)
#  define COMPILER STRINGIFY(__GNUC__) "." STRINGIFY(__GNUC_MINOR__) "." STRINGIFY(__GNUC_PATCHLEVEL__)
# else
#  define COMPILER STRINGIFY(__GNUC__) "." STRINGIFY(__GNUC_MINOR__)
# endif
#else
# define COMPILER "Non-GCC"
#endif

#define GREETING_COLOR_ANSI_TITLE  "\033[1;32m"
#define GREETING_COLOR_ANSI_INFO   "\033[0;32m"
#define GREETING_COLOR_ANSI_OFF    "\033[0m"

class Config
{
public:

  static const char *const kernel_version_string;

  enum {
    MAX_NUM_IRQ = 20,
    SERIAL_ESC_IRQ = 2,
    SERIAL_ESC_NOIRQ = 1,
    SERIAL_NO_ESC = 0,
  };


  static void init();

  // global kernel configuration
  static const unsigned kernel_version_id = 0x01004444; // "DD\000\001"

  /* These constants must be defined in the arch part.
     
  // the default uart to use for serial console
  static unsigned const default_console_uart = xxx;
  static unsigned const default_console_uart_baudrate = 115200;

  */

 
  static const unsigned kernel_mem_per_cent = 10;

  static const unsigned thread_block_size = THREAD_BLOCK_SIZE;

  static const unsigned thread_block_mask 
    = (thread_block_size - 1) | ~((thread_block_size << (11+7)) - 1);
  static const unsigned thread_block_id_factor = thread_block_size/0x400;
  


  static const bool conservative = false;
  static const bool monitor_page_faults = conservative;


#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
  static const bool deceit_bit_disables_switch = true;
#else
  static const bool deceit_bit_disables_switch = false;
#endif
#ifdef CONFIG_DISABLE_AUTO_SWITCH
  static const bool disable_automatic_switch = true;
#else
  static const bool disable_automatic_switch = false;
#endif
  static const bool fine_grained_cputime = false;


  static bool irq_ack_in_kernel;

  static bool esc_hack;
  static int  serial_esc;
  static bool watchdog;
  static bool apic;
  
  static unsigned tbuf_entries;

#ifdef CONFIG_PROFILE
  static bool profiling;
#else
  static const bool profiling = false;
#endif
  static const int profiling_rate = 100;
  static const int profile_irq = 0;

  // kernel (idle) task definitions
  static const unsigned kernel_taskno = 0;
  static const L4_uid kernel_id;
  static const unsigned kernel_prio = 1;
  static const unsigned kernel_mcp = 255;
  
  // sigma0 task definitions
  static const unsigned sigma0_taskno = 2;
  static const L4_uid sigma0_id; 
  static const unsigned sigma0_prio = 0x10;
  static const unsigned sigma0_mcp = 0;

  // root (boot) task definitions
  static const unsigned boot_taskno = 4;
  static const L4_uid boot_id;
  static const unsigned boot_prio = 0x10;
  static const unsigned boot_mcp = 255;

  // default number of clock ticks / time slice
  static const int default_time_slice = 10;
  // one clock tick takes this many microsecs

  
  enum {
#ifndef CONFIG_X2_LIKE_SYS_CALLS
    X2_LIKE_SYS_CALLS = 0,
#else
    X2_LIKE_SYS_CALLS = 1,
#endif
  };

  enum {
#ifndef CONFIG_SMALL_SPACES
    USE_SMALL_SPACES = 0,
#else
    USE_SMALL_SPACES = 1,
#endif
  };
};

IMPLEMENTATION:

#include "boot_info.h"
#include "initcalls.h"

#include <cstring>
#include <cstdlib>


// define some constants which need a memory representation
const L4_uid Config::kernel_id( L4_uid::NIL );
const L4_uid Config::sigma0_id( sigma0_taskno, 0, 0, boot_taskno );
const L4_uid Config::boot_id  ( boot_taskno, 0, 0, kernel_taskno );

// class variables
bool Config::esc_hack = false;
int  Config::serial_esc = Config::SERIAL_NO_ESC;
bool Config::watchdog = false;
bool Config::apic = false;
bool Config::irq_ack_in_kernel = false;
unsigned Config::tbuf_entries = 256;

#ifdef CONFIG_PROFILE
bool Config::profiling = false;
#endif

#ifdef BOCHS
bool Config::hlt_works_ok = true;
bool Config::getchar_does_hlt = true;
bool Config::pic_prio_modify = true;
#endif /* BOCHS */

// don't change the part "DD-L4(xx)/x86 microkernel" -- Rmgr depends on it.
const char *const Config::kernel_version_string = 
  GREETING_COLOR_ANSI_TITLE "Welcome to Fiasco("CONFIG_XARCH")!\n"
  GREETING_COLOR_ANSI_INFO  "DD-L4("CONFIG_ABI")/x86 "
                            "microkernel (C) 1998-2003 TU Dresden\n"
                            "BUILD: " __DATE__ " - gcc: " COMPILER
                            " optimized for " CONFIG_IA32_TARGET
  GREETING_COLOR_ANSI_OFF;


IMPLEMENT FIASCO_INIT
void Config::init()
{
  char const *cmdline = Boot_info::cmdline();
  const char *c;

  if (strstr(cmdline, " -esc"))
    esc_hack = true;

#ifdef CONFIG_PROFILE
  if (strstr(cmdline, " -profile"))
    profiling = true;
#endif

  if (strstr(cmdline, " -always_irqack"))
    irq_ack_in_kernel = true;

  if (    strstr(cmdline, " -serial_esc")
      &&  strstr(cmdline, " -nokdb")
      && !strstr(cmdline, " -nojdb"))
    serial_esc = SERIAL_ESC_NOIRQ;

  if ((c = strstr(cmdline, " -tbuf_entries=")))
    Config::tbuf_entries = strtol(c+15, 0, 0);

  // this makes our life much easier
  if (Config::tbuf_entries < 2)
    Config::tbuf_entries = 2;

  if (strstr(cmdline, " -watchdog"))
    {
      watchdog = true;
      apic = true;
    }
  
  if (strstr(cmdline, " -apic"))
    apic = true;

#ifdef CONFIG_APIC_MASK
  apic = true;
  watchdog = false;
#endif
}

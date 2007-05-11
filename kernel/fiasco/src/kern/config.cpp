/*
 * Global kernel configuration
 */

INTERFACE:

#include <globalconfig.h>
#include "config_tcbsize.h"
#include "l4_types.h"

// special magic to allow old compilers to inline constants

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#if defined(__GNUC__)
# if defined(__GNUC_PATCHLEVEL__)
#  define COMPILER STRINGIFY(__GNUC__) "." STRINGIFY(__GNUC_MINOR__) "." STRINGIFY(__GNUC_PATCHLEVEL__)
# else
#  define COMPILER STRINGIFY(__GNUC__) "." STRINGIFY(__GNUC_MINOR__)
# endif
# define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
# define COMPILER "Non-GCC"
# define GCC_VERSION 0
#endif

#define GREETING_COLOR_ANSI_OFF    "\033[0m"

class Config
{
public:

  static const char *const kernel_warn_config_string;

  enum {
    SERIAL_ESC_IRQ	= 2,
    SERIAL_ESC_NOIRQ	= 1,
    SERIAL_NO_ESC	= 0,
  };

  static void init();
  static void init_arch();

  // global kernel configuration
  static const unsigned kernel_version_id = 0x87004444; // "DD\000\001"

  static const Mword thread_block_size = THREAD_BLOCK_SIZE;

  static const bool conservative = false;
  static const bool monitor_page_faults = false;

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
  static const bool deceit_bit_disables_switch = true;
#else
  static const bool deceit_bit_disables_switch = false;
#endif
#ifdef CONFIG_FINE_GRAINED_CPUTIME
  static const bool fine_grained_cputime = true;
#else
  static const bool fine_grained_cputime = false;
#endif

  static bool irq_ack_in_kernel;
  static bool esc_hack;

  static unsigned tbuf_entries;

#ifdef CONFIG_PROFILE
  static bool profiling;
#else
  static const bool profiling = false;
#endif
#ifdef CONFIG_STACK_DEPTH
  static const bool stack_depth = true;
#else
  static const bool stack_depth = false;
#endif
  static const int profiling_rate = 100;
  static const int profile_irq = 0;

  // kernel (idle) task definitions
  static const unsigned kernel_prio = 0;
  static const unsigned kernel_mcp = 255;
  
  // sigma0 task definitions
  static const unsigned sigma0_prio = 0x10;
  static const unsigned sigma0_mcp = 0;

  // root (boot) task definitions
  static const unsigned boot_prio = 0x10;
  static const unsigned boot_mcp = 255;

  static const int warn_level = CONFIG_WARN_LEVEL;

  static const L4_uid kernel_id;
  static const unsigned kernel_taskno = 0;

  static const L4_uid sigma0_id; 
  static const unsigned sigma0_taskno = 2;

  static const L4_uid boot_id;
  static const unsigned boot_taskno = 4;

  enum {
    Kip_syscalls = 1,
#ifdef CONFIG_SMALL_SPACES
    Small_spaces = 1,
#else
    Small_spaces = 0,
#endif
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
    Assembler_ipc_shortcut = 1,
#else
    Assembler_ipc_shortcut = 0,
#endif
#ifdef CONFIG_NO_FRAME_PTR
    Have_frame_ptr = 0,
#else
    Have_frame_ptr = 1,
#endif
    Mapdb_ram_only = 0,
#ifdef CONFIG_DEBUG_KERNEL_PAGE_FAULTS
    Log_kernel_page_faults = 1,
#else
    Log_kernel_page_faults = 0,
#endif

#ifdef CONFIG_JDB
    Jdb = 1,
#else
    Jdb = 0,
#endif
#ifdef CONFIG_JDB_LOGGING
    Jdb_logging = 1,
#else
    Jdb_logging = 0,
#endif
#ifdef CONFIG_JDB_ACCOUNTING
    Jdb_accounting = 1,
#else
    Jdb_accounting = 0,
#endif
  };
};

INTERFACE [v2]:

#define GREETING_COLOR_ANSI_TITLE  "\033[1;32m"
#define GREETING_COLOR_ANSI_INFO   "\033[0;32m"

EXTENSION class Config
{
public:
  enum 
  { 
    Abi_v2 = 1,
  };
};

INTERFACE [x0]:

#define GREETING_COLOR_ANSI_TITLE  "\033[1;33m"
#define GREETING_COLOR_ANSI_INFO   "\033[0;33m"

EXTENSION class Config
{
public:
  enum
  {
    Abi_v2 = 0,
  };
};

INTERFACE:
// don't change the part "DD-L4(xx)/x86 microkernel" -- Rmgr depends on it.
#ifdef CONFIG_ARM
#define ARCH_NAME "arm"
#else
#define ARCH_NAME "x86"
#endif

INTERFACE[ia32,ux,amd64]:
#define CONFIG_KERNEL_VERSION_STRING \
  GREETING_COLOR_ANSI_TITLE "Welcome to Fiasco("CONFIG_XARCH")!\\n"             \
  GREETING_COLOR_ANSI_INFO "DD-L4("CONFIG_ABI")/" ARCH_NAME " "                 \
                           "microkernel (C) 1998-2007 TU Dresden\\n"            \
                           "Rev: " CODE_VERSION " compiled with gcc " COMPILER \
                            " for " CONFIG_IA32_TARGET                         \
  GREETING_COLOR_ANSI_OFF


//---------------------------------------------------------------------------
INTERFACE [ux]:

EXTENSION class Config
{
public:
  // 32MB RAM => 2.5MB kmem, 128MB RAM => 16MB kmem, >=512MB RAM => 64MB kmem
  static const unsigned kernel_mem_per_cent = 8;
  static const unsigned kernel_mem_max      = 64 << 20;
};

//---------------------------------------------------------------------------
INTERFACE [!ux]:

EXTENSION class Config 
{
public:
  // 32MB RAM => 2.5MB kmem, 128MB RAM => 16MB kmem, >=512MB RAM => 60MB kmem
  static const unsigned kernel_mem_per_cent = 8;
  static const unsigned kernel_mem_max      = 60 << 20;
};

//---------------------------------------------------------------------------
INTERFACE [serial]:

EXTENSION class Config
{
public:
  static int  serial_esc;
};

//---------------------------------------------------------------------------
INTERFACE [!serial]:

EXTENSION class Config
{
public:
  static const int serial_esc = 0;
};

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstring>
#include <cstdlib>
#include <feature.h>
#include "cmdline.h"
#include "initcalls.h"
#include "panic.h"

#ifdef CONFIG_DECEIT_BIT_DISABLES_SWITCH
KIP_KERNEL_FEATURE("deceit_bit_disables_switch");
#endif
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
KIP_KERNEL_FEATURE("asm_ipc_shortcut");
#endif

KIP_KERNEL_ABI_VERSION("9");

// define some constants which need a memory representation
const L4_uid Config::kernel_id( L4_uid::Nil );
const L4_uid Config::sigma0_id( sigma0_taskno, 0, 0, boot_taskno );
const L4_uid Config::boot_id  ( boot_taskno, 0, 0, kernel_taskno );

// class variables
bool Config::esc_hack = false;
#ifdef CONFIG_SERIAL
int  Config::serial_esc = Config::SERIAL_NO_ESC;
#endif
bool Config::irq_ack_in_kernel = false;

#ifdef CONFIG_PROFILE
bool Config::profiling = false;
#endif

unsigned Config::tbuf_entries = 1024;


//-----------------------------------------------------------------------------
IMPLEMENTATION [!arm]:

IMPLEMENT FIASCO_INIT
void Config::init()
{
  char const *cmdline = Cmdline::cmdline();

  init_arch();

  if (strstr(cmdline, " -esc"))
    esc_hack = true;

#ifdef CONFIG_PROFILE
  if (strstr(cmdline, " -profile"))
    profiling = true;
#endif

  if (strstr(cmdline, " -always_irqack"))
    irq_ack_in_kernel = true;

#ifdef CONFIG_SERIAL
  if (    strstr(cmdline, " -serial_esc")
      && !strstr(cmdline, " -noserial")
# ifdef CONFIG_KDB
      &&  strstr(cmdline, " -nokdb")
# endif
      && !strstr(cmdline, " -nojdb"))
    {
      serial_esc = SERIAL_ESC_IRQ;
    }
#endif
}


INTERFACE:

#include "l4_types.h"
#include "pic.h"
#include "config.h"

class trap_state;
class Thread;
class Console_buffer;

class Jdb_entry_frame
{
public:
  Mword gs, fs, es, ds;
  Mword edi, esi, ebp, cr2, ebx, edx, ecx, eax;
  Mword trapno;
  Mword err;
  Mword eip;
  Mword cs;
  Mword eflags;
  Mword esp;
  Mword ss;
  Mword v86_es, v86_ds, v86_fs, v86_gs;
};

class Jdb
{
public:
  static void init();

  static Pic::Status pic_status;
  static const char * const reg_names[];
  static char lbr_active;
  static volatile char test_msr;

  typedef enum
    {
      s_unknown, s_ipc, s_pagefault, s_fputrap, 
      s_interrupt, s_timer_interrupt, s_slowtrap
    } Guessed_thread_state;

  enum { LOGO = 6 };

private:
  Jdb();			// default constructors are undefined
  Jdb(const Jdb&);

  typedef struct 
    {
      char const *name;
      char error_code;
    } Traps;

  template < typename T > static T peek(T const *addr);
 
  static Jdb_entry_frame *entry_frame;
  
  static Traps const traps[20];

  static Mword gdb_trap_eip;
  static Mword gdb_trap_cr2;
  static Mword gdb_trap_no;
  static Mword gdb_trap_err;
  static Mword dr6;
  static unsigned old_phys_pte;

  static int (*nested_trap_handler)(trap_state *state);
 
  static char _connected;
  static char was_input_error;
  static char permanent_single_step;
  static char hide_statline;
  static char use_nested;
  static char code_ret, code_call, code_bra, code_int;

  static const char *toplevel_cmds;
  static const char *non_interactive_cmds;

  typedef enum
    {
      SS_NONE=0, SS_BRANCH, SS_RETURN
    } Step_state;

  static Step_state ss_state;
  static int ss_level;

  static const Unsigned8 *debug_ctrl_str;
  static int              debug_ctrl_len;

  static Mword show_tb_nr;
  static Mword show_tb_refy;
  static Mword show_tb_absy;
  static int jdb_irqs_disabled;

  enum { DEBREG_ACCESS = (1 << 13), SINGLE_STEP = (1 << 14) };

public:
  static char  esc_prompt[32];
  static char  esc_iret[];
  static char  esc_emph[];
  static char  esc_emph2[];
  static char  esc_line[];
  static char  esc_symbol[];
};

#define jdb_enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii \""text "\"	\n\t"\
    "1:			\n\t"\
    "nop;nop;nop	\n\t")


IMPLEMENTATION[ia32]:

#include <flux/x86/gdb.h>

#include <cstring>
#include <csetjmp>
#include <cstdarg>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include "simpleio.h"

#include "apic.h"
#include "boot_info.h"
#include "checksum.h"
#include "cmdline.h"
#include "config.h"
#include "console_buffer.h"
#include "cpu.h"
#include "entry_frame.h"
#include "initcalls.h"
#include "jdb.h"
#include "jdb_core.h"
#include "jdb_bp.h"
#include "jdb_tbuf.h"
#include "jdb_tbuf_init.h"
#include "jdb_screen.h"
#include "jdb_symbol.h"
#include "jdb_lines.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "kernel_uart.h"
#include "kmem.h"
#include "pic.h"
#include "push_console.h"
#include "processor.h"
#include "regdefs.h"
#include "static_init.h"
#include "terminate.h"
#include "thread.h" 
#include "timer.h"
#include "virq.h"
#include "watchdog.h"


Jdb_entry_frame *Jdb::entry_frame;

char Jdb::_connected;			// Jdb::init() was done
char Jdb::was_input_error;		// error in command sequence
char Jdb::permanent_single_step;	// explicit single_step command
char Jdb::hide_statline;		// show status line on enter_kdebugger
char Jdb::use_nested;			// switched to gdb
char Jdb::lbr_active;			// last branch recording active
volatile char Jdb::test_msr;		// = 1: trying to access an msr
char Jdb::code_ret;			// current instruction is ret/iret
char Jdb::code_call;			// current instruction is call
char Jdb::code_bra;			// current instruction is jmp/jxx
char Jdb::code_int;			// current instruction is int x

// holds all commands executable in top level (regardless of current mode)
const char *Jdb::toplevel_cmds = "gjsV_^";

// a short command must be included in this list to be enabled for non-
// interactive execution
const char *Jdb::non_interactive_cmds = "bEIOPS";

Mword Jdb::gdb_trap_eip;		// eip on last trap while in Jdb
Mword Jdb::gdb_trap_cr2;		// cr2 on last trap while in Jdb
Mword Jdb::gdb_trap_no;			// trapno on last trap while in Jdb
Mword Jdb::gdb_trap_err;		// error number last trap while in Jdb
Mword Jdb::dr6;				// debug state when entered Jdb
unsigned Jdb::old_phys_pte = (unsigned)-1;

Jdb::Step_state Jdb::ss_state = SS_NONE; // special single step state
int Jdb::ss_level;			// current call level
  
int (*Jdb::nested_trap_handler)(trap_state *state);

const Unsigned8*Jdb::debug_ctrl_str;	// string+length for remote control of
int             Jdb::debug_ctrl_len;	// Jdb via enter_kdebugger("*#");

Mword Jdb::show_tb_nr = (Mword)-1;	// current event nr
Mword Jdb::show_tb_refy;		// event nr of reference event
Mword Jdb::show_tb_absy;		// event nr of event on top of screen

char Jdb::esc_prompt[32] = "\033[32;1m"; // light green
char Jdb::esc_iret[]     = "\033[36;1m";
char Jdb::esc_emph[]     = "\033[33;1m";
char Jdb::esc_emph2[]    = "\033[32;1m";
char Jdb::esc_line[]     = "\033[37m";
char Jdb::esc_symbol[]   = "\033[33;1m";
Pic::Status Jdb::pic_status;
int  Jdb::jdb_irqs_disabled;

PUBLIC static 
Console_buffer*
Jdb::console_buffer()
{
  static Console_buffer cb;
  return &cb;
}

#define CONFIG_FANCY_TRAP_NAMES
#ifdef CONFIG_FANCY_TRAP_NAMES
struct Jdb::Traps const Jdb::traps[20] = 
{ 
  /* 0*/{"DE DivError", 0},
  /* 1*/{"DB Debug", 0},
  /* 2*/{"NI NMI", 0},
  /* 3*/{"BP Breakpoint", 0},
  /* 4*/{"OF Overflow", 0},
  /* 5*/{"BR Bound range exception", 0},
  /* 6*/{"UD Invalid opcode", 0},
  /* 7*/{"NM Device not available", 0},
  /* 8*/{"DF Double fault", 0},
  /* 9*/{"OV Coprocessor segment overrun", 0},
  /*10*/{"TS Invalid TSS", 1},
  /*11*/{"NP Segment not present", 1},
  /*12*/{"SS Stack segment fault", 1},
  /*13*/{"GP General Protection fault", 1},
  /*14*/{"PF Page fault", 1},
  /*15*/{"Reserved", 0},
  /*16*/{"MF Math Fault", 0},
  /*17*/{"AC Alignment Check", 0},
  /*18*/{"MC Machine Check", 0},
  /*19*/{"XF SIMD floating point exception", 0}
};
#else
struct Jdb::Traps const Jdb::traps[20] = 
{ 
  /* 0*/{"DE", 0},
  /* 1*/{"DB", 0}, 
  /* 2*/{"NI", 0}, 
  /* 3*/{"BP", 0}, 
  /* 4*/{"OF", 0}, 
  /* 5*/{"BR", 0},
  /* 6*/{"UD", 0}, 
  /* 7*/{"NM", 0}, 
  /* 8*/{"DF", 0}, 
  /* 9*/{"OV", 0}, 
  /*10*/{"TS", 1},
  /*11*/{"NP", 1}, 
  /*12*/{"SS", 1}, 
  /*13*/{"GP", 1}, 
  /*14*/{"PF", 1}, 
  /*15*/{"RE", 0},
  /*16*/{"MF", 0},
  /*17*/{"AC", 0},
  /*18*/{"MC", 0},
  /*19*/{"XF", 0}
};
#endif

const char * const Jdb::reg_names[] =
{ "EAX", "EBX", "ECX", "EDX", "EBP", "ESI", "EDI", "EIP", "ESP", "EFL" };

// available from the jdb_dump module
int jdb_dump_addr_task (Address addr, Task_num task, int level)
  __attribute__((weak));


STATIC_INITIALIZE_P(Jdb,JDB_INIT_PRIO);

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void Jdb::init()
{
  if (strstr (Cmdline::cmdline(), " -nojdb"))
    return;

  if (Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::enable_rcv_irq();

  size_t obl = 2*Config::PAGE_SIZE;
  char const *s;

  if ((s = strstr (Cmdline::cmdline(), " -out_buf=")))
    obl = strtoul(s + 10, 0, 0);

  if ((s = strstr (Cmdline::cmdline(), " -jdb_color=")))
    set_prompt_color(s[12]);

  console_buffer()->alloc(obl);
  console_buffer()->enable();

  Kconsole::console()->register_console(console_buffer());

  nested_trap_handler = base_trap_handler;
  base_trap_handler   = (int(*)(struct trap_state*))enter_kdebugger;

  // if esc_hack, serial_esc or watchdog enabled, set slow timer handler
  set_timer_vector_run();
  static Virq tbuf_irq(Config::TBUF_IRQ);

  Jdb_tbuf_init::init(&tbuf_irq);

  // disable lbr feature per default since it eats cycles on AMD Athlon boxes
  Cpu::disable_lbr();

  // reset cursor
  blink_cursor(Jdb_screen::height(), 1);

  _connected = true;
  Thread::may_enter_jdb = true;

  static Push_console c;
  Kconsole::console()->register_console(&c);
}

PUBLIC static inline bool
Jdb::connected()
{
  return _connected;
}

PUBLIC static inline int 
Jdb::source_level_debugging() 
{
  return use_nested; 
}

IMPLEMENT inline
template< typename T >
T Jdb::peek(T const *addr)
{
  // on IA32 we can touch directly into the user-space
  return *(T*)addr;
}

// set position the next screen write goes to
PUBLIC static
void 
Jdb::cursor(int y=0, int x=0)
{
  if (y || x)
    printf("\033[%d;%dH", y, x);
  else
    putstr("\33[H");
}

// set blinking cursor on screen
static void
Jdb::blink_cursor(int y, int x)
{
  printf("\033[%d;%df", y, x);
}

static inline
void
Jdb::backspace()
{
  putstr("\b \b");
}

PUBLIC static inline NEEDS ["simpleio.h"]
void
Jdb::clear_to_eol()
{
  putstr("\033[K");
}

static inline NOEXPORT void
Jdb::home()
{
  putstr("\033[H");
}

static inline NOEXPORT void
Jdb::serial_setscroll(int begin, int end)
{
  printf("\033[%d;%dr",begin,end);
}

static inline NOEXPORT void
Jdb::serial_end_of_screen()
{
  putstr("\033[127;1H");
}

// clear screen by preserving the history of the serial console
PUBLIC static
void
Jdb::fancy_clear_screen()
{
  putstr("\033[24;1H");
  for (int i=0; i<25; i++)
    {
      putchar('\n');
      clear_to_eol();
    }
  home();
}

// Command aborted. If we are interpreting a debug command like
// enter_kdebugger("*#...") this is an error
PUBLIC static
void
Jdb::abort_command()
{
  cursor(Jdb_screen::height(), LOGO);
  clear_to_eol();

  was_input_error = true;
}

static Proc::Status jdb_saved_flags;
#ifdef CONFIG_APIC_MASK
static unsigned apic_timer_irq_enabled;
#endif

// disable interrupts before entering the kernel debugger
static void
Jdb::save_disable_irqs()
{
  if (!jdb_irqs_disabled++)
    {
      // save interrupt flags
      jdb_saved_flags = Proc::cli_save();

#ifdef CONFIG_APIC_MASK
      apic_timer_irq_enabled = Apic::timer_is_irq_enabled();
      Apic::timer_disable_irq();
#endif
      
      Watchdog::disable();

      Jdb::pic_status = Pic::disable_all_save();
      
      if (Config::getchar_does_hlt)
	Timer::enable();

      if (Config::getchar_does_hlt)
	// set timer interrupt does nothing than wakeup from hlt
      	set_timer_vector_stop();
      else
	// we don't need interrupts because getchar does busy waiting
      	Proc::sti_restore(jdb_saved_flags);
    }

  if (Config::getchar_does_hlt)
    // explicit enable interrupts because the timer interrupt is
    // needed to wakeup from "hlt" state in getchar(). All other
    // interrupts are disabled at the pic.
    Proc::sti();
}

// restore interrupts after leaving the kernel debugger
static void
Jdb::restore_irqs()
{
  if (!--jdb_irqs_disabled)
    {
      Proc::cli();

      Pic::restore_all(Jdb::pic_status);

      Watchdog::enable();

#ifdef CONFIG_APIC_MASK
      if (apic_timer_irq_enabled)
	Apic::timer_enable_irq();
#endif

      // reset timer interrupt vector
      if (Config::getchar_does_hlt)  
      	set_timer_vector_run();

      // reset interrupt flags
      Proc::sti_restore(jdb_saved_flags);
    }
}

// save pic state and mask all interupts
static void 
Jdb::open_debug_console(Thread *t)
{
  Cpu::tsc_stop();
  at_enter.execute(t);
  save_disable_irqs();
}

static void 
Jdb::close_debug_console(Thread *t)
{
  restore_irqs();
  at_leave.execute(t);
  Cpu::tsc_continue();
}

// request user for 32-bit hex input
// 'r' treated special as register indicator
// 's' treated special as symbol indicator
// 't' treated special as task indicator
// returns
//   true,  *value = ...        valid input
//   false, *value = 0          no value specified (return/enter without value)
//   false, *value = SPECIAL_T  't' (for thread/task) pressed
//   false, *value = SEPCIAL_R  'r' (for register) pressed
//   false, *value = -1         aborted/symbol not found (escape) */
#define SPECIAL_R		0x01
#define SPECIAL_T		0x02
PUBLIC
static bool
Jdb::x_get_32(Mword *value, Mword special, int first_char=0)
{
  unsigned val=0;
  
  for(int digit=0; digit<8; ) 
    {
      int c, v;

      if (first_char)
	{
	  c=v=first_char;
	  first_char=0;
	}
      else
	c=v=getchar();
      switch(c) 
	{
	case 'a': case 'b': case 'c':
	case 'd': case 'e': case 'f':
	  v -= 'a' - '9' - 1;
	case '0': case '1': case '2':
	case '3': case '4': case '5':
	case '6': case '7': case '8':
	case '9':
	  val = (val << 4) + (v - '0');
	  digit++;
	  putchar(c);
	  break;
	case KEY_BACKSPACE:
	  if (digit) 
	    {
	      backspace();
	      digit--;
	      val >>= 4;
	    }
	  break;
	case 't':
	  if (special & SPECIAL_T)
	    {
	      for (; digit--; )
		backspace();
	      *value = SPECIAL_T;
	      return false;
	    }
	case 'r':
	  if (special & SPECIAL_R)
	    {
	      for (; digit--; )
		backspace();
	      *value = SPECIAL_R;
	      return false;
	    }
	  break;
	case 'x':
	  // 
	  // If last digit was 0, delete it. This makes it possible to 
	  // cut 'n paste hex values like 0x12345678 into the serial terminal
	  if (digit && ((val & 0x10) == 0))
	    {
	      backspace();
	      digit--;
	      val >>= 4;
	    }
	  break;
	case ' ':
	case KEY_RETURN: 
	  if (digit) 
	    { 
	      *value = val; 
	      return true; 
	    }
	  *value = 0; 
	  return false; 
	case KEY_ESC:
	  *value = (Mword)-1;
	  abort_command();
	  return false;
	}
    }
  *value = val;
  return true;
}

PUBLIC
static int
Jdb::get_register(char *reg)
{
  char reg_name[4];
  int i;

  putchar(reg_name[0] = 'E');

  for (i=1; i<3; i++)
    {
      int c = getchar();
      if (c == KEY_ESC)
	return false;
      putchar(reg_name[i] = c & 0xdf);
    }

  reg_name[3] = '\0';

  for (i=0; i<9; i++)
    if (*((unsigned*)reg_name) == *((unsigned*)reg_names[i]))
      break;

  if (i==9)
    return false;
  
  *reg = i+1;
  return true;
}

// do thread lookup using trap_state
PUBLIC
static Thread*
Jdb::get_thread()
{
  Address esp = (Address) entry_frame;

  // special case since we come from the double fault handler stack
  if (entry_frame->trapno == 8 && !(entry_frame->cs & 3))
    esp = entry_frame->esp; // we can trust esp since it comes from main_tss

  if (Kmem::is_tcb_page_fault( esp, 0 ))
    return reinterpret_cast<Thread*>(esp & ~(Config::thread_block_size - 1));

  return reinterpret_cast<Thread*>(Kmem::mem_tcbs);
}

PUBLIC
static Address
Jdb::establish_phys_mapping(Address address, Address *offset)
{
  unsigned pte = pa_to_pte(address);

  if (pte != old_phys_pte)
    {
      // setup adapter page table entry
      *Kmem::jdb_adapter_pt = INTEL_PTE_VALID | INTEL_PTE_WRITE
			    | INTEL_PTE_WTHRU | INTEL_PTE_NCACHE 
			    | INTEL_PTE_REF | INTEL_PTE_MOD
			    | Kmem::pde_global() | pte;
      Kmem::tlb_flush(Kmem::jdb_adapter_page);
      old_phys_pte = pte;
    }

  *offset = address - pte;
  return Kmem::jdb_adapter_page;
}

PUBLIC static
int
Jdb::is_valid_task(Task_num task)
{
  return task == 0 || lookup_space(task) != 0;
}

PUBLIC static
Task_num
Jdb::translate_task(Address addr, Task_num task)
{
  return (Kmem::is_kmem_page_fault(addr, 0)) ? 0 : task;
}

PUBLIC static
int
Jdb::peek_phys(Address phys)
{
  Address offs;
  Address virt = establish_phys_mapping(phys, &offs);

  return *(Unsigned8*)(virt + offs);
}

PUBLIC static
int
Jdb::poke_phys(Address phys, Unsigned8 value)
{
  Address offs;
  Address virt = establish_phys_mapping(phys, &offs);

  *(Unsigned8*)(virt + offs) = value;
  return 0;
}

PUBLIC
static int
Jdb::peek_task(Address addr, Task_num task)
{
  Address phys;

  if (task == 0 || task == 2)
    {
      // address of kernel directory
      phys = Kmem::virt_to_phys((const void*)addr);
      if (phys != (Address)-1)
	return peek_phys(phys); 
    }
  else
    {
      // specific address space, use temporary mapping
      Space *s = lookup_space(task);
      if (!s)
	return -1;
      phys = s->virt_to_phys(addr);
    }

  if (phys == (Address)-1)
    return -1;

  return peek_phys(phys);
}

PUBLIC static
int
Jdb::poke_task(Address addr, Task_num task, Unsigned8 value)
{
  Address phys;

  if (Kmem::is_kmem_page_fault(addr, 0) || task == 0)
    {
      // kernel address
      phys = Kmem::virt_to_phys((const void*)addr);
    }
  else
    {
      // user address, use temporary mapping
      Space *s = lookup_space(task);
      if (!s)
	return -1;
      phys = s->virt_to_phys(addr);
    }

  if (phys == (Address)-1)
    return -1;

  return poke_phys(phys, value);
}

PUBLIC static
int
Jdb::peek_mword_task(Address addr, Task_num task, Mword *result)
{
  Mword mword = 0;

  for (Mword u=0; u<sizeof(Mword); u++)
    {
      int c = Jdb::peek_task(addr + u, task);
      if (c == -1)
	{
	  *result = (Mword)-1;
	  return -1;
	}
      // little endian
      mword |= ((Mword)c) << (8*u);
    }

  *result = mword;
  return 1;
}

PUBLIC static
int
Jdb::peek_addr_task(Address virt, Task_num task, Address *result)
{
  Address addr = 0;

  for (Mword u=0; u<sizeof(Address); u++)
    {
      int c = Jdb::peek_task(virt + u, task);
      if (c == -1)
	{
	  *result = (Address)-1;
	  return -1;
	}
      addr |= ((Address)c) << (8*u);
    }

  *result = addr;
  return 1;
}

PUBLIC static
void
Jdb::poke_mword_task(Address virt, Task_num task, Mword value)
{
  for (Mword u=0; u<sizeof(Mword); u++)
    Jdb::poke_task(virt + u, task, (value >> (8*u)) & 0xff);
}

PUBLIC static
int
Jdb::is_adapter_memory(Address addr, Task_num task)
{
  if (Kmem::is_kmem_page_fault(addr, 0) || task == 0)
    {
      // kernel address
      Address phys = Kmem::virt_to_phys((const void*)addr);
      return (phys != (Address)-1 && phys >= 0x40000000);
    }
  else
    {
      // user address, use temporary mapping
      Space *s = lookup_space(task);
      if (!s)
	return 0;
      Address phys = s->virt_to_phys(addr);
      return (phys != (Address)-1 && phys >= 0x40000000);
    }
}

#define WEAK __attribute__((weak))
extern "C" char in_slowtrap, in_page_fault, in_handle_fputrap;
extern "C" char in_interrupt, in_timer_interrupt, in_timer_interrupt_slow;
extern "C" char ret_switch WEAK, se_ret_switch WEAK, in_slow_ipc1 WEAK;
extern "C" char in_slow_ipc2 WEAK, in_slow_ipc3 WEAK, in_slow_ipc4;
extern "C" char in_slow_ipc5, in_slow_ipc6 WEAK, in_sc_ipc1 WEAK;
extern "C" char in_sc_ipc2 WEAK;
#undef WEAK

// Try to guess the thread state of t by walking down the kernel stack and
// locking at the first return address we find.
PUBLIC static
Jdb::Guessed_thread_state
Jdb::guess_thread_state(Thread *t)
{
  Guessed_thread_state state = s_unknown;
  Mword *ktop = (Mword*)((((Mword)t->kernel_sp) & ~0x7ff) + 0x800);

  for (int i=-1; i>-22; i--)
    {
      if (ktop[i] != 0)
	{
	  if (ktop[i] == (Mword)&in_page_fault)
	    state = s_pagefault;
	  if ((ktop[i] == (Mword)&ret_switch) ||    // shortcut.S, int 0x30
	      (ktop[i] == (Mword)&in_slow_ipc1) ||  // shortcut.S, int 0x30
	      (ktop[i] == (Mword)&se_ret_switch) || // shortcut.S, sysenter
	      (ktop[i] == (Mword)&in_slow_ipc2) ||  // shortcut.S, sysenter
#if defined (CONFIG_JDB_LOGGING) \
    || !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) || defined(CONFIG_PROFILE)
	      (ktop[i] == (Mword)&in_slow_ipc3) ||  // entry.S, int 0x30
#endif
	      (ktop[i] == (Mword)&in_slow_ipc4) ||  // entry.S, int 0x30 log
	      (ktop[i] == (Mword)&in_slow_ipc5) ||  // entry.S, sysenter log
#if defined (CONFIG_JDB_LOGGING) \
    || !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) || defined(CONFIG_PROFILE)
	      (ktop[i] == (Mword)&in_slow_ipc6) ||  // entry.S, sysenter
	      (ktop[i] == (Mword)&in_sc_ipc1)   ||  // entry.S, int 0x30
	      (ktop[i] == (Mword)&in_sc_ipc2)   ||  // entry.S, sysenter
#endif
	     0)
	    state = s_ipc;
	  else if (ktop[i] == (Mword)&in_handle_fputrap)
	    state = s_fputrap;
	  else if (ktop[i] == (Mword)&in_interrupt)
	    state = s_interrupt;
	  else if ((ktop[i] == (Mword)&in_timer_interrupt) ||
		   (ktop[i] == (Mword)&in_timer_interrupt_slow))
	    state = s_timer_interrupt;
	  else if (ktop[i] == (Mword)&in_slowtrap)
	    state = s_slowtrap;
	  if (state != s_unknown)
	    break;
	}
    }

  if (state == s_unknown && (t->state() & Thread_ipc_mask))
    state = s_ipc;

  return state;
}

static bool
Jdb::search_cmd()
{
  Mword   string;
  Address address = 0;
  
  putstr("earch dword:");

  if (!x_get_32(&string, 0))
    return false;

  for (bool research=true; research; )
    {
      if (search_dword(string, &address))
	{
	  Address pa = Kmem::virt_to_phys((void*)address);
	  
	  cursor(Jdb_screen::height(), 1);
      	  printf("  dword %08x found at %08x\033[K\n", string, pa);
	  printf_statline("s %08x%63s", string, "n=next d=goto addr");
          
	  switch (int c=getchar())
	    {
	    case 'd':
	      if (jdb_dump_addr_task != 0)
		{
		  if (!jdb_dump_addr_task((Address)pa, 0, 1))
		    return false;
		}
	      break;
	    case 'n':
	    case KEY_RETURN:
	      address++;
	      research = true;
	      break;
	    default:
	      if (is_toplevel_cmd(c))
		return false;
	      research = false;
	      break;
	    }
	}
      else
	{
	  cursor(Jdb_screen::height(), 1);
      	  printf("  dword %08x not found\033[K\n", string);
	  research = false;
	}
    }
  return true;
}

PUBLIC static
void
Jdb::set_single_step(int on)
{
  if (on)
    entry_frame->eflags |= EFLAGS_TF;
  else
    entry_frame->eflags &= ~EFLAGS_TF;

  permanent_single_step = on;
}

static int
Jdb::switch_debug_state()
{
  putstr("\bswitch_debug_state");
  switch (int c=getchar()) 
    {
    case 'a':
      putchar('\n');
      return 1;

    case 's':
      // pass control to source level debugger
      if (nested_trap_handler) 
	{
	  putchar(c);
	  blink_cursor(Jdb_screen::height(), 1); // the last thing to do
	  use_nested = 1;
	  gdb_trap_recover = 0;
	  if (use_nested)
    	    return nested_trap_handler((trap_state*)entry_frame);
	  return 1;
	}
      puts("no source level debugger available");
      return 1;

    default:
      abort_command();
      return 1;
    }
}

// mail loop
static int
Jdb::execute_command()
{
  for (;;) 
    {
      int c;
      do 
	{
	  if ((c = get_next_cmd()))
	    set_next_cmd(0);
	  else   
	    c = getchar();
	} while (c<' ' && c!=KEY_RETURN);

      printf("\033[K%c", c);

      char _cmd[] = {c, 0};
      Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);

      if (cmd.cmd)
	{
	  int ret;

	  if (!(ret = Jdb_core::exec_cmd (cmd)))
	    {
	      hide_statline = false;
	      last_cmd = 0;
	    }

	  last_cmd = c;
	  return ret;
	}

      switch(c)
	{
	case KEY_RETURN: // show debug message
	  hide_statline = false;
	  break;
	case 's': // search in physical memory
	  search_cmd();
	  break;  
	case 'V': // switch console to hercules/color/gdb
	  return switch_debug_state();
	case 'j': // do restricted "go"
	  switch (putchar(c=getchar()))
	    {
	    case 'b': // go until next branch
	    case 'r': // go until current function returns
	      ss_level = 0;
    	      if (code_call)
		{
	     	  // increase call level because currently we
		  // stay on a call instruction
    		  ss_level++;
		}
	      ss_state = (c == 'b') ? SS_BRANCH : SS_RETURN;
	      // if we have lbr feature, the processor treats the single
	      // step flag as step on branches instead of step on instruction
	      if (Cpu::lbr_type() == Cpu::LBR_P6)
		Cpu::wrmsr(2, 0x1d9);
	      // fall through
	    case 's': // do one single step
	      entry_frame->eflags |= EFLAGS_TF;
	      hide_statline = false;
	      return 0;
	    default:
	      abort_command();
	      break;
	    }
	  break;
	case 'g': // leave kernel debugger, continue current task
	  hide_statline = false;
	  last_cmd = 0;
	  return 0;
	case '^': // (qwerty: shift-6) do reboot
 	  putchar('\n');
	  serial_setscroll(1,127);
	  blink_cursor(Jdb_screen::height(), 1);
	  serial_end_of_screen();
	  terminate(1);
	default:
	  backspace();
	  // ignore character and get next input
	  break;
	}
      last_cmd = c;
      return 1;
    }
}

// Interprete str as control sequences for jdb
// We only allow mostly non-interactive commands here which make sense 
// (e.g. we don't allow d, t, l, u commands)
PUBLIC static int
Jdb::execute_command_non_interactive(const Unsigned8 *str, int len)
{
  Thread *t = get_thread();

  // disable all interrupts
  open_debug_console(t);

  old_phys_pte = (unsigned)-1;
  Push_console::push(str, len, t);

  // prevent output of sequences
  Kconsole::console()->change_state(true, "", ~Mux_console::INENABLED, 0);

  was_input_error = false;
  for (;;)
    {
      int c = getchar();
     
      if (c == '^')
	{
	  terminate(1);
	}
      else if (0 != strchr(non_interactive_cmds, c))
	{
	  char _cmd[] = {c, 0};
	  Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);

	  if (cmd.cmd)
	    {
	      if (!Jdb_core::exec_cmd (cmd))
    		was_input_error = true;
	    }
	}

      if (c == KEY_ESC || was_input_error)
	{
	  Push_console::flush();
	  Kconsole::console()->change_state(true, "", ~Mux_console::INENABLED,
						       Mux_console::INENABLED);
	  close_debug_console(t);
	  return c == KEY_ESC;
	}
    }
}

// take a look at the code of the current thread eip
// set global indicators code_call, code_ret, code_bra, code_int
// This can fail if the current page is still not mapped
static void
Jdb::analyze_code()
{
  // do nothing if page not mapped into this address space
  Space *s = current_space();
  if (  (s->virt_to_phys(entry_frame->eip)   == (Address)-1)
      ||(s->virt_to_phys(entry_frame->eip+1) == (Address)-1))
    return;

  Unsigned8 op1, op2=0;

  op1 = peek((Unsigned8*)entry_frame->eip);
  if (op1 == 0x0f || op1 == 0xff)
    op2 = peek((Unsigned8*)(entry_frame->eip+1));

  code_ret =  (   ((op1 & 0xf6) == 0xc2)	// ret/lret /xxxx
	       || (op1 == 0xcf));		// iret

  code_call = (   (op1 == 0xe8)			// call near
	       || ((op1 == 0xff)
	     	 && ((op2 & 0x30) == 0x10))	// call/lcall *(...)
	       || (op1 == 0x9a));		// lcall xxxx:xxxx

  code_bra =  (   ((op1 & 0xfc) == 0xe0)	// loop/jecxz
	       || ((op1 & 0xf0) == 0x70)	// jxx rel 8 bit
	       || (op1 == 0xeb)			// jmp rel 8 bit
	       || (op1 == 0xe9)			// jmp rel 16/32 bit
	       || ((op1 == 0x0f)
		 && ((op2 & 0xf0) == 0x80))	// jxx rel 16/32 bit
	       || ((op1 == 0xff)
		 && ((op2 & 0x30) == 0x20))	// jmp/ljmp *(...)
	       || (op1 == 0xea));		// ljmp xxxx:xxxx

  code_int =  (   (op1 == 0xcc)			// int3
	       || (op1 == 0xcd)			// int xx
	       || (op1 == 0xce));		// into
}

// entered debugger because of single step trap
static inline NOEXPORT bool
Jdb::handle_single_step(char *error_buffer)
{
  bool really_break = true;

  // special single_step ('j' command): go until branch/return
  if (ss_state != SS_NONE)
    {
      if (Cpu::lbr_type() != Cpu::LBR_NONE)
	{
	  // don't worry, the CPU always knows what she is doing :-)
       	}
      else
	{
	  // we have to emulate lbr looking at the code ...
	  switch (ss_state)
	    {
	    case SS_RETURN:
	      // go until function return
	      really_break = false;
	      if (code_call)
		{
		  // increase call level
		  ss_level++;
		}
	      else if (code_ret)
		{
		  // decrease call level
		  really_break = (ss_level-- == 0);
		}
	      break;
	    case SS_BRANCH:
	    default:
	      // go until next branch
	      really_break = (code_ret || code_call || code_bra || code_int);
	      break;
	    }
	}

      if (really_break)
	{
	  // condition met
	  ss_state = SS_NONE;
	  strcpy(error_buffer, "Branch/Call");
	}
    }
  else // (ss_state == SS_NONE)
    // regular single_step
    strcpy(error_buffer, "Singlestep");

  return really_break;
}

// entered debugger due to debug exception
static inline NOEXPORT bool
Jdb::handle_trap_1(char *error_buffer)
{
  Jdb_bp::reset_debug_status_register(/*dummy=*/0);

  if (dr6 & SINGLE_STEP)
    {
      // the single step mode is the highest-priority debug exception
      return handle_single_step(error_buffer);
    }
  else if (dr6 & DEBREG_ACCESS)
    {
      // read/write from/to debug register
      sprintf(error_buffer, "debug register access");
      return true;
    }
  else if (dr6 & 0xf)
    {
      // hardware breakpoint
      int ret = Jdb_bp::test_break(entry_frame, get_thread(),
				   dr6, error_buffer);
      Jdb_bp::handle_instruction_breakpoint(entry_frame, dr6);
      return ret;
    }
  else
    {
      // other unknown reason
      sprintf(error_buffer, "unknown debug exception (dr6=%08x)", dr6);
      return true;
    }
}

// entered debugger due to software breakpoint
static inline NOEXPORT bool
Jdb::handle_trap_3(char *error_buffer)
{
  Unsigned8 op   = peek((Unsigned8*)entry_frame->eip);
  Mword len      = peek((Unsigned8*)(entry_frame->eip+1));
  Unsigned8 *msg = (Unsigned8*)(entry_frame->eip+2);

  if (op == 0xeb)
    {
      // we are entering here because enter_kdebugger("*#..."); failed
      if (   len > 1
	  && peek(msg)   == '*'
	  && peek(msg+1) == '#')
	{
    	  unsigned i;
	  char ctrl[29];
	  len-=2;
	  msg+=2;
	  for (i=0; (i<sizeof(ctrl)-1) && (i<len); i++)
	    ctrl[i] = peek(msg);
	  ctrl[i] = '\0';
	  // in case there is something wrong ...
	  sprintf(error_buffer, "invalid ctrl sequence \"%s\"", ctrl);
	}
      // enter_kdebugger("...");
      else if (len > 0)
	{
	  unsigned i;
	  len = len < 47 ? len : 47;
	  for(i=0; i<len; i++)
	    error_buffer[i] = peek(msg+i);
	  error_buffer[i]='\0';
	}
    }
  else
    strcpy(error_buffer, "INT 3");

  return true;
}

// entered debugger due to other exception
static inline NOEXPORT bool
Jdb::handle_trap_x(char *error_buffer)
{
  if (traps[entry_frame->trapno].error_code)
    sprintf(error_buffer, "%s (ERR=%08x)",
	traps[entry_frame->trapno].name, entry_frame->err);
  else 
    sprintf(error_buffer, "%s",
	traps[entry_frame->trapno].name);

  return true;
}

static inline NOEXPORT int
Jdb::handle_int3()
{
#ifdef CONFIG_APIC_MASK
  extern unsigned apic_irq_nr;
#endif

  if(entry_frame->trapno==3) 
    {
      Unsigned8 todo = peek((Unsigned8*)entry_frame->eip);
      switch (todo)
	{
	case 0xeb:		// jmp == enter_kdebug()
	  {
	    Unsigned8 len  = peek((Unsigned8*)(entry_frame->eip+1));
	    Unsigned8 *str = (Unsigned8*)(entry_frame->eip+2);
	    if (   len > 2
		&& (peek(str)) == '*'
		&& (peek(str+1)) == '#')
	      {
    		// jdb command
		if (execute_command_non_interactive(str+2, len-2))
		  return 1;
	      }
	  }
	  break;

	case 0x3c: // cmpb
	  todo = peek((Unsigned8*)(entry_frame->eip+1));
	  switch (todo)
	    {
	    case 29:
	      switch (entry_frame->eax)
		{
		case 2:
		  Jdb_tbuf::clear_tbuf();
		  show_tb_absy = 0;
		  show_tb_refy = 0;
		  show_tb_nr   = (Mword)-1;
		  return 1;
		case 3:
		  Watchdog::disable();
		  execute_command("Tgzip");
		  Watchdog::enable();
		  return 1;
		}
	      break;
	    case 30:		// register debug symbols/lines information
	      switch (entry_frame->ecx)
		{
		case 1:
		  Jdb_symbol::register_symbols(entry_frame->eax, 
					       entry_frame->ebx);
		  return 1;
		case 2:
		  Jdb_lines::register_lines(entry_frame->eax, 
					    entry_frame->ebx);
		  return 1;
		case 3:
#ifdef CONFIG_APIC_MASK
		  apic_irq_nr = entry_frame->eax;
		  printf("Special handling for APIC interrupt 0x%02x\n",
			 apic_irq_nr);
#else
		  panic("CONFIG_APIC_MASK not defined");
#endif
		  return 1;
		}
	    }
	  break;
	}
    }
  return 0;
}

static inline NOEXPORT void
Jdb::enable_lbr()
{
  if (lbr_active)
    Cpu::enable_lbr();
}

PUBLIC static int
Jdb::enter_kdebugger(Jdb_entry_frame *ef)
{
  Cpu::disable_lbr();

  entry_frame = ef;

  // check for int $3 user debugging interface
  if (handle_int3())
    return 0;

  volatile bool really_break = true;

  static jmp_buf recover_buf;
  static char error_buffer[81];

  if (use_nested && nested_trap_handler)
    // switch to gdb
    return nested_trap_handler((trap_state*)entry_frame);

  gdb_trap_eip = entry_frame->eip;
  gdb_trap_cr2 = entry_frame->cr2;
  gdb_trap_no  = entry_frame->trapno;
  gdb_trap_err = entry_frame->err;

  if (gdb_trap_recover)
    {
      // re-enable interrupts if we need them because they are disabled
      // due to the if (Config::getchar_does_hlt)
	Proc::sti();
      longjmp(recover_buf, 1);
    }

  if (entry_frame->trapno != 1)
    {
      // don't enter kdebug if user pressed a non-debug command key
      int c = Kconsole::console()->getchar(false);
      if (   (c != -1)		/* valid input */
	  && (c != 0x20)	/* <SPACE> */
	  && (c != 0x1b)	/* <ESC> */
	  && (!is_toplevel_cmd(c)))
	return 0;
    }

  // check reason if we entered through trap #1
  dr6 = Jdb_bp::get_debug_status_register();

  // reset service mapping
  old_phys_pte = (unsigned)-1;

  if (entry_frame->trapno == 1 && (dr6 & 0xf))
    {
      int ret = Jdb_bp::test_log(entry_frame, get_thread(), dr6);
      Jdb_bp::reset_debug_status_register(/*dummy=*/0);
      Jdb_bp::handle_instruction_breakpoint(ef, dr6);
      if (!ret)
	{
	  enable_lbr();
	  return 0;
	}
    }

  console_buffer()->disable();
  // disable all interrupts

  Thread *t = Thread::lookup(context_of(entry_frame));

  open_debug_console(t);

  hide_statline = false;

  // Update page directory to latest version. We need the thread control
  // blocks for showing thread lists. Moreover, we need the kernel slab
  // for executing the destructors.
  if (t->is_valid())
    {
      // update tcb area
      t->space()->
	remote_update(Kmem::mem_tcbs, (Space_context*)Kmem::dir(),
		      Kmem::mem_tcbs, 0x20000000 / Config::SUPERPAGE_SIZE);
      // update slab
      t->space()->
	remote_update(Kmem::_mappings_1_addr, (Space_context*)Kmem::dir(),
		      Kmem::_mappings_1_addr,
		      (Kmem::_mappings_end_1_addr - Kmem::_mappings_1_addr)
		        / Config::SUPERPAGE_SIZE);
    }

  // look at code (necessary for 'j' command)
  analyze_code();

  // clear error message
  *error_buffer = '\0';

  if (entry_frame->trapno == 1)
    really_break = handle_trap_1( error_buffer);
  else if (entry_frame->trapno == 3)
    really_break = handle_trap_3(error_buffer);
  else if (entry_frame->trapno < 20)
    really_break = handle_trap_x(error_buffer);

  if (really_break)
    {
      if (!permanent_single_step)
	entry_frame->eflags &= ~EFLAGS_TF;
      // else S+ mode
    }

  while (setjmp(recover_buf))
    {
      // handle traps which occured while we are in Jdb
      switch (gdb_trap_no)
	{
	case 2:
	  cursor(Jdb_screen::height(), 1);
	  printf("\nNMI occured\n");
	  break;
	case 3:
	  cursor(Jdb_screen::height(), 1);
	  printf("\nSoftware breakpoint inside jdb at %08x\n", gdb_trap_eip-1);
	  break;
	case 13:
	  if (test_msr)
	    {
	      printf(" MSR does not exist or invalid value\n");
	      test_msr = 0;
	    }
	  else
	    {
	      cursor(Jdb_screen::height(), 1);
	      printf("\nGeneral Protection (eip=%08x, err=%08x) -- jdb bug?\n",
		  gdb_trap_eip, gdb_trap_err);
	    }
	  break;
	default:
	  cursor(Jdb_screen::height(), 1);
	  printf("\nInvalid access (trap=%02x err=%08x cr2=%08x eip=%08x) "
	         "-- jdb bug?\n",
	      gdb_trap_no, gdb_trap_err, gdb_trap_cr2, gdb_trap_eip);
	  break;
	}
    }

  if (really_break) 
    {
      // determine current task/thread from stack pointer
      get_current();

      do 
	{
	  serial_setscroll(1, Jdb_screen::height());
	  if (!hide_statline) 
	    {
	      cursor(Jdb_screen::height(), 1);
	      putstr(esc_prompt);
	      printf("\n%s"
		     "    "
		     "--------------------------------------------------------"
		     "EIP: %08x\033[m      \n", 
		     Boot_info::get_checksum_ro() != Checksum::get_checksum_ro()
			? "    WARNING: Fiasco kernel checksum differs -- "
			  "read-only data has changed!\n"
			: "",		     
		     entry_frame->eip);
	      cursor(Jdb_screen::height()-1, LOGO+1);
	      printf("%s", error_buffer);
	      hide_statline = true;
	    }

	  gdb_trap_recover = 1;
	  print_current_tid_statline();

	} while (execute_command());

      // next time we do show_tracebuffer start listing at last entry
      show_tb_absy = 0;
      show_tb_refy = 0;
      
      // scroll one line
      putchar('\n');
      
      // reset scrolling region of serial terminal
      serial_setscroll(1,127);
      
      // reset cursor
      blink_cursor(Jdb_screen::height(), 1);
      
      // goto end of screen
      serial_end_of_screen();
      console_buffer()->enable();
    }
 
  // reenable interrupts
  close_debug_console(t);
  gdb_trap_recover = 0;

  if (Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::enable_rcv_irq();

  enable_lbr();
  
  return 0;
}

PUBLIC static inline
Jdb_entry_frame*
Jdb::get_entry_frame()
{
  return entry_frame;
}

PUBLIC inline
Address_type
Jdb_entry_frame::from_user()
{
  return cs & 3 ? USER : KERNEL;
}

PUBLIC inline
Mword
Jdb_entry_frame::_get_esp()
{
  return from_user() ? esp : (Mword)&esp;
}

PUBLIC inline
Mword
Jdb_entry_frame::_get_ss()
{
  return from_user() ? ss : ::get_ss();
}


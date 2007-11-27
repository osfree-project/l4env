// Inside Jdb the method Jdb::get_thread() should be used instead of
// Thread::current_thread(). The latter function cannot not handle the
// case when we came from the kernel stack context!

INTERFACE:

#include "l4_types.h"
#include "pic.h"

class Trap_state;
class Thread;
class Console_buffer;
class Jdb_entry_frame;

class Jdb
{
public:
  enum
  {
    Msr_test_default     = 0,
    Msr_test_fail_warn   = 1,
    Msr_test_fail_ignore = 2,
  };

  static void init();

  static Pic::Status pic_status;
  static volatile char msr_test;
  static volatile char msr_fail;

  typedef enum
    {
      s_unknown, s_ipc, s_syscall, s_pagefault, s_fputrap,
      s_interrupt, s_timer_interrupt, s_slowtrap, s_user_invoke,
    } Guessed_thread_state;

  template < typename T > static T peek(T const *addr, Address_type user);

  static int (*bp_test_log_only)();
  static int (*bp_test_sstep)();
  static int (*bp_test_break)(char *errbuf, size_t bufsize);
  static int (*bp_test_other)(char *errbuf, size_t bufsize);

private:
  Jdb();			// default constructors are undefined
  Jdb(const Jdb&);

  template < typename T > static T peek(T const *addr);

  static char error_buffer[81];

  static Mword gdb_trap_ip;
  static Mword gdb_trap_pfa;
  static Mword gdb_trap_no;
  static Mword gdb_trap_err;

  static char _connected;
  static char was_input_error;
  static char permanent_single_step;
  static char hide_statline;
  static char use_nested;
  static char jdb_trap_recover;
  static char code_ret, code_call, code_bra, code_int;

  static const char *toplevel_cmds;
  static const char *non_interactive_cmds;

  static char never_break;

  typedef enum
    {
      SS_NONE=0, SS_BRANCH, SS_RETURN
    } Step_state;

  static Step_state ss_state;
  static int ss_level;

  static const Unsigned8 *debug_ctrl_str;
  static int              debug_ctrl_len;

  static int jdb_irqs_disabled;

public:
  static char  esc_iret[];
  static char  esc_bt[];
  static char  esc_emph[];
  static char  esc_emph2[];
  static char  esc_mark[];
  static char  esc_line[];
  static char  esc_symbol[];
};

IMPLEMENTATION [{amd64,ia32}-!serial]:

static inline
void Jdb::init_serial_console()
{}

IMPLEMENTATION [{amd64,ia32}-serial]:

#include <cstdio>
#include "kernel_uart.h"

static
void Jdb::init_serial_console()
{
  if (Config::serial_esc == Config::SERIAL_ESC_IRQ &&
      !Kernel_uart::uart()->failed())
    {
      int irq;

      if ((irq = Kernel_uart::uart()->irq()) == -1)
	{
	  Config::serial_esc = Config::SERIAL_ESC_NOIRQ;
	  puts("SERIAL ESC: Using serial hack in slow timer handler.");
	}
      else
	{
	  Kernel_uart::enable_rcv_irq();
	  printf("SERIAL ESC: allocated IRQ %d for serial uart\n", irq);
	}
    }
}

IMPLEMENTATION[ia32,amd64]:

#include <flux/x86/gdb.h>

#include <cstring>
#include <csetjmp>
#include <cstdarg>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include "simpleio.h"

#include "boot_info.h"
#include "checksum.h"
#include "cmdline.h"
#include "config.h"
#include "cpu.h"
#include "initcalls.h"
#include "idt.h"
#include "jdb_core.h"
#include "jdb_tbuf_init.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "kernel_uart.h"
#include "kernel_task.h"
#include "kmem.h"
#include "logdefs.h"
#include "mem_layout.h"
#include "pic.h"
#include "push_console.h"
#include "processor.h"
#include "regdefs.h"
#include "static_init.h"
#include "terminate.h"
#include "thread.h" 
#include "thread_state.h"
#include "timer.h"
#include "trap_state.h"
#include "virq.h"
#include "vkey.h"
#include "watchdog.h"

char Jdb::_connected;			// Jdb::init() was done
char Jdb::was_input_error;		// error in command sequence
char Jdb::permanent_single_step;	// explicit single_step command
char Jdb::hide_statline;		// show status line on enter_kdebugger
char Jdb::use_nested;			// switched to gdb
char Jdb::jdb_trap_recover;
volatile char Jdb::msr_test;		// = 1: trying to access an msr
volatile char Jdb::msr_fail;		// = 1: MSR access failed
char Jdb::code_ret;			// current instruction is ret/iret
char Jdb::code_call;			// current instruction is call
char Jdb::code_bra;			// current instruction is jmp/jxx
char Jdb::code_int;			// current instruction is int x

// holds all commands executable in top level (regardless of current mode)
const char *Jdb::toplevel_cmds = "jV_";

// a short command must be included in this list to be enabled for non-
// interactive execution
const char *Jdb::non_interactive_cmds = "bEIJLMNOPSU^Z";

char Jdb::never_break;			// Never stop in Jdb if set

Mword Jdb::gdb_trap_ip;			// eip on last trap while in Jdb
Mword Jdb::gdb_trap_pfa;		// pfa on last trap while in Jdb
Mword Jdb::gdb_trap_no;			// trapno on last trap while in Jdb
Mword Jdb::gdb_trap_err;		// error number last trap while in Jdb

Jdb::Step_state Jdb::ss_state = SS_NONE; // special single step state
int Jdb::ss_level;			// current call level
  
char Jdb::error_buffer[81];

const Unsigned8*Jdb::debug_ctrl_str;	// string+length for remote control of
int             Jdb::debug_ctrl_len;	// Jdb via enter_kdebugger("*#");

char Jdb::esc_iret[]     = "\033[36;1m";
char Jdb::esc_bt[]       = "\033[31m";
char Jdb::esc_emph[]     = "\033[33;1m";
char Jdb::esc_emph2[]    = "\033[32;1m";
char Jdb::esc_mark[]     = "\033[35;1m";
char Jdb::esc_line[]     = "\033[37m";
char Jdb::esc_symbol[]   = "\033[33;1m";
Pic::Status Jdb::pic_status;
int  Jdb::jdb_irqs_disabled;

int  (*Jdb::bp_test_log_only)();
int  (*Jdb::bp_test_sstep)();
int  (*Jdb::bp_test_break)(char *errbuf, size_t bufsize);
int  (*Jdb::bp_test_other)(char *errbuf, size_t bufsize);

// available from the jdb_dump module
int jdb_dump_addr_task (Address addr, Task_num task, int level)
  __attribute__((weak));


STATIC_INITIALIZE_P(Jdb,JDB_INIT_PRIO);


IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void Jdb::init()
{
  if (strstr (Cmdline::cmdline(), " -nojdb"))
    return;

  if (strstr (Cmdline::cmdline(), " -jdb_never_stop"))
    never_break = 1;

  init_serial_console();
  
  nested_trap_handler      = Trap_state::base_handler;
  Trap_state::base_handler = enter_kdebugger;

  // if esc_hack, serial_esc or watchdog enabled, set slow timer handler
  Idt::set_vectors_run();

  static Virq tbuf_irq(Config::Tbuf_irq);
  Jdb_tbuf_init::init(&tbuf_irq);

  // disable lbr feature per default since it eats cycles on AMD Athlon boxes
  Cpu::lbr_enable(false);

  static Push_console c;
  Kconsole::console()->register_console(&c);

  Thread::set_int3_handler(handle_int3_threadctx);

  _connected = true;
  Thread::may_enter_jdb = true;
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

IMPLEMENT
template <typename T> T
Jdb::peek(T const *addr)
{
  return current_mem_space()->peek(addr, entry_frame->from_user());
}

IMPLEMENT inline
template <typename T> T
Jdb::peek(T const *addr, Address_type)
{
  // on IA32 we can touch directly into the user-space
  return *(T*)addr;
}

static inline
void
Jdb::backspace()
{
  putstr("\b \b");
}

// Command aborted. If we are interpreting a debug command like
// enter_kdebugger("*#...") this is an error
PUBLIC static
void
Jdb::abort_command()
{
  cursor(Jdb_screen::height(), 6);
  clear_to_eol();
  was_input_error = true;
}

static Proc::Status jdb_saved_flags;

// disable interrupts before entering the kernel debugger
static void
Jdb::save_disable_irqs()
{
  if (!jdb_irqs_disabled++)
    {
      // save interrupt flags
      jdb_saved_flags = Proc::cli_save();

      Watchdog::disable();
      Jdb::pic_status = Pic::disable_all_save();

      if (Config::getchar_does_hlt && Config::getchar_does_hlt_works_ok)
	{
	  // set timer interrupt does nothing than wakeup from hlt
	  Idt::set_vectors_stop();
	  Timer::enable();
	}

    }

  if (Config::getchar_does_hlt && Config::getchar_does_hlt_works_ok)
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

      // reset timer interrupt vector
      if (Config::getchar_does_hlt && Config::getchar_does_hlt_works_ok)
      	Idt::set_vectors_run();

      // reset interrupt flags
      Proc::sti_restore(jdb_saved_flags);
    }
}

// save pic state and mask all interupts
PRIVATE
static void 
Jdb::open_debug_console()
{
  jdb_enter.execute();
  save_disable_irqs();
  if (!Jdb_screen::direct_enabled())
    Kconsole::console()->
      change_state(Console::DIRECT, 0, ~Console::OUTENABLED, 0);
}

PRIVATE
static void 
Jdb::close_debug_console()
{
  Kconsole::console()->
    change_state(Console::DIRECT, 0, ~0UL, Console::OUTENABLED);
  restore_irqs();
  jdb_leave.execute();
}

PUBLIC
static int
Jdb::get_register(char *reg)
{
  char reg_name[4];
  int i;

  putchar(reg_name[0] = Jdb_screen::Reg_prefix);

  for (i=1; i<3; i++)
    {
      int c = getchar();
      if (c == KEY_ESC)
	return false;
      putchar(reg_name[i] = c & 0xdf);
    }

  reg_name[3] = '\0';

  for (i=0; i<9; i++)
    if (*((unsigned*)reg_name) == *((unsigned*)Jdb_screen::Reg_names[i]))
      break;

  if (i==9)
    return false;
  
  *reg = i+1;
  return true;
}

// Do thread lookup using Trap_state. In contrast to Thread::current_thread()
// this function can also handle cases where we entered from kernel stack
// context. We _never_ return 0!
IMPLEMENT
Thread*
Jdb::get_thread()
{
  Address sp = (Address) entry_frame;

  // special case since we come from the double fault handler stack
  if (entry_frame->_trapno == 8 && !(entry_frame->cs() & 3))
    sp = entry_frame->sp(); // we can trust esp since it comes from main_tss

  if (Kmem::is_tcb_page_fault( sp, 0 ))
    return reinterpret_cast<Thread*>(context_of((const void*)sp));

  return reinterpret_cast<Thread*>(Mem_layout::Tcbs);
}

PUBLIC static
Task_num
Jdb::translate_task(Address addr, Task_num task)
{
  return (Kmem::is_kmem_page_fault(addr, 0)) ? 0 : task;
}

PUBLIC static
void
Jdb::peek_phys(Address phys, Mword *value, int width)
{
  assert(!(phys & (width-1)));

  Address virt = Kmem::map_phys_page_tmp(phys, 0);

  switch (width)
    {
    case 1: *value = *(Unsigned8*) (virt); return;
    case 2: *value = *(Unsigned16*)(virt); return;
    case 4: *value = *(Unsigned32*)(virt); return;
    case 8: *value = *(Unsigned64*)(virt); return;
    }
  assert(false);
}

PUBLIC static
void
Jdb::poke_phys(Address phys, Mword value, int width)
{
  assert(!(phys & (width-1)));

  Address virt = Kmem::map_phys_page_tmp(phys, 0);

  switch (width)
    {
    case 1: *(Unsigned8*) (virt) = value; return;
    case 2: *(Unsigned16*)(virt) = value; return;
    case 4: *(Unsigned32*)(virt) = value; return;
    case 8: *(Unsigned64*)(virt) = value; return;
    }
  assert(false);
}

PUBLIC static
int
Jdb::peek_task(Address addr, Task_num task, Mword *value, int width)
{
  Address phys;

  if (task == Config::kernel_taskno || (!Kmem::is_io_bitmap_page_fault(addr)
	&& Kmem::is_kmem_page_fault(addr, 0)))
    {
      // address of kernel directory
      phys = Kmem::virt_to_phys((const void*)addr);
    }
  else
    {
      // specific address space, use temporary mapping
      Space *s = lookup_space(task);
      if (!s)
	return -1;
      phys = s->mem_space()->virt_to_phys_s0 ((void*)addr);
    }

  if (phys == ~0UL)
    return -1;

  peek_phys(phys, value, width);
  return 0;
}

PUBLIC static
int
Jdb::poke_task(Address addr, Task_num task, Mword value, int width)
{
  Address phys;

  if (task == Config::kernel_taskno || (!Kmem::is_io_bitmap_page_fault(addr)
	&& Kmem::is_kmem_page_fault(addr, 0)))
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
      phys = s->mem_space()->virt_to_phys_s0 ((void*)addr);
    }

  if (phys == ~0UL)
    return -1;

  poke_phys(phys, value, width);
  return 0;
}

// The content of apdapter memory is not shown by default because reading
// memory-mapped I/O registers may confuse the hardware. We assume that all
// memory above the end of the RAM is adapter memory.
PUBLIC static
int
Jdb::is_adapter_memory(Address virt, Task_num task)
{
  Address phys;

  if (task == Config::kernel_taskno || (!Kmem::is_io_bitmap_page_fault(virt) 
	&& Kmem::is_kmem_page_fault(virt, 0)))
    {
      // kernel address
      phys = Kmem::virt_to_phys((const void*)virt);
    }
  else
    {
      // user address
      Space *s = lookup_space(task);
      if (!s)
	return false;
      phys = s->mem_space()->virt_to_phys_s0((void*)virt);
    }

  if (phys == ~0UL)
    return false;

  Mem_desc *m = Kip::k()->mem_descs();
  Mem_desc *e = m + Kip::k()->num_mem_descs();
  for (; m != e; ++m)
    if (m->type() == Mem_desc::Conventional && !m->is_virtual()
	&& m->start() <= phys && m->end() >= phys)
      return false;

  return true;
}

#define WEAK __attribute__((weak))
extern "C" char in_slowtrap, in_page_fault, in_handle_fputrap;
extern "C" char in_interrupt, in_timer_interrupt, in_timer_interrupt_slow;
extern "C" char i30_ret_switch WEAK, se_ret_switch WEAK, in_slow_ipc1 WEAK;
extern "C" char in_slow_ipc2 WEAK, in_slow_ipc4;
extern "C" char in_slow_ipc5, in_sc_ipc1 WEAK;
extern "C" char in_sc_ipc2 WEAK, in_syscall WEAK;
#undef WEAK

// Try to guess the thread state of t by walking down the kernel stack and
// locking at the first return address we find.
PUBLIC static
Jdb::Guessed_thread_state
Jdb::guess_thread_state(Thread *t)
{
  Guessed_thread_state state = s_unknown;
  Mword *ktop = (Mword*)((Mword)context_of(t->get_kernel_sp()) +
			  Config::thread_block_size);

  for (int i=-1; i>-26; i--)
    {
      if (ktop[i] != 0)
	{
	  if (ktop[i] == (Mword)&in_page_fault)
	    state = s_pagefault;
	  if ((ktop[i] == (Mword)&i30_ret_switch) ||// shortcut.S, int 0x30
	      (ktop[i] == (Mword)&in_slow_ipc1) ||  // shortcut.S, int 0x30
	      (ktop[i] == (Mword)&se_ret_switch) || // shortcut.S, sysenter
	      (ktop[i] == (Mword)&in_slow_ipc2) ||  // shortcut.S, sysenter
	      (ktop[i] == (Mword)&in_slow_ipc4) ||  // entry.S, int 0x30 log
	      (ktop[i] == (Mword)&in_slow_ipc5) ||  // entry.S, sysenter log
#if defined (CONFIG_JDB_LOGGING) || !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT)
	      (ktop[i] == (Mword)&in_sc_ipc1)   ||  // entry.S, int 0x30
	      (ktop[i] == (Mword)&in_sc_ipc2)   ||  // entry.S, sysenter
#endif
	     0)
	    state = s_ipc;
	  else if (ktop[i] == (Mword)&in_syscall)
	    state = s_syscall;
	  else if (ktop[i] == (Mword)&Thread::user_invoke)
	    state = s_user_invoke;
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

PUBLIC static
void
Jdb::set_single_step(int on)
{
  if (on)
    entry_frame->flags(entry_frame->flags() | EFLAGS_TF);
  else
    entry_frame->flags(entry_frame->flags() & ~EFLAGS_TF);

  permanent_single_step = on;
}

static int
Jdb::switch_debug_state()
{
  if (!nested_trap_handler)
    {
      puts("GDB stub not available");
      return 1;
    }
  
  puts("\npress 's' to switch to GDB stub");
  if (getchar() == 's')
    {
      puts("passing control to GDB stub");
      // disable UART and KDB output
      Kconsole::console()->change_state(Console::UART | Console::DEBUG, 0,
					~Console::OUTENABLED, 0);
      blink_cursor(Jdb_screen::height(), 1);
      use_nested = 1;
      gdb_trap_recover = 0;
      return nested_trap_handler((Trap_state*)entry_frame);
    }

  abort_command();
  return 1;
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

      if (c == KEY_F1)
	c = 'h';

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
	      Cpu::btf_enable(true);
	      // fall through
	    case 's': // do one single step
	      entry_frame->flags(entry_frame->flags() | EFLAGS_TF);
	      hide_statline = false;
	      return 0;
	    default:
	      abort_command();
	      break;
	    }
	  break;
	default:
	  backspace();
	  // ignore character and get next input
	  break;
	}
      last_cmd = c;
      return 1;
    }
}

IMPLEMENTATION[ia32]:

// take a look at the code of the current thread eip
// set global indicators code_call, code_ret, code_bra, code_int
// This can fail if the current page is still not mapped
static void
Jdb::analyze_code()
{
  // do nothing if page not mapped into this address space
  Mem_space *s = current_mem_space();
  if (   entry_frame->ip()+1 > Kmem::user_max()
      || s->virt_to_phys_s0 ((void*)entry_frame->ip())     == (Address)~0UL
      || s->virt_to_phys_s0 ((void*)(entry_frame->ip()+1)) == (Address)~0UL)
    return;

  Unsigned8 op1, op2=0;

  op1 = peek((Unsigned8*)entry_frame->ip());
  if (op1 == 0x0f || op1 == 0xff)
    op2 = peek((Unsigned8*)(entry_frame->ip()+1));

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

IMPLEMENTATION[amd64]:

static void
Jdb::analyze_code()
{}

IMPLEMENTATION[ia32,amd64]:

// entered debugger because of single step trap
static inline NOEXPORT int
Jdb::handle_single_step()
{
  int really_break = 1;

  // special single_step ('j' command): go until branch/return
  if (ss_state != SS_NONE)
    {
      if (Cpu::lbr_type() != Cpu::Lbr_unsupported)
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
	      really_break = 0;
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
	  snprintf(error_buffer, sizeof(error_buffer), "%s", "Branch/Call");
	}
    }
  else // (ss_state == SS_NONE)
    // regular single_step
    snprintf(error_buffer, sizeof(error_buffer), "%s", "Singlestep");

  return really_break;
}

// entered debugger due to debug exception
static inline NOEXPORT int
Jdb::handle_trap1()
{
  if (bp_test_sstep && bp_test_sstep())
    return handle_single_step();

  if (bp_test_break
      && bp_test_break(error_buffer, sizeof(error_buffer)))
    return 1;

  if (bp_test_other
      && bp_test_other(error_buffer, sizeof(error_buffer)))
    return 1;

  return 0;
}

// entered debugger due to software breakpoint
static inline NOEXPORT int
Jdb::handle_trap3()
{
  Unsigned8 op   = peek((Unsigned8*)entry_frame->ip());
  Mword len      = peek((Unsigned8*)(entry_frame->ip()+1));
  Unsigned8 *msg = (Unsigned8*)(entry_frame->ip()+2);

  if (op != 0xeb)
    {
      snprintf(error_buffer, sizeof(error_buffer), "%s", "INT 3");
      return 1;
    }

  // we are entering here because enter_kdebugger("*#..."); failed
  if (len > 1 && peek(msg) == '*' && peek(msg+1) == '#')
    {
      unsigned i;
      char ctrl[29];

      len-=2;
      msg+=2;
      if (peek(msg) == '#')
	{
	  // the ``-jdb_cmd='' sequence
	  msg = (Unsigned8*)entry_frame->value();
	  for (i=0; i<sizeof(ctrl)-1 && *msg; i++, msg++)
	    ctrl[i] = *msg;
	}
      else
	{
	  // a ``enter_kdebug("*#")'' sequence
	  for (i=0; i<sizeof(ctrl)-1 && i<len; i++, msg++)
	    ctrl[i] = peek(msg);
	}
      ctrl[i] = '\0';
      snprintf(error_buffer, sizeof(error_buffer),
	       "invalid ctrl sequence \"%s\"", ctrl);
    }
  // enter_kdebugger("...");
  else if (len > 0)
    {
      unsigned i;
      len = len < 47 ? len : 47;
      len = len < sizeof(error_buffer)-1 ? len : sizeof(error_buffer)-1;
      for(i=0; i<len; i++)
	error_buffer[i] = peek(msg+i);
      error_buffer[i]='\0';
    }

  return 1;
}

// entered debugger due to other exception
static inline NOEXPORT int
Jdb::handle_trapX()
{
  unsigned pos = snprintf(error_buffer, sizeof(error_buffer), "%s",
			  Cpu::exception_string(entry_frame->_trapno)) + 1;
  if (pos < sizeof(error_buffer) &&
      entry_frame->_trapno >= 10 && entry_frame->_trapno <= 14)
    snprintf(error_buffer+pos, sizeof(error_buffer)-pos, "(ERR="L4_PTR_FMT")",
	     entry_frame->_err);

  return 1;
}

/** Int3 debugger interface. This function is called immediately
 * after entering the kernel debugger.
 * @return 1 if command was successfully interpreted
 */
static inline NOEXPORT int
Jdb::handle_int3()
{
  if (entry_frame->_trapno == 3) 
    {
      Unsigned8 todo = peek((Unsigned8*)entry_frame->ip());

      // jmp == enter_kdebug()
      if (todo == 0xeb)
	{
	  Unsigned8 len  = peek((Unsigned8*)(entry_frame->ip()+1));
	  Unsigned8 *str = (Unsigned8*)(entry_frame->ip()+2);

      	  if (len > 2 && peek(str) == '*' && peek(str+1) == '#')
	    {
	      // jdb command
	      int ret;

	      open_debug_console();
	      if (peek(str+2) == '#')
		ret = execute_command_ni((Unsigned8 const*)
		                          entry_frame->value());
	      else
	      	ret = execute_command_ni(str+2, len-2);
      	      close_debug_console();
	      if (ret)
		return 1;
	    }
	}
      // cmpb
      else if (todo == 0x3c)
	{
	  todo = peek((Unsigned8*)(entry_frame->ip()+1));
	  switch (todo)
	    {
	    case 13:
	      entry_frame->value(Vkey::get());
	      Vkey::clear();
	      return 1;
	      // fiasco_tbuf
	    case 29:
	      if (entry_frame->value() == 3)
		{
		  Watchdog::disable();
		  execute_command("Tgzip");
		  Watchdog::enable();
		  return 1;
		}
	      break;
	    }
	}
    }
  return 0;
}

static FIASCO_FASTCALL
int
Jdb::enter_kdebugger(Trap_state *ts)
{
  Cpu::debugctl_disable();

  if (!jdb_trap_recover)
    entry_frame = static_cast<Jdb_entry_frame*>(ts);

  // check for int $3 user debugging interface
  if (handle_int3())
    {
      Cpu::debugctl_enable();
      return 0;
    }

  volatile bool really_break = true;

  static jmp_buf recover_buf;

  if (use_nested && nested_trap_handler)
    // switch to gdb
    return nested_trap_handler((Trap_state*)entry_frame);

  if (jdb_trap_recover)
    {
      gdb_trap_ip  = ts->ip();
      gdb_trap_pfa = ts->_cr2;
      gdb_trap_no  = ts->_trapno;
      gdb_trap_err = ts->_err;

      // Since we entred the kernel debugger a second time, gdb_trap_recover
      // has a value of 2 now. We don't leave this function so correct the
      // entry counter
      gdb_trap_recover--;

      // re-enable interrupts if we need them because they are disabled
      if (Config::getchar_does_hlt && Config::getchar_does_hlt_works_ok)
	Proc::sti();
      longjmp(recover_buf, 1);
    }

  if (entry_frame->_trapno == 1 && bp_test_log_only && bp_test_log_only())
    {
      // don't enter debugger, only logged breakpoint
      Cpu::debugctl_enable();
      return 0;
    }

  // all following exceptions are handled by jdb itself
  jdb_trap_recover = 1;

  // disable all interrupts
  open_debug_console();

  hide_statline = false;

  // Update page directory to latest version. We need the thread control
  // blocks for showing thread lists. Moreover, we need the kernel slab
  // for executing the destructors.

  if (current_mem_space() != Kernel_task::kernel_task()->mem_space())
    {
  // update tcb area
  current_mem_space()->
	remote_update(Mem_layout::Tcbs, 
		      Kernel_task::kernel_task()->mem_space(),
		      Mem_layout::Tcbs, 
		      (Mem_layout::Tcbs_end -
		       Mem_layout::Tcbs) / Config::SUPERPAGE_SIZE);
  // update slab
  current_mem_space()->
	remote_update(Mem_layout::Slabs_start, 
		      Kernel_task::kernel_task()->mem_space(),
		      Mem_layout::Slabs_start,
		      (Mem_layout::Slabs_end - 
		       Mem_layout::Slabs_start) / Config::SUPERPAGE_SIZE);
  // update symbols/lines info
  current_mem_space()->
	remote_update(Mem_layout::Jdb_debug_start, 
		      Kernel_task::kernel_task()->mem_space(),
		      Mem_layout::Jdb_debug_start,
		      (Mem_layout::Jdb_debug_end -
		       Mem_layout::Jdb_debug_start) / Config::SUPERPAGE_SIZE);
    }

  // look at code (necessary for 'j' command)
  analyze_code();

  // clear error message
  *error_buffer = '\0';

  if (entry_frame->_trapno == 1)
    really_break = handle_trap1();
  else if (entry_frame->_trapno == 3)
    really_break = handle_trap3();
  else if (entry_frame->_trapno < 20)
    really_break = handle_trapX();

  if (really_break)
    {
      if (!permanent_single_step)
	entry_frame->flags(entry_frame->flags() & ~EFLAGS_TF);
      // else S+ mode
    }

  while (setjmp(recover_buf))
    {
      // handle traps which occured while we are in Jdb
      Kconsole::console()->end_exclusive(Console::GZIP);

      switch (gdb_trap_no)
	{
	case 2:
	  cursor(Jdb_screen::height(), 1);
	  printf("\nNMI occured\n");
	  break;
	case 3:
	  cursor(Jdb_screen::height(), 1);
	  printf("\nSoftware breakpoint inside jdb at "L4_PTR_FMT"\n", 
	         gdb_trap_ip-1);
	  break;
	case 13:
	  switch (msr_test)
	    {
	    case Msr_test_fail_warn:
	      printf(" MSR does not exist or invalid value\n");
	      msr_test = Msr_test_default;
	      msr_fail = 1;
	      break;
	    case Msr_test_fail_ignore:
	      msr_test = Msr_test_default;
	      msr_fail = 1;
	      break;
	    default:
	      cursor(Jdb_screen::height(), 1);
	      printf("\nGeneral Protection (eip="L4_PTR_FMT","
		     " err="L4_PTR_FMT") -- jdb bug?\n",
		  gdb_trap_ip, gdb_trap_err);
	      break;
	    }
	  break;
	default:
	  cursor(Jdb_screen::height(), 1);
	  printf("\nInvalid access (trap=%02lx err="L4_PTR_FMT
	         " pfa="L4_PTR_FMT" eip="L4_PTR_FMT") "
	         "-- jdb bug?\n",
	      gdb_trap_no, gdb_trap_err, gdb_trap_pfa, gdb_trap_ip);
	  break;
	}
    }

  if (!never_break && really_break) 
    {
      // determine current task/thread from stack pointer
      update_prompt();

      LOG_MSG(current_active, "=== enter jdb ===");

      do 
	{
	  screen_scroll(1, Jdb_screen::height());
	  if (!hide_statline) 
	    {
	      cursor(Jdb_screen::height(), 1);
	      printf("\n%s%s    %sIP: "L4_PTR_FMT"\033[m      \r",
		     esc_prompt,
		     Boot_info::get_checksum_ro() != Checksum::get_checksum_ro()
			? "    WARNING: Fiasco kernel checksum differs -- "
			  "read-only data has changed!\n"
			: "",
		     Jdb_screen::Line, entry_frame->ip());
	      printf("    %s--\033[m%s\n", esc_prompt, error_buffer);
	      hide_statline = true;
	    }

	  printf_statline(0, 0, "_");

	} while (execute_command());

      // reset scrolling region of serial terminal
      screen_scroll(1,127);
      
      // reset cursor
      blink_cursor(Jdb_screen::height(), 1);
      
      // goto end of screen
      Jdb::cursor(127, 1);
    }
 
  // reenable interrupts
  close_debug_console();
  jdb_trap_recover = 0;

  if (Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::enable_rcv_irq();

  Cpu::debugctl_enable();
  return 0;
}

PUBLIC static inline
void
Jdb::enter_getchar()
{}

PUBLIC static inline
void
Jdb::leave_getchar()
{}

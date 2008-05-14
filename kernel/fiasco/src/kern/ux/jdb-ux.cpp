INTERFACE [ux]:

#include "types.h"
#include "l4_types.h"

class Trap_state;
class Thread;
class Context;
class Jdb_entry_frame;
class Mem_space;

class Jdb
{
public:
  static const char * const reg_names[];

  typedef enum
    {
      s_unknown, s_ipc, s_syscall, s_pagefault, s_fputrap,
      s_interrupt, s_timer_interrupt, s_slowtrap, s_user_invoke,
    } Guessed_thread_state;

  enum { MIN_SCREEN_HEIGHT = 20, MIN_SCREEN_WIDTH = 80 };

  template < typename T > static T peek(T const *addr, Address_type user);

  static int (*bp_test_log_only)();
  static int (*bp_test_break)(char *errbuf, size_t bufsize);

private:
  static unsigned short rows, cols;
  static char was_input_error;
  static const char *toplevel_cmds;
  static const char *non_interactive_cmds;
  static char hide_statline;
  static char error_buffer[64];


public:
  static char esc_iret[];
  static char esc_bt[];
  static char esc_emph[];
  static char esc_emph2[];
  static char esc_mark[];
  static char esc_line[];
  static char esc_symbol[];
};

IMPLEMENTATION [ux]:

#include <cstdio>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "globals.h"
#include "initcalls.h"
#include "jdb_core.h"
#include "jdb_dbinfo.h"
#include "jdb_screen.h"
#include "jdb_tbuf_init.h"
#include "kernel_console.h"
#include "kernel_thread.h"
#include "keycodes.h"
#include "kmem.h"
#include "libc_support.h"
#include "logdefs.h"
#include "mem_layout.h"
#include "push_console.h"
#include "simpleio.h"
#include "space.h"
#include "static_init.h"
#include "thread.h"
#include "thread_state.h"
#include "trap_state.h"
#include "usermode.h"
#include "vkey.h"

int (*Jdb::bp_test_log_only)();
int (*Jdb::bp_test_break)(char *errbuf, size_t bufsize);

unsigned short Jdb::rows, Jdb::cols;

const char * const Jdb::reg_names[] =
{ "EAX", "EBX", "ECX", "EDX", "EBP", "ESI", "EDI", "EIP", "ESP", "EFL" };

const char *Jdb::toplevel_cmds = "";
const char *Jdb::non_interactive_cmds = "bIJNOP^";

char Jdb::error_buffer[64];

char Jdb::esc_iret[]     = "\033[36m";
char Jdb::esc_bt[]       = "\033[31m";
char Jdb::esc_emph[]     = "\033[33m";
char Jdb::esc_emph2[]    = "\033[32m";
char Jdb::esc_mark[]     = "\033[35m";
char Jdb::esc_line[]     = "\033[36m";
char Jdb::esc_symbol[]   = "\033[33m";
char Jdb::hide_statline;
char Jdb::was_input_error;


STATIC_INITIALIZE_P(Jdb,JDB_INIT_PRIO);

PUBLIC static FIASCO_INIT
void
Jdb::init()
{
  // Install JDB handler
  nested_trap_handler      = Trap_state::base_handler;
  Trap_state::base_handler = enter_kdebugger;

  Jdb_tbuf_init::init(0);

  // be sure that Push_console comes very first
  static Push_console c;
  Kconsole::console()->register_console(&c, 0);

  register_libc_atexit(leave_getchar);

  Thread::set_int3_handler(handle_int3_threadctx);
}

PRIVATE
static void
Jdb::conf_screen()
{
  struct winsize win;
  ioctl (fileno (stdin), TIOCGWINSZ, &win);
  rows = win.ws_row;
  cols = win.ws_col;

  if (rows < MIN_SCREEN_HEIGHT || cols < MIN_SCREEN_WIDTH)
    printf ("%sTerminal probably too small, should be at least %dx%d!\033[0m\n",
	    esc_emph, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);

  Jdb_screen::set_height(rows);
}

/** handle int3 debug extension */
PUBLIC static inline NOEXPORT
int
Jdb::int3_extension()
{
  Address      addr = entry_frame->ip();
  Address_type user = (entry_frame->cs() & 3) ? ADDR_USER : ADDR_KERNEL;
  Unsigned8    todo = peek ((Unsigned8 *) addr, user);

  if (todo == 0x3c && peek ((Unsigned8 *) (addr+1), user) == 13)
    {
      enter_getchar();
      entry_frame->_eax = Vkey::get();
      Vkey::clear();
      leave_getchar();
      return 1;
    }
  else if (todo != 0xeb)
    {
      snprintf (error_buffer, sizeof (error_buffer), "INT 3");
      return 0;
    }

  // todo == 0xeb => enter_kdebug()
  Mword i;
  Mword len = peek ((Unsigned8 *) ++addr, user);

  if (len > 2 &&
      peek (((Unsigned8 *) addr + 1), user) == '*' &&
      peek (((Unsigned8 *) addr + 2), user) == '#')
    {
      char c = peek (((Unsigned8 *) addr + 3), user);

      if ((c == '#')
	  ? execute_command_ni((Unsigned8 *) entry_frame->_eax)
	  : execute_command_ni((Unsigned8 *)(addr + 3), len-2))
	return 1; // => leave Jdb
    }

  len = len < sizeof(error_buffer)-1 ? len : sizeof(error_buffer)-1;
  for (i = 0; i < len; i++)
    error_buffer[i] = peek ((Unsigned8 *) ++addr, user);
  error_buffer[i] = 0;
  return 0;
}

PRIVATE static
int
Jdb::execute_command()
{
  int c;

  do
    {
      if ((c = get_next_cmd()))
	set_next_cmd(0);
      else
	c = getchar();
    } while (c < ' ' && c != KEY_RETURN);

  if (c == KEY_F1)
    c = 'h';

  printf("\033[K%c", c);

  char _cmd[] = {c,0};
  Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);

  if (cmd.cmd)
    {
      int ret;

      if (0 == (ret = Jdb_core::exec_cmd (cmd)))
	{
	  // go -- leave kernel debugger
	  hide_statline = false;
	  last_cmd = 0;
	}

      last_cmd = c;
      return ret;
    }

  if (c == KEY_RETURN)
    hide_statline = false;

  last_cmd = c;
  return 1;
}

PUBLIC static FIASCO_FASTCALL
int
Jdb::enter_kdebugger (Trap_state *ts)
{
  Thread *t;
  
  entry_frame = static_cast<Jdb_entry_frame*>(ts);
  t = get_thread();

  switch (entry_frame->_trapno)
    {
      case 1:
        snprintf (error_buffer, sizeof (error_buffer), "Interception");
        break;
      case 3:
	if (int3_extension())
      	  return 0;
    	if (t->d_taskno())
	  {
	    if (bp_test_log_only && bp_test_log_only())
	      return 0;
	    if (bp_test_break
		&& bp_test_break(error_buffer, sizeof(error_buffer)))
	      break;
	  }
    }

  // execute enter list
  jdb_enter.execute();

  conf_screen();
  
  // Set terminal raw mode
  enter_getchar();

  // Flush all output streams
  fflush (NULL);

  hide_statline = false;

  get_current();

  LOG_MSG(current_active, "=== enter jdb ===");

  do
    {
      if (!hide_statline)
        {
          cursor (Jdb_screen::height(), 1);
          putstr (esc_prompt);
          printf ("\n--%s%.*sESP:%08lx EIP:%08lx\033[m\n",
                      error_buffer, (int)(50 - strlen (error_buffer)),
                      "------------------------------------------------------",
              entry_frame->sp(), entry_frame->ip());
          hide_statline = true;
        }

      printf_statline(0, 0, "_");

    } while (execute_command());

  jdb_leave.execute();

  // Restore terminal mode
  leave_getchar();

  // Flush all output streams
  fflush (NULL);
  
  if (!running)
    signal (SIGIO, SIG_IGN);		// Ignore hardware interrupts

  return 0;
}

/** Deliver Thread object which was active at entry of kernel debugger.
 * If we came from the kernel itself, return Thread with id 0.0 */
PUBLIC static
Context *
Jdb::current_context()
{
  return context_of(entry_frame);
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

PUBLIC
static void
Jdb::abort_command()
{
  cursor(Jdb_screen::height(), 6);
  clear_to_eol();

  was_input_error = true;
}

PUBLIC
static Task_num
Jdb::translate_task(Address /*addr*/, Task_num task)
{
  // we have no idea if addr belongs to kernel or user space
  // since kernel and user occupy different address spaces
  return task;
}

PUBLIC
static Address
Jdb::virt_to_kvirt(Address virt, Task_num task)
{
  Address phys;
  Address size;

  switch (task)
    {
      // Kernel address.
      // We can directly access it via virtual addresses if it's kernel code
      // (which is always mapped, but doesn't appear in the kernel pagetable)
      //  or if we find a mapping for it in the kernel's master pagetable.
    case 0:
      return (virt >= ((Address)&Mem_layout::load) && 
	      virt <  (Kernel_thread::init_done() 
					? (Address)&Mem_layout::end
			      		: (Address)&Mem_layout::initcall_end)
	      || lookup_space (0)->mem_space()->v_lookup (virt, 0, 0, 0))
	? virt
	: (Address) -1;

      // User address.
      // We can't directly access it because it's in a different host process
      // but if the task's pagetable has a mapping for it, we can translate
      // task-virtual -> physical -> kernel-virtual address and then access.
    default:
      return (lookup_space (task)->mem_space()->v_lookup (virt, &phys, 
							  &size, 0))
	? (Address) Kmem::phys_to_virt (phys + (virt & (size-1)))
	: (Address) -1;
     }
  return (Address) -1;
}

PUBLIC
static Address
Jdb::virt_to_kvirt(Address virt, Mem_space* space)
{
  Address phys;
  Address size;

  if (!space)
    {
      // Kernel address.
      // We can directly access it via virtual addresses if it's kernel code
      // (which is always mapped, but doesn't appear in the kernel pagetable)
      //  or if we find a mapping for it in the kernel's master pagetable.
      return (virt >= (Address)&Mem_layout::load && 
	      virt <  (Kernel_thread::init_done() 
				? (Address)&Mem_layout::end
				: (Address)&Mem_layout::initcall_end)
	      || lookup_space(0)->mem_space()->v_lookup (virt, 0, 0, 0))
	? virt
	: (Address) -1;
    }
  else
    {
      // User address.
      // We can't directly access it because it's in a different host process
      // but if the task's pagetable has a mapping for it, we can translate
      // task-virtual -> physical -> kernel-virtual address and then access.
      return (space->v_lookup (virt, &phys, &size, 0))
	? (Address) Kmem::phys_to_virt (phys + (virt & (size-1)))
	: (Address) -1;
    }
}

IMPLEMENT inline NEEDS ["space.h"]
template <typename T>
T
Jdb::peek (T const *addr, Address_type user)
{
  return current_mem_space()->peek(addr, user);
}

PUBLIC static
int
Jdb::peek_task(Address virt, Task_num task, Mword *result, int width)
{
  // make sure we don't cross a page boundary
  if (virt & (width-1))
    return -1;

  Address kvirt = virt_to_kvirt(virt, task);
  if (kvirt == (Address)-1)
    return -1;

  switch (width)
    {
    case 1: *result = *(Unsigned8*) kvirt; return 0;
    case 2: *result = *(Unsigned16*)kvirt; return 0;
    case 4: *result = *(Unsigned32*)kvirt; return 0;
    }

  assert(false);
  return -1;
}

PUBLIC static
int
Jdb::poke_task(Address virt, Task_num task, Mword value, int width)
{
  // make sure we don't cross a page boundary
  if (virt & (width-1))
    return -1;

  Address kvirt = virt_to_kvirt(virt, task);

  if (kvirt == (Address)-1)
    return -1;

  switch (width)
    {
    case 1: *(Unsigned8*) kvirt = value; return 0;
    case 2: *(Unsigned16*)kvirt = value; return 0;
    case 4: *(Unsigned32*)kvirt = value; return 0;
    }

  assert(false);
  return -1;
}

PUBLIC
static int
Jdb::is_adapter_memory(Address /*addr*/, Task_num /*task*/)
{
  return 0;
}

#define WEAK __attribute__((weak))
extern "C" char in_slowtrap, in_page_fault, in_handle_fputrap;
extern "C" char in_interrupt, in_timer_interrupt, in_timer_interrupt_slow;
extern "C" char i30_ret_switch WEAK, in_slow_ipc1 WEAK;
extern "C" char in_slow_ipc2 WEAK, in_slow_ipc4;
extern "C" char in_sc_ipc1 WEAK, in_sc_ipc2 WEAK, in_syscall WEAK;
#undef WEAK

/** Try to guess the thread state of t by walking down the kernel stack and
 * locking at the first return address we find. */
PUBLIC
static Jdb::Guessed_thread_state
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
	      (ktop[i] == (Mword)&in_slow_ipc4) ||  // entry.S, int 0x30
#if !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT)
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

// Don't make these members of Jdb else we have to include <termios.h>
// into the Jdb interface ...
static struct termios raw, new_raw;
static int    getchar_entered;

/** prepare Linux console for raw input */
PUBLIC static
void
Jdb::enter_getchar()
{
  if (!getchar_entered++)
    {
      tcgetattr (fileno (stdin), &raw);
      memcpy(&new_raw, &raw, sizeof(new_raw));
      new_raw.c_lflag    &= ~(ICANON|ECHO);
      new_raw.c_cc[VMIN]  = 0;
      new_raw.c_cc[VTIME] = 1;
      tcsetattr (fileno (stdin), TCSAFLUSH, &new_raw);
    }
}

/** restore Linux console. */
PUBLIC static
void
Jdb::leave_getchar()
{
  if (!--getchar_entered)
    tcsetattr (fileno (stdin), TCSAFLUSH, &raw);
}

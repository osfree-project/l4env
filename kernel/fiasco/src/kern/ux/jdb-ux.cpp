INTERFACE:

#include "types.h"
#include "l4_types.h"

class trap_state;
class Thread;

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
  static int enter_kdebugger (Jdb_entry_frame *e);
  static Thread *get_thread ();
  static const char * const reg_names[];

  typedef enum
    {
      s_unknown, s_ipc, s_pagefault, s_fputrap, 
      s_interrupt, s_timer_interrupt, s_slowtrap
    } Guessed_thread_state;

  enum { MIN_SCREEN_HEIGHT = 20, MIN_SCREEN_WIDTH = 80, LOGO = 6 };

private:
  static int int3_extension();
  static int execute_command();
  static int execute_command_non_interactive(const Unsigned8 *str, int len);

  static Jdb_entry_frame *entry_frame;

  static unsigned short int rows, cols;
  static int (*nested_trap_handler)(trap_state *state);
  static Mword dr6;

  static char was_input_error;
  static const char *toplevel_cmds;
  static const char *non_interactive_cmds;
  static char hide_statline;
  static char error_buffer[64];

public:
  static char esc_prompt[32];
  static char esc_iret[];
  static char esc_emph[];
  static char esc_emph2[];
  static char esc_line[];
  static char esc_symbol[];
};

IMPLEMENTATION[ux]:

#include <cstdio>
#include <flux/x86/base_trap.h>
#include <flux/x86/base_paging.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "globals.h"
#include "initcalls.h"
#include "kernel_thread.h"
#include "jdb_bp.h"
#include "jdb_core.h"
#include "jdb_lines.h"
#include "jdb_screen.h"
#include "jdb_symbol.h"
#include "jdb_tbuf_init.h"
#include "kmem.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "libc_support.h"
#include "push_console.h"
#include "regdefs.h"
#include "simpleio.h"
#include "static_init.h"
#include "thread.h"
#include "usermode.h"

Jdb_entry_frame *Jdb::entry_frame;

int (*Jdb::nested_trap_handler)(trap_state *state);
unsigned short int Jdb::rows, Jdb::cols;
Mword Jdb::dr6;

const char * const Jdb::reg_names[] =
{ "EAX", "EBX", "ECX", "EDX", "EBP", "ESI", "EDI", "EIP", "ESP", "EFL" };

const char *Jdb::toplevel_cmds = "gt^";
const char *Jdb::non_interactive_cmds = "IOP";

char Jdb::error_buffer[64];
char Jdb::esc_prompt[32] = "\033[32m";
char Jdb::esc_iret[]     = "\033[36m";
char Jdb::esc_emph[]     = "\033[33m";
char Jdb::esc_emph2[]    = "\033[32m";
char Jdb::esc_line[]     = "\033[36m";
char Jdb::esc_symbol[]   = "\033[33m";
char Jdb::hide_statline;
char Jdb::was_input_error;

STATIC_INITIALIZE_P(Jdb,JDB_INIT_PRIO);

IMPLEMENT FIASCO_INIT
void
Jdb::init()
{
  // Install JDB handler
  nested_trap_handler = base_trap_handler;
  base_trap_handler   = (int(*)(struct trap_state*))enter_kdebugger;

  Jdb_tbuf_init::init(0);

  // be sure that Push_console comes very first
  static Push_console c;
  Kconsole::console()->register_console(&c, 0);
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

IMPLEMENT inline NOEXPORT NEEDS [<flux/x86/base_trap.h>, "thread.h"]
int
Jdb::int3_extension()
{
  Task_num task = entry_frame->cs == 2 ? get_thread()->id().task() : 0;
  Address addr  = entry_frame->eip;
  unsigned char todo = peek_task (addr, task);

  switch (todo)
    {
      case 0x3c:						// cmpb
        switch (peek_task ((addr + 1), task))
          { 
            case  0:
              putchar (entry_frame->eax & 0xff);		// outchar
              break;
            case  2:						// outstring
              char c;
              while ((c = peek_task (entry_frame->eax++, task)))
                putchar (c);
              break;
            case  5:
              printf ("%08x", entry_frame->eax);		// outhex32
              break;
            case  6:
              printf ("%05x", entry_frame->eax & 0xfffff);	// outhex20
              break; 
            case  7: 
              printf ("%04x", entry_frame->eax & 0xffff);	// outhex16
              break;
            case  8:
              printf ("%03x", entry_frame->eax & 0xfff);	// outhex12
              break;
            case  9:
              printf ("%02x", entry_frame->eax & 0xff);		// outhex8
              break;
            case 11:
              printf ("%d", entry_frame->eax);			// outdec
              break;
            case 30:		// register debug symbols/lines information
              switch (entry_frame->ecx)
                {
                  case 1:
                    Jdb_symbol::register_symbols (entry_frame->eax,
                                                  entry_frame->ebx);
                    break;
                  case 2:
                    Jdb_lines::register_lines (entry_frame->eax,
                                               entry_frame->ebx);
                    break;
                }
          }
        return 1;

      case 0x90:					// kd_display()
        if (peek_task (++addr, task) == 0xeb)
          {
            unsigned char len = peek_task (++addr, task);
            while (len--)
              putchar (peek_task (++addr, task));
            return 1;
          }
        return 0;

      case 0xeb:					// enter_kdebug()
        unsigned char i, len = peek_task (++addr, task);

        if (len > 2 &&
            peek_task ((addr + 1), task) == '*' &&
            peek_task ((addr + 2), task) == '#')
	  {
	    if (peek_task ((addr + 3), task) == '^')
	      {
		running = 0;				// Flag Shutdown
		return 1;
	      }

	    if (execute_command_non_interactive((Unsigned8 *)(addr + 3), len-2))
	      return 1;
	  }

        for (i = 0; i < len && sizeof (error_buffer) - 1; i++)
          error_buffer[i] = peek_task (++addr, task);

        error_buffer[i] = 0;				// Nullterminate
        return 0;
    }

  snprintf (error_buffer, sizeof (error_buffer), "???");

  return 0;
}

IMPLEMENT
int
Jdb::execute_command ()
{
  int c;

  do
    {
      if ((c = get_next_cmd()))
	set_next_cmd(0);
      else
	c = getchar();
    } while (c < ' ' & c!= KEY_RETURN);

  printf("\033[K%c", c);

  char _cmd[] = {c,0};
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

  switch (c)
    {
      case 'g':
	hide_statline = false;
	last_cmd = 0;
        return 0;

      case '^':
        running = 0;
        return 0;

      case 'b':
	if (getchar() == 't')
	  {
	    putchar('t');
	    Jdb_core::Cmd cmd = Jdb_core::has_cmd("bt");
	    if (cmd.cmd)
      	      Jdb_core::exec_cmd (cmd);
	  }
	break;

      case KEY_RETURN:
	hide_statline = false;
	break;
    }
  last_cmd = c;
  return 1;
}

// Interprete str as input sequences for jdb. We only allow mostly non-
// interactive commands which make sense (e.g. we don't allow d, t, u)
IMPLEMENT
int
Jdb::execute_command_non_interactive(const Unsigned8 *str, int len)
{
  Push_console::push(str, len, get_thread());

  was_input_error = false;
  for (;;)
    {
      int c = getchar();

      if (0 != strchr(non_interactive_cmds, c))
	{
	  char _cmd[] = {c, 0};
	  Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);

	  if (cmd.cmd)
	    {
	      putchar(c);
	      if (!Jdb_core::exec_cmd (cmd))
    		was_input_error = true;
	    }
	}

      if (c == KEY_ESC || was_input_error)
	{
	  Push_console::flush();
	  return c == KEY_ESC;
	}
    }
}

IMPLEMENT
int
Jdb::enter_kdebugger (Jdb_entry_frame *e)
{
  struct termios raw;
  int was_raw;
  Thread *t;
  
  entry_frame = e;
  t = get_thread();
  switch (entry_frame->trapno)
    {
      case 1:
        snprintf (error_buffer, sizeof (error_buffer), "Interception");
        break;
      case 3:
	  {
	    int task  = t->id().task();
	    if (task != 0)
	      {
		Mword dr6 = Jdb_bp::get_debug_status_register(task);
		Jdb_bp::reset_debug_status_register(task);
		if (dr6 & 0xf)
		  {
		    char *errbuf = error_buffer;
		    // immediately return if log breakpoint created
		    if (!Jdb_bp::test_log(entry_frame, t, dr6))
		      return 0;

		    Jdb_bp::test_break(entry_frame, t, dr6, errbuf);
		    Jdb_bp::handle_instruction_breakpoint(entry_frame, dr6);
		    break;
		  }
	      }
	  }
	if (int3_extension())
      	  return 0;
    }

  at_enter.execute(t);

  conf_screen();
  
  // Set terminal raw mode
  tcgetattr (fileno (stdin), &raw);
  was_raw         = raw.c_lflag & (ICANON|ECHO);
  raw.c_lflag    &= ~(ICANON|ECHO);
  raw.c_cc[VMIN]  = 0;
  raw.c_cc[VTIME] = 2;
  tcsetattr (fileno (stdin), TCSAFLUSH, &raw);

  hide_statline = false;

  get_current();

  do
    {
      if (!hide_statline)
	{
	  cursor (Jdb_screen::height(), 1);
	  putstr (esc_prompt);
	  printf ("\n--%s%.*sESP:%08x EIP:%08x\033[m\n",
                  error_buffer, 50 - strlen (error_buffer),
                  "------------------------------------------------------",
		  entry_frame->_get_esp(), entry_frame->eip);
	  hide_statline = true;
	}

      print_current_tid_statline();

    } while (execute_command());

  at_leave.execute(t);

  fflush (NULL);

  // restore terminal mode
  tcgetattr (fileno (stdin), &raw);
  raw.c_lflag &= ~(ICANON|ECHO);
  raw.c_lflag |= was_raw;
  tcsetattr (fileno (stdin), TCSAFLUSH, &raw);

  Usermode::set_signal (SIGINT, true);

  if (!running)
    signal (SIGIO, SIG_IGN);		// Ignore hardware interrupts

  return 0;
}

IMPLEMENT
Thread *
Jdb::get_thread ()
{
  Address esp = (Address) entry_frame;

  if (Kmem::is_tcb_page_fault (esp, 0))
    return reinterpret_cast<Thread*>(esp & ~(Config::thread_block_size - 1));
  
  return reinterpret_cast<Thread*>(Kmem::mem_tcbs);
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
static inline NEEDS["simpleio.h"] void
Jdb::clear_to_eol()
{
  putstr("\033[K");
}

// clear screen by preserving the history of the serial console
PUBLIC
static void
Jdb::fancy_clear_screen()
{
  cursor(Jdb_screen::height(), 1);
  for (unsigned int i=0; i<Jdb_screen::height(); i++)
    {
      putchar('\n');
      clear_to_eol();
    }
  cursor();
}

PUBLIC
static void
Jdb::abort_command()
{
  cursor(Jdb_screen::height(), LOGO);
  clear_to_eol();

  was_input_error = true;
}

PUBLIC
static int
Jdb::is_valid_task(Task_num task)
{
  return task == 0 || lookup_space(task) != 0;
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
  extern char load_address, _end;
  Address phys;
  Address size;

  switch (task)
    {
      // Kernel address.
      // We can directly access it via virtual addresses if it's kernel code
      // (which is always mapped, but doesn't appear in the kernel pagetable)
      //  or if we find a mapping for it in the kernel's master pagetable.
    case 0:
      return (virt >= (Address)&load_address && 
	      virt <  (Kernel_thread::init_done() ? (Address)&_end
						  : (Address)&_initcall_end)
	      || lookup_space (0)->v_lookup (virt, 0, 0, 0))
	? virt
	: (Address) -1;

      // User address.
      // We can't directly access it because it's in a different host process
      // but if the task's pagetable has a mapping for it, we can translate
      // task-virtual -> physical -> kernel-virtual address and then access.
    default:
      return (lookup_space (task)->v_lookup (virt, &phys, &size, 0))
	? (Address) Kmem::phys_to_virt (phys + (virt & (size-1)))
	: (Address) -1;
     }
  return (Address) -1;
}

PUBLIC
static int
Jdb::peek_task(Address addr, Task_num task)
{
  Address kvirt = virt_to_kvirt(addr, task);

  return (kvirt == (Address)-1) ? -1 : *(Unsigned8*)kvirt;
}

PUBLIC
static int
Jdb::poke_task(Address addr, Task_num task, Unsigned8 value)
{
  Address kvirt = virt_to_kvirt(addr, task);

  if (kvirt == (Address)-1)
    return -1;

  *(Unsigned8*)kvirt = value;
  return 0;
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

PUBLIC
static int
Jdb::is_adapter_memory(Address /*addr*/, Task_num /*task*/)
{
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

PUBLIC
Mword
Jdb_entry_frame::_get_esp()
{
  return from_user() ? esp : (Mword)&esp;
}

PUBLIC
Mword
Jdb_entry_frame::_get_ss()
{
  return ss;
}

#define WEAK __attribute__((weak))
extern "C" char in_slowtrap, in_page_fault, in_handle_fputrap;
extern "C" char in_interrupt, in_timer_interrupt, in_timer_interrupt_slow;
extern "C" char ret_switch WEAK, se_ret_switch WEAK, in_slow_ipc1 WEAK;
extern "C" char in_slow_ipc2 WEAK, in_slow_ipc3 WEAK, in_slow_ipc4;
extern "C" char in_sc_ipc1 WEAK, in_sc_ipc2 WEAK;
#undef WEAK

// Try to guess the thread state of t by walking down the kernel stack and
// locking at the first return address we find.
PUBLIC
static Jdb::Guessed_thread_state
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
#if !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT)
	      (ktop[i] == (Mword)&in_slow_ipc3) ||  // entry.S, int 0x30
#endif
	      (ktop[i] == (Mword)&in_slow_ipc4) ||  // entry.S, int 0x30
#if !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT)
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

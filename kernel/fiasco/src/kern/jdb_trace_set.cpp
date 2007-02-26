IMPLEMENTATION:

#include <cstdio>

#include "config.h"
#include "cpu.h"
#include "idt.h"
#include "jdb_module.h"
#include "jdb_tbuf.h"
#include "jdb_trace.h"
#include "keycodes.h"
#include "simpleio.h"
#include "static_init.h"

class Jdb_set_trace : public Jdb_module
{
  static char first_char;
  static char second_char;
};

char Jdb_set_trace::first_char;
char Jdb_set_trace::second_char;

extern void (*syscall_table[])();

extern "C" void entry_sys_ipc_log (void);
extern "C" void entry_sys_ipc_c (void);
extern "C" void entry_sys_ipc (void);
extern "C" void entry_sys_fast_ipc_log (void);
extern "C" void entry_sys_fast_ipc_c (void);
extern "C" void entry_sys_fast_ipc (void);

extern "C" void sys_ipc_wrapper (void);
extern "C" void sys_ipc_log_wrapper (void);
extern "C" void sys_ipc_trace_wrapper (void);

extern void jdb_misc_set_log_pf_res (int enable) __attribute__((weak));

static
void
Jdb_set_trace::set_ipc_vector()
{
  void (*int30_entry)(void);
  void (*sysenter_entry)(void);

  if (Jdb_ipc_trace::_trace || Jdb_ipc_trace::_slow_ipc || 
      Jdb_ipc_trace::_log   || Jdb_nextper_trace::_log)
    {
      int30_entry    = entry_sys_ipc_log;
      sysenter_entry = entry_sys_fast_ipc_log;
    }
  else if (!Config::Assembler_ipc_shortcut ||
           (Config::Jdb_logging && Jdb_ipc_trace::_cshortcut) ||
	   (Config::Jdb_logging && Jdb_ipc_trace::_cpath))
    {
      int30_entry    = entry_sys_ipc_c;
      sysenter_entry = entry_sys_fast_ipc_c;
    }
  else
    {
      int30_entry    = entry_sys_ipc;
      sysenter_entry = entry_sys_fast_ipc;
    }

  Idt::set_entry (0x30, (Address) int30_entry, true);

  Cpu::set_sysenter(sysenter_entry);

  if (Jdb_ipc_trace::_trace)
    syscall_table[0] = sys_ipc_trace_wrapper;
  else if ((Jdb_ipc_trace::_log && !Jdb_ipc_trace::_slow_ipc) ||
	   Jdb_nextper_trace::_log)            
    syscall_table[0] = sys_ipc_log_wrapper;
  else
    syscall_table[0] = sys_ipc_wrapper;
}


extern "C" void sys_fpage_unmap_wrapper (void);
extern "C" void sys_fpage_unmap_log_wrapper (void);

static
void
Jdb_set_trace::set_unmap_vector()
{
  if (Jdb_unmap_trace::_log)
    syscall_table[2] = sys_fpage_unmap_log_wrapper;
  else
    syscall_table[2] = sys_fpage_unmap_wrapper;
}

PUBLIC static FIASCO_NOINLINE
void
Jdb_set_trace::set_cpath()
{
  Jdb_ipc_trace::_cpath = 0;
  BEGIN_LOG_EVENT(show_log_context_switch)
  Jdb_ipc_trace::_cpath = 1;
  END_LOG_EVENT;
  BEGIN_LOG_EVENT(show_log_shortcut)
  Jdb_ipc_trace::_cpath = 1;
  END_LOG_EVENT;
  set_ipc_vector();
}

PUBLIC
Jdb_module::Action_code
Jdb_set_trace::action(int cmd, void *&args, char const *&fmt, int &)
{
  switch (cmd)
    {
    case 0:
      // ipc tracing
      if (args == &first_char)
	{
	  switch (first_char)
	    {
	    case ' ':
	    case KEY_RETURN:
	      first_char = ' '; // print status
	      break;
	    case '+': // on
	      Jdb_ipc_trace::_log        = 1;
	      Jdb_ipc_trace::_log_to_buf = 0;
	      break;
	    case '-': // off
	      Jdb_ipc_trace::_log        = 0;
	      break;
	    case '*': // to buffer
	      Jdb_ipc_trace::_log        = 1;
	      Jdb_ipc_trace::_log_to_buf = 1;
	      break;
	    case 'r': // restriction
	    case 'R': // results on
	    case 'S': // use slow ipc path
	    case 'C': // use C shortcut
	    case 'T': // use special tracing format
	      putchar(first_char);
	      fmt  = "%C";
	      args = &second_char;
	      return EXTRA_INPUT;
	    default:
	      return ERROR;
	    }
	  set_ipc_vector();
	  putchar(first_char);
	}
      else if (args == &second_char)
	{
	  switch (first_char)
	    {
	    case 'r':
	      switch (second_char)
		{
		case 'a':
		case 'A':
		  putstr(" restrict to task");
		  fmt  = second_char == 'A' ? "!=%3x" : "==%3x";
		  args = &Jdb_ipc_trace::_task;
		  Jdb_ipc_trace::_other_task = second_char == 'A';
		  return EXTRA_INPUT;
		case 't':
		case 'T':
		  putstr(" restrict to thread");
		  fmt  = second_char == 'T' ? "!=%t" : "==%t";
		  args = &Jdb_ipc_trace::_thread;
		  Jdb_ipc_trace::_other_thread = second_char == 'T';
		  return EXTRA_INPUT;
		case 's':
		  Jdb_ipc_trace::_snd_only = 1;
		  break;
		case '-':
		  Jdb_ipc_trace::clear_restriction();
		  puts(" IPC logging restrictions disabled");
		  return NOTHING;
		default:
		  return ERROR;
		}
	      break;
	    case 'R':
	      if (second_char == '+')
		Jdb_ipc_trace::_log_result = 1;
	      else if (second_char == '-')
		Jdb_ipc_trace::_log_result = 0;
	      else
		return ERROR;
	      putchar(second_char);
	      break;
	    case 'S':
	      if (second_char == '+')
		Jdb_ipc_trace::_slow_ipc = 1;
	      else if (second_char == '-')
		Jdb_ipc_trace::_slow_ipc = 0;
	      else
		return ERROR;
	      set_ipc_vector();
	      putchar(second_char);
	      break;
	    case 'C':
	      if (second_char == '+')
		Jdb_ipc_trace::_cshortcut = 1;
	      else if (second_char == '-')
		Jdb_ipc_trace::_cshortcut = 0;
	      else
		return ERROR;
	      set_ipc_vector();
	      putchar(second_char);
	      break;
	    case 'T':
	      if (second_char == '+')
      		Jdb_ipc_trace::_trace = 1;
	      else if (second_char == '-')
		Jdb_ipc_trace::_trace = 0;
	      else
		return ERROR;
	      set_ipc_vector();
	      putchar(second_char);
	      break;
	    default:
	      return ERROR;
	    }
	}
      else if (args == &Jdb_ipc_trace::_thread)
	Jdb_ipc_trace::_gthread = Jdb_ipc_trace::_thread.gthread();

      putchar('\n');
      Jdb_ipc_trace::show();
      break;

    case 1:
      // pagefault tracing
      if (args == &first_char)
	{
	  switch (first_char)
	    {
	    case ' ':
	    case KEY_RETURN:
	      first_char = ' '; // print status
	      break;
	    case '+': // on
	      Jdb_pf_trace::_log        = 1;
	      Jdb_pf_trace::_log_to_buf = 0;
	      break;
	    case '-': // off
	      Jdb_pf_trace::_log        = 0;
	      break;
	    case '*': // to buffer
	      Jdb_pf_trace::_log        = 1;
	      Jdb_pf_trace::_log_to_buf = 1;
	      break;
	    case 'R': // results on
	      if (!Config::Jdb_logging)
		{
		  puts(" logging disabled");
		  return ERROR;
		}
	      // fall through
	    case 'r': // restriction
	      putchar(first_char);
	      fmt  = "%C";
	      args = &second_char;
	      return EXTRA_INPUT;
	    default:
	      return ERROR;
	    }
	  putchar(first_char);
	}
      else if (args == &second_char)
	{
	  switch (first_char)
	    {
	    case 'R':
	      if (jdb_misc_set_log_pf_res != 0)
		jdb_misc_set_log_pf_res(second_char == '+');
	      break;
	    case 'r':
	      switch (second_char)
		{
		case 't':
		case 'T':
		  putstr(" restrict to thread");
		  fmt  = second_char == 'T' ? "!=%t" : "==%t";
		  args = &Jdb_pf_trace::_thread;
		  Jdb_pf_trace::_other_thread = second_char == 'T';
		  return EXTRA_INPUT;
		case 'x':
		  putstr(" restrict to addr in ");
		  fmt  = "[%p-%p]";
		  args = &Jdb_pf_trace::_addr;
		  return EXTRA_INPUT;
		case '-':
		  Jdb_pf_trace::clear_restriction();
		  puts(" pagefault restrictions disabled");
		  return NOTHING;
		default:
		  return ERROR;
		}
	      break;
	    default:
	      return ERROR;
	    }
      	}
      else if (args == &Jdb_pf_trace::_thread)
	Jdb_pf_trace::_gthread = Jdb_pf_trace::_thread.gthread();

      putchar('\n');
      Jdb_pf_trace::show();
      break;

    case 2:
      // unmap syscall tracing
      if (args == &first_char)
	{
	  switch (first_char)
	    {
	    case ' ':
	    case KEY_RETURN:
	      first_char = ' '; // print status
	      break;
	    case '+': // on
	      Jdb_unmap_trace::_log        = 1;
	      Jdb_unmap_trace::_log_to_buf = 0;
	      break;
	    case '-': // off
	      Jdb_unmap_trace::_log        = 1;
	      break;
	    case '*': // to buffer
	      Jdb_unmap_trace::_log        = 1;
	      Jdb_unmap_trace::_log_to_buf = 1;
	      break;
	    case 'r': // restriction
	      fmt  = "r%C";
	      args = &second_char;
	      return EXTRA_INPUT;
	    default:
	      return ERROR;
	    }
	  putchar(first_char);
	}
      else if (args == &second_char)
	{
	  switch (first_char)
	    {
	    case 'r':
	      switch (second_char)
		{
		case 't':
		case 'T':
		  putstr(" restrict to thread");
		  fmt  = second_char == 'T' ? "!=%t" : "==%t";
		  args = &Jdb_unmap_trace::_thread;
		  Jdb_unmap_trace::_other_thread = second_char == 'T';
		  return EXTRA_INPUT;
		case 'x':
		  putstr(" restrict to addr in ");
		  fmt  = "[%p-%p]";
		  args = &Jdb_unmap_trace::_addr;
		  return EXTRA_INPUT;
		case '-':
		  Jdb_unmap_trace::clear_restriction();
		  puts(" unmap syscall restrictions disabled");
		default:
		  return ERROR;
		}
	      break;
	    default:
	      return ERROR;
	    }
	}
      else if (args == &Jdb_unmap_trace::_thread)
	Jdb_unmap_trace::_gthread = Jdb_unmap_trace::_thread.gthread();

      putchar('\n');
      Jdb_unmap_trace::show();
      break;

    case 3:
      // unmap syscall tracing
      if (args == &first_char)
	{
	  switch (first_char)
	    {
	    case ' ':
	    case KEY_RETURN:
	      first_char = ' '; // print status
	      break;
	    case '+': // buffer
	    case '*': // buffer
	      Jdb_nextper_trace::_log = 1;
	      break;
	    case '-': // off
	      Jdb_nextper_trace::_log = 0;
	      break;
	    default:
	      return ERROR;
	    }
	  set_ipc_vector();
	  putchar(first_char);
	}

      putchar('\n');
      Jdb_nextper_trace::show();
      break;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_set_trace::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "I", "I", "%C",
	  "I{+|-|*|R{+|-}|T{+|-}}\ton/off/buffer ipc logging, on/off result, "
	  "tracing\n"
	  "IS{+|-}\tipc \033[1mwithout\033[m shortcut on/off\n"
	  "IC{+|-}\tipc with C fast path / IPC with Assembler fast path\n"
	  "Ir{t|T|a|A|s|-}\trestrict ipc log to (!)thread/(!)task/snd-only/"
	  "clr",
	  &first_char },
	{ 1, "P", "P", "%C",
	  "P{+|-|*|R{+|-}}\ton/off/buffer pagefault logging, on/off result\n"
	  "Pr{t|T|x|-}\trestrict pagefault log to (!)thread/!thread/addr/clr",
	  &first_char },
        { 2, "U", "U", "%C",
	  "U{+|-|*}\ton/off/buffer unmap logging\n"
	  "Ur{t|T|x|-}\trestrict unmap log to (!)thread/addr/clr",
	  &first_char },
	{ 3, "N", "N", "%C",
	  "N{+|-|*}\tbuffer/off/buffer next period IPC",
	  &first_char },
    };

  return cs;
}

PUBLIC
int const
Jdb_set_trace::num_cmds() const
{
  return 4;
}

PUBLIC
Jdb_set_trace::Jdb_set_trace()
  : Jdb_module("MONITORING")
{
}

static Jdb_set_trace jdb_set_trace INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

void
jdb_trace_set_cpath(void)
{
  Jdb_set_trace::set_cpath();
}

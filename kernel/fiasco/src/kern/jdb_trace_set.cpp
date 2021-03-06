IMPLEMENTATION:

#include <cstdio>

#include "config.h"
#include "cpu.h"
#include "jdb_module.h"
#include "jdb_tbuf.h"
#include "jdb_trace.h"
#include "keycodes.h"
#include "simpleio.h"
#include "static_init.h"

class Jdb_set_trace : public Jdb_module
{
public:
  enum Mode { Off, Log, Log_to_buf, Trace, Use_c_short_cut, Use_slow_path };
  
  Jdb_set_trace() FIASCO_INIT;
  void ipc_tracing(Mode mode);
  void next_preiod_tracing(bool enable);
  void page_fault_tracing(bool enable);
  void unmap_tracing(bool enable);
  
private:
  static char first_char;
  static char second_char;
};

char Jdb_set_trace::first_char;
char Jdb_set_trace::second_char;

extern void jdb_misc_set_log_pf_res (int enable) __attribute__((weak));

IMPLEMENTATION[!{ia32,ux,amd64}]:

PRIVATE inline
void
Jdb_set_trace::set_unmap_vector()
{}


IMPLEMENTATION:

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
	      ipc_tracing(Log);
	      break;
	    case '-': // off
	      ipc_tracing(Off);
	      break;
	    case '*': // to buffer
	      ipc_tracing(Log_to_buf);
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
		ipc_tracing(Use_slow_path);
	      else if (second_char == '-')
		ipc_tracing(Off);
	      else
		return ERROR;
	      putchar(second_char);
	      break;
	    case 'C':
	      if (second_char == '+')
		ipc_tracing(Use_c_short_cut);
	      else if (second_char == '-')
		ipc_tracing(Off);
	      else
		return ERROR;
	      putchar(second_char);
	      break;
	    case 'T':
	      if (second_char == '+')
		ipc_tracing(Trace);
	      else if (second_char == '-')
		ipc_tracing(Off);
	      else
		return ERROR;
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
	      Jdb_unmap_trace::_log        = 0;
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
	  set_unmap_vector();
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
      // next period tracing
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
	      next_preiod_tracing(true);
	      break;
	    case '-': // off
	      next_preiod_tracing(false);
	      break;
	    default:
	      return ERROR;
	    }
	  putchar(first_char);
	}

      putchar('\n');
      Jdb_nextper_trace::show();
      break;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
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
int
Jdb_set_trace::num_cmds() const
{
  return 4;
}

IMPLEMENT
Jdb_set_trace::Jdb_set_trace()
  : Jdb_module("MONITORING")
{
}

static Jdb_set_trace jdb_set_trace INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


INTERFACE:

#include "l4_types.h"

class Sys_ipc_frame;

class Jdb_ipc_trace
{
private:
  static int         _other_thread;
  static L4_uid      _thread;
  static GThread_num _gthread;
  static int         _other_task;
  static Task_num    _task;
  static int         _send_only;
  static int         _log;
  static int         _log_to_buf;
  static int         _log_result;
  static int         _trace;
  static int         _slow_ipc;
  static int         _cpath;
  friend class Jdb_set_trace;
};

class Jdb_pf_trace
{
private:
  static int         _other_thread;
  static L4_uid      _thread;
  static GThread_num _gthread;
  static Address     _y, _z;
  static int         _log;
  static int         _log_to_buf;
  friend class Jdb_set_trace;
};

class Jdb_unmap_trace
{
private:
  static int         _other_thread;
  static L4_uid      _thread;
  static GThread_num _gthread;
  static Address     _y, _z;
  static int         _log;
  static int         _log_to_buf;
  friend class Jdb_set_trace;
};
 

IMPLEMENTATION:

#include <cstdio>

#include "cpu.h"
#include "entry_frame.h"
#include "idt.h"
#include "jdb_module.h"
#include "jdb_tbuf.h"
#include "keycodes.h"
#include "simpleio.h"
#include "static_init.h"

class Jdb_set_trace : public Jdb_module
{
  static char first_char;
  static char second_char;
};

char        Jdb_set_trace::first_char;
char        Jdb_set_trace::second_char;

int         Jdb_ipc_trace::_other_thread;
L4_uid      Jdb_ipc_trace::_thread;
GThread_num Jdb_ipc_trace::_gthread = (GThread_num)-1;
int         Jdb_ipc_trace::_other_task;
Task_num    Jdb_ipc_trace::_task = (Task_num)-1;
int         Jdb_ipc_trace::_send_only;
int         Jdb_ipc_trace::_log;
int         Jdb_ipc_trace::_log_to_buf;
int         Jdb_ipc_trace::_log_result;
int         Jdb_ipc_trace::_trace;
int         Jdb_ipc_trace::_slow_ipc;
int         Jdb_ipc_trace::_cpath;

int         Jdb_pf_trace::_other_thread;
L4_uid      Jdb_pf_trace::_thread;
GThread_num Jdb_pf_trace::_gthread = (GThread_num)-1;
Address     Jdb_pf_trace::_y;
Address     Jdb_pf_trace::_z;
int         Jdb_pf_trace::_log;
int         Jdb_pf_trace::_log_to_buf;

int         Jdb_unmap_trace::_other_thread;
L4_uid      Jdb_unmap_trace::_thread;
GThread_num Jdb_unmap_trace::_gthread = (GThread_num)-1;
Address     Jdb_unmap_trace::_y;
Address     Jdb_unmap_trace::_z;
int         Jdb_unmap_trace::_log;
int         Jdb_unmap_trace::_log_to_buf;


PUBLIC static inline int Jdb_ipc_trace::log()        { return _log; }
PUBLIC static inline int Jdb_ipc_trace::log_buf()    { return _log_to_buf; }
PUBLIC static inline int Jdb_ipc_trace::log_result() { return _log_result; }

PUBLIC static inline NEEDS ["entry_frame.h"]
int
Jdb_ipc_trace::check_restriction(L4_uid id, Sys_ipc_frame *ipc_regs)
{
  return (   ((_gthread == (GThread_num)-1)
	      || ((_other_thread) ^ (_gthread == id.gthread())))
          && ((!_send_only || ipc_regs->snd_desc().has_send()))
	  && ((_task == (Task_num)-1)
	      || ((_other_task)   
		  ^ ((_task == id.task())
		     && (_task == ipc_regs->snd_dest().task()))))
	  );
}

PUBLIC static 
void 
Jdb_ipc_trace::clear_restriction()
{
  _other_thread = 0;
  _gthread      = (GThread_num)-1;
  _other_task   = 0;
  _task         = (Task_num)-1;
  _send_only    = 0;
}

PUBLIC static
void
Jdb_ipc_trace::show()
{
  if (_trace)
    putstr("IPC tracing to tracebuffer enabled");
  else if (_log)
    {
      printf("IPC logging%s%s enabled",
	  _log_result ? " incl. results" : "",
	  _log_to_buf ? " to tracebuffer" : "");
      if (_gthread != (GThread_num)-1)
	{
	  printf("\n    restricted to thread%s %x.%02x%s",
	      _other_thread ? "s !=" : "",
	      _gthread / L4_uid::threads_per_task(),
 	      _gthread % L4_uid::threads_per_task(),
	      _send_only ? ", send-only" : "");
	}
      if (_task != (Task_num)-1)
	{
	  printf("\n    restricted to task%s %x",
	      _other_task ? "s !=" : "", _task);
	}
    }
  else if (_slow_ipc)
    putstr("IPC logging disabled / using the IPC slowpath");
  else
    putstr("IPC logging disabled / using the IPC fastpath");

  putchar('\n');
}

PUBLIC static inline int Jdb_pf_trace::log()     { return _log; }
PUBLIC static inline int Jdb_pf_trace::log_buf() { return _log_to_buf; }

PUBLIC static inline
int
Jdb_pf_trace::check_restriction(L4_uid id, Address pfa)
{
  return (   (((_gthread == (GThread_num)-1)
	      || ((_other_thread) ^ (_gthread == id.gthread()))))
	  && (!(_y | _z)
	      || ((_y > _z) ^ ((pfa >= _y) && (pfa <= _z))))
         );
}

PUBLIC static
void
Jdb_pf_trace::show()
{
  if (_log)
    {
      printf("PF logging%s%s enabled",
#ifdef CONFIG_JDB_LOGGING
	      Jdb_tbuf::log_events[LOG_EVENT_PF_RES]->enabled() 
	      ? " incl. results" : "",
#else
	      "",
#endif
	      _log_to_buf ? " to tracebuffer" : "");
      if (_gthread != (GThread_num)-1)
	{
    	  printf(", restricted to thread%s %x.%02x",
	      _other_thread ? "s !=" : "",
	      _gthread / L4_uid::threads_per_task(),
	      _gthread % L4_uid::threads_per_task());
	    }
      if (_y || _z)
	{
     	  if (_gthread != (GThread_num)-1)
	    putstr(" and ");
	  else
    	    putstr(", restricted to ");
	  if (_y <= _z)
	    printf("%08x <= pfa <= %08x", _y, _z);
	  else
    	    printf("pfa < %08x || pfa > %08x", _z, _y);
	}
    }
  else
    putstr("PF logging disabled");
  putchar('\n');
}

PUBLIC static 
void 
Jdb_pf_trace::clear_restriction()
{
  _other_thread = 0;
  _gthread      = (GThread_num)-1;
  _y            = 0;
  _z            = 0;
}

PUBLIC static inline int Jdb_unmap_trace::log_buf() { return _log_to_buf; }

PUBLIC static inline
int
Jdb_unmap_trace::check_restriction(L4_uid id, Address addr)
{
  return (   (((_gthread == (GThread_num)-1)
	      || ((_other_thread) ^ (_gthread == id.gthread()))))
	  && (!(_y | _z)
	      || ((_y > _z) ^ ((addr >= _y) && (addr <= _z))))
         );
}

PUBLIC static
void
Jdb_unmap_trace::show()
{
  if (_log)
    {
      printf("UNMAP logging%s enabled",
	      _log_to_buf ? " to tracebuffer" : "");
      if (_gthread != (GThread_num)-1)
	{
    	  printf(", restricted to thread%s %x.%02x",
		 _other_thread ? "s !=" : "",
	      	 _gthread / L4_uid::threads_per_task(), 
    		 _gthread % L4_uid::threads_per_task());
	}
      if (_y || _z)
	{
     	  if (_gthread != (GThread_num)-1)
	    putstr(" and ");
	  else
    	    putstr(", restricted to ");
	  if (_y <= _z)
	    printf("%08x <= addr <= %08x", _y, _z);
	  else
    	    printf("addr < %08x || addr > %08x", _z, _y);
	}
    }
  else
    putstr("UNMAP logging disabled");
  putchar('\n');
}

PUBLIC static 
void 
Jdb_unmap_trace::clear_restriction()
{
  _other_thread = 0;
  _gthread      = (GThread_num)-1;
  _y            = 0;
  _z            = 0;
}

extern void (*syscall_table[])();

extern "C" void sys_ipc_entry_log (void);
extern "C" void do_sysenter_log (void);
extern "C" void sys_ipc_entry_c (void);
extern "C" void sys_ipc_entry (void);
extern "C" void do_sysenter_c (void);
extern "C" void do_sysenter (void);

extern "C" void sys_ipc_wrapper (void);
extern "C" void sys_ipc_log_wrapper (void);
extern "C" void sys_ipc_trace_wrapper (void);

static
void
Jdb_set_trace::set_ipc_vector()
{
  void (*int30_entry)(void);
  void (*sysenter_entry)(void);

  if (Jdb_ipc_trace::_trace || Jdb_ipc_trace::_slow_ipc || Jdb_ipc_trace::_log)
    {
      int30_entry    = sys_ipc_entry_log;
      sysenter_entry = do_sysenter_log;
    }
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
#ifdef CONFIG_JDB_LOGGING
  else if (Jdb_ipc_trace::_cpath)
    {
      int30_entry    = sys_ipc_entry_c;
      sysenter_entry = do_sysenter_c;
    }
#endif
  else
    {
      int30_entry    = sys_ipc_entry;
      sysenter_entry = do_sysenter;
    }
#else
  else
    {
      int30_entry    = sys_ipc_entry_c;
      sysenter_entry = do_sysenter_c;
    }
#endif

  Idt::set_vector (0x30, (Address) int30_entry, true);

  Cpu::set_sysenter(sysenter_entry);

  if (Jdb_ipc_trace::_trace)
    syscall_table[0] = sys_ipc_trace_wrapper;
  else if (Jdb_ipc_trace::_log && !Jdb_ipc_trace::_slow_ipc)
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

PUBLIC static
void
Jdb_set_trace::set_cpath()
{
#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT
#ifdef CONFIG_JDB_LOGGING
  if (   Jdb_tbuf::log_events[0]->enabled()
      || Jdb_tbuf::log_events[1]->enabled())
    Jdb_ipc_trace::_cpath = 1;
  else
    Jdb_ipc_trace::_cpath = 0;
  set_ipc_vector();
#endif
#endif
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
	    case 'T': // use special tracing format
	      putchar(first_char);
	      fmt  = "%C";
	      args = &second_char;
	      return EXTRA_INPUT;
	    default:
	      return NOTHING;
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
		  Jdb_ipc_trace::_send_only = 1;
		  break;
		case '-':
		  Jdb_ipc_trace::clear_restriction();
		  puts(" IPC logging restrictions disabled");
		  // fall throgh
		default:
		  return NOTHING;
		}
	      break;
	    case 'R':
	      if (second_char == '+')
		Jdb_ipc_trace::_log_result = 1;
	      else if (second_char == '-')
		Jdb_ipc_trace::_log_result = 0;
	      else
		return NOTHING;
	      putchar(second_char);
	      break;
	    case 'S':
	      if (second_char == '+')
		Jdb_ipc_trace::_slow_ipc = 1;
	      else if (second_char == '-')
		Jdb_ipc_trace::_slow_ipc = 0;
	      else
		return NOTHING;
	      set_ipc_vector();
	      putchar(second_char);
	      break;
	    case 'T':
	      if (second_char == '+')
      		Jdb_ipc_trace::_trace = 1;
	      else if (second_char == '-')
		Jdb_ipc_trace::_trace = 0;
	      else
		return NOTHING;
	      set_ipc_vector();
	      putchar(second_char);
	      break;
	    default:
	      return NOTHING;
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
	    case 'r': // restriction
#ifdef CONFIG_JDB_LOGGING
	    case 'R': // results on
#endif
	      putchar(first_char);
	      fmt  = "%C";
	      args = &second_char;
	      return EXTRA_INPUT;
#ifndef CONFIG_JDB_LOGGING
	    case 'R':
	      puts(" logging disabled");
	      // fall through
#endif
	    default:
	      return NOTHING;
	    }
	  putchar(first_char);
	}
      else if (args == &second_char)
	{
	  switch (first_char)
	    {
#ifdef CONFIG_JDB_LOGGING
	    case 'R':
	      if (second_char == '+' || second_char == '-')
		Jdb_tbuf::log_events[LOG_EVENT_PF_RES]->
					  enable(second_char=='+');
	      else
		return NOTHING;
	      putchar(second_char);
	      break;
#endif
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
		  fmt  = "[%08x-%08x]";
		  args = &Jdb_pf_trace::_y;
		  return EXTRA_INPUT;
		case '-':
		  Jdb_pf_trace::clear_restriction();
		  puts(" pagefault restrictions disabled");
		  // fall throgh
		default:
		  return NOTHING;
		}
	      break;
	    default:
	      return NOTHING;
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
	      return NOTHING;
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
		  fmt  = "[%08x-%08x]";
		  args = &Jdb_unmap_trace::_y;
		  return EXTRA_INPUT;
		case '-':
		  Jdb_unmap_trace::clear_restriction();
		  puts(" unmap syscall restrictions disabled");
		default:
		  return NOTHING;
		}
	      break;
	    default:
	      return NOTHING;
	    }
	}
      else if (args == &Jdb_unmap_trace::_thread)
	Jdb_unmap_trace::_gthread = Jdb_unmap_trace::_thread.gthread();

      putchar('\n');
      Jdb_unmap_trace::show();
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
      Cmd (0, "I", "I", "%C",
	  "I{+|-|*|R{+|-}|T{+|-}}\ton/off/buffer ipc logging, on/off result, "
	  "tracing\n"
	  "Ir{t|T|a|A|s|-}\trestrict ipc log to (!)thread/(!)task/send-only/"
	  "clr",
	  &first_char),
      Cmd (1, "P", "P", "%C",
	  "P{+|-|*|R{+|-}}\ton/off/buffer pagefault logging, on/off result\n"
	  "Pr{t|T|x|-}\trestrict pagefault log to (!)thread/!thread/addr/clr",
	  &first_char),
      Cmd (2, "U", "U", "%C",
	  "U{+|-|*}\ton/off/buffer unmap logging\n"
	  "Ur{t|T|x|-}\trestrict unmap log to (!)thread/addr/clr",
	  &first_char),
    };

  return cs;
}

PUBLIC
int const
Jdb_set_trace::num_cmds() const
{
  return 3;
}

PUBLIC
Jdb_set_trace::Jdb_set_trace()
  : Jdb_module("MONITORING")
{
}

static Jdb_set_trace jdb_set_trace INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

void
jdb_trace_set_cpath(void)
{ Jdb_set_trace::set_cpath(); }


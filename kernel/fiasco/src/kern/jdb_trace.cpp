INTERFACE:

#include "l4_types.h"

class Sys_ipc_frame;
typedef struct
{
  Address lo, hi;
} Addr_range;

class Jdb_ipc_trace
{
private:
  static int         _other_thread;
  static Global_id   _thread;
  static GThread_num _gthread;
  static int         _other_task;
  static Task_num    _task;
  static int         _snd_only;
  static int         _log;
  static int         _log_to_buf;
  static int         _log_result;
  static int         _trace;
  static int         _slow_ipc;
  static int         _cpath;
  static int         _cshortcut;
  friend class Jdb_set_trace;
};

class Jdb_pf_trace
{
private:
  static int         _other_thread;
  static Global_id   _thread;
  static GThread_num _gthread;
  static Addr_range  _addr;
  static int         _log;
  static int         _log_to_buf;
  friend class Jdb_set_trace;
};

class Jdb_unmap_trace
{
private:
  static int         _other_thread;
  static Global_id   _thread;
  static GThread_num _gthread;
  static Addr_range  _addr;
  static int         _log;
  static int         _log_to_buf;
  friend class Jdb_set_trace;
};

class Jdb_nextper_trace
{
private:
  static int         _log;
  friend class Jdb_set_trace;
};


IMPLEMENTATION:

#include <cstdio>

#include "config.h"
#include "entry_frame.h"
#include "jdb_ktrace.h"
#include "jdb_tbuf.h"
#include "simpleio.h"

int         Jdb_ipc_trace::_other_thread;
Global_id   Jdb_ipc_trace::_thread;
GThread_num Jdb_ipc_trace::_gthread = (GThread_num)-1;
int         Jdb_ipc_trace::_other_task;
Task_num    Jdb_ipc_trace::_task = (Task_num)-1;
int         Jdb_ipc_trace::_snd_only;
int         Jdb_ipc_trace::_log;
int         Jdb_ipc_trace::_log_to_buf;
int         Jdb_ipc_trace::_log_result;
int         Jdb_ipc_trace::_trace;
int         Jdb_ipc_trace::_slow_ipc;
int         Jdb_ipc_trace::_cpath;
int         Jdb_ipc_trace::_cshortcut;

int         Jdb_pf_trace::_other_thread;
Global_id   Jdb_pf_trace::_thread;
GThread_num Jdb_pf_trace::_gthread = (GThread_num)-1;
Addr_range  Jdb_pf_trace::_addr;
int         Jdb_pf_trace::_log;
int         Jdb_pf_trace::_log_to_buf;

int         Jdb_unmap_trace::_other_thread;
Global_id   Jdb_unmap_trace::_thread;
GThread_num Jdb_unmap_trace::_gthread = (GThread_num)-1;
Addr_range  Jdb_unmap_trace::_addr;
int         Jdb_unmap_trace::_log;
int         Jdb_unmap_trace::_log_to_buf;

int         Jdb_nextper_trace::_log;

PUBLIC static inline int Jdb_ipc_trace::log()        { return _log; }
PUBLIC static inline int Jdb_ipc_trace::log_buf()    { return _log_to_buf; }
PUBLIC static inline int Jdb_ipc_trace::log_result() { return _log_result; }

PUBLIC static inline NEEDS ["entry_frame.h"]
int
Jdb_ipc_trace::check_restriction (Global_id id,
				  Task_num task,
				  Sys_ipc_frame *ipc_regs,
				  Task_num dst_task)
{
  return (   ((_gthread == (GThread_num)-1)
	      || ((_other_thread) ^ (_gthread == id.gthread()))
	      )
	   && ((!_snd_only || ipc_regs->has_snd()))
	   && ((_task == (Task_num)-1)
	      || ((_other_task)   
		  ^ ((_task == task) || (_task == dst_task))))
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
  _snd_only    = 0;
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
		 L4_uid::task_from_gthread (_gthread),
		 L4_uid::lthread_from_gthread (_gthread),
		 _snd_only ? ", snd-only" : "");
	}
      if (_task != (Task_num)-1)
	{
	  printf("\n    restricted to task%s %x",
	      _other_task ? "s !=" : "", _task);
	}
    }
  else
    {
      printf("IPC logging disabled -- using the IPC %s path",
	  _slow_ipc
	    ? "slow" 
	    : (!Config::Assembler_ipc_shortcut ||
	       (Config::Jdb_logging && Jdb_ipc_trace::_cshortcut) ||
	       (Config::Jdb_logging && Jdb_ipc_trace::_cpath))
	        ? "C fast" : "Assembler fast");
    }

  putchar('\n');
}

PUBLIC static inline int Jdb_pf_trace::log()     { return _log; }
PUBLIC static inline int Jdb_pf_trace::log_buf() { return _log_to_buf; }

PUBLIC static inline NEEDS[<cstdio>]
int
Jdb_pf_trace::check_restriction (Global_id id, Address pfa)
{
  return (   (((_gthread == (GThread_num)-1)
	      || ((_other_thread) ^ (_gthread == id.gthread()))))
	  && (!(_addr.lo | _addr.hi)
	      || (_addr.lo <= _addr.hi && pfa >= _addr.lo && pfa <= _addr.hi)
	      || (_addr.lo >  _addr.hi && pfa <  _addr.hi && pfa >  _addr.lo)));
}

PUBLIC static
void
Jdb_pf_trace::show()
{
  if (_log)
    {
      int res_enabled = 0;
      BEGIN_LOG_EVENT(show_log_pf_res)
      res_enabled = 1;
      END_LOG_EVENT;
      printf("PF logging%s%s enabled",
	     res_enabled ? " incl. results" : "",
	     _log_to_buf ? " to tracebuffer" : "");
      if (_gthread != (GThread_num)-1)
	{
    	  printf(", restricted to thread%s %x.%02x",
		 _other_thread ? "s !=" : "",
		 L4_uid::task_from_gthread (_gthread),
		 L4_uid::lthread_from_gthread (_gthread));
	}
      if (_addr.lo || _addr.hi)
	{
     	  if (_gthread != (GThread_num)-1)
	    putstr(" and ");
	  else
    	    putstr(", restricted to ");
	  if (_addr.lo <= _addr.hi)
	    printf(L4_PTR_FMT" <= pfa <= "L4_PTR_FMT
		   , _addr.lo, _addr.hi);
	  else
    	    printf("pfa < "L4_PTR_FMT" || pfa > "L4_PTR_FMT, 
		   _addr.hi, _addr.lo);
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
  _addr.lo      = 0;
  _addr.hi      = 0;
}

PUBLIC static inline int Jdb_unmap_trace::log() { return _log; }
PUBLIC static inline int Jdb_unmap_trace::log_buf() { return _log_to_buf; }

PUBLIC static inline
int
Jdb_unmap_trace::check_restriction(Global_id id, Address addr)
{
  return (   (((_gthread == (GThread_num)-1)
	      || ((_other_thread) ^ (_gthread == id.gthread()))))
	  && (!(_addr.lo | _addr.hi)
	      || (_addr.lo <= _addr.hi && addr >= _addr.lo && addr <= _addr.hi)
	      || (_addr.lo >  _addr.hi && addr <  _addr.hi && addr >  _addr.lo)
	      ));
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
		 L4_uid::task_from_gthread (_gthread),
		 L4_uid::lthread_from_gthread (_gthread));
	}
      if (_addr.lo | _addr.hi)
	{
     	  if (_gthread != (GThread_num)-1)
	    putstr(" and ");
	  else
    	    putstr(", restricted to ");
	  if (_addr.lo <= _addr.hi)
	    printf(L4_PTR_FMT" <= addr <= "L4_PTR_FMT,
		   _addr.lo, _addr.hi);
	  else
    	    printf("addr < "L4_PTR_FMT" || addr > "L4_PTR_FMT
		   , _addr.hi, _addr.lo);
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
  _addr.lo      = 0;
  _addr.hi      = 0;
}

PUBLIC static inline int Jdb_nextper_trace::log() { return _log; }

PUBLIC static
void
Jdb_nextper_trace::show()
{
  if (_log)
    puts("Next period logging to tracebuffer enabled");
  else
    puts("Next period logging to tracebuffer disabled");
}

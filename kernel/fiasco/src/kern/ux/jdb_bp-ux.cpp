INTERFACE:

EXTENSION class Jdb_bp
{
};


IMPLEMENTATION[ux]:

#include <sys/types.h>			// for pid_t
#include "boot_info.h"
#include "space_index.h"
#include "usermode.h"

PUBLIC inline
Task_num
Breakpoint::restricted_task()
{
  return restrict.task.task;
}

PUBLIC inline
void
Breakpoint::set(Address _addr, Mword _len, Mode _mode, Log _log)
{
  addr = _addr;
  mode = _mode;
  user = USER;   // we don't allow breakpoints in kernel space
  log  = _log;
  len  = _len;
}

IMPLEMENT
int
Jdb_bp::global_breakpoints()
{
  return 0;
}

PUBLIC static
Mword
Jdb_bp::read_debug_register(int reg, Task_num task)
{
  pid_t pid = task ? Space_index(task).lookup()->pid()
		   : Boot_info::pid();
  Mword val;
  if (!Usermode::read_debug_register(pid, reg, val))
    printf("[read debugreg #%d task %02x failed]\n", reg, task);

  return val;
}

static
void
Jdb_bp::write_debug_register(int reg, Mword val, Task_num task)
{
  pid_t pid = task ? Space_index(task).lookup()->pid()
		   : Boot_info::pid();
  if (!Usermode::write_debug_register(pid, reg, val))
    printf("[write %08x to debugreg #%d task %02x failed]\n", val, reg, task);
}

PUBLIC static inline
Mword 
Jdb_bp::get_debug_status_register(Task_num task)
{
  return read_debug_register(6, task);
}

PUBLIC static inline
void 
Jdb_bp::reset_debug_status_register(Task_num task)
{
  write_debug_register(6, 0, task);
}

PUBLIC static inline
Mword 
Jdb_bp::get_debug_control_register(Task_num task)
{ 
  return read_debug_register(7, task);
}

PUBLIC static inline
void 
Jdb_bp::set_debug_control_register(Mword val, Task_num task)
{
  for (int i=0; i<4; i++)
    if (!(val & (2 << 2*i)))
      val &= ~(0x0f << (16 + 4*i));
  write_debug_register(7, val, task);
}

static
int
Jdb_bp::set_debug_address_register(int num, Mword addr, Mword len,
				   Breakpoint::Mode mode, Task_num task)
{
  if (task == 0)
    {
      putstr(" => task 0 (kernel) not allowed for breakpoints");
      return 0;
    }
  if (num >= 0 && num <= 3)
    {
      Mword local_dr7;
      Task_num old_task = bps[num].restricted_task();

      if (old_task != (Task_num)-1)
	{
	  // clear old breakpoint of other process
	  local_dr7 = get_debug_control_register(old_task);
	  clr_dr7(num, local_dr7);
	  set_debug_control_register(local_dr7, old_task);
	}
      bps[num].restrict_task(0, task);
      write_debug_register(num, addr, task);
      local_dr7 = get_debug_control_register(task);
      clr_dr7(num, local_dr7);
      set_dr7(num, len, mode, local_dr7);
      set_debug_control_register(local_dr7, task);
      return 1;
    }

  return 0;
}

static
void
Jdb_bp::clr_debug_address_register(int num)
{
  Task_num task = bps[num].restricted_task();
  Mword local_dr7 = get_debug_control_register(task);
  clr_dr7(num, local_dr7);
  set_debug_control_register(local_dr7, task);
}

IMPLEMENT
void
Jdb_bp::at_jdb_enter(Thread *)
{
}

IMPLEMENT
void
Jdb_bp::at_jdb_leave(Thread *t)
{
  reset_debug_status_register(t->id().task());
}


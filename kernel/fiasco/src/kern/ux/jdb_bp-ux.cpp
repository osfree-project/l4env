INTERFACE:

EXTENSION class Jdb_bp
{
private:
  static int		Jdb_bp::test_log_only();
  static int		Jdb_bp::test_break(char *errbuf, size_t bufsize);
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
  user = ADDR_USER;   // we don't allow breakpoints in kernel space
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
    printf("[write %08lx to debugreg #%d task %02x failed]\n", val, reg, task);
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
Jdb_bp::at_jdb_enter()
{}

IMPLEMENT
void
Jdb_bp::at_jdb_leave()
{
  if (Jdb::get_current_active() 
      && Jdb::get_current_active()->d_taskno() != 0)
    write_debug_register(6, 0, Jdb::get_current_active()->d_taskno());
}

IMPLEMENT
int
Jdb_bp::test_log_only()
{
  Task_num t = Jdb::get_thread()->d_taskno();
  Mword dr6  = read_debug_register(6, t);

  if (dr6 & 0x0000000f)
    {
      test_log(dr6);
      write_debug_register(6, dr6, t);
      if (!(dr6 & 0x0000e00f))
	// don't enter jdb, breakpoints only logged
	return 1;
    }
  // enter jdb
  return 0;
}

/** @return 1 if breakpoint occured */
IMPLEMENT
int
Jdb_bp::test_break(char *errbuf, size_t bufsize)
{
  Task_num t = Jdb::get_thread()->d_taskno();
  Mword dr6  = read_debug_register(6, t);

  if (!(dr6 & 0x000000f))
    return 0;

  test_break(dr6, errbuf, bufsize);
  write_debug_register(6, dr6 & ~0x0000000f, t);
  return 1;
}

IMPLEMENT
void
Jdb_bp::init_arch()
{
  Jdb::bp_test_log_only = test_log_only;
  Jdb::bp_test_break    = test_break;
}


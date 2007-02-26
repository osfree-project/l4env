INTERFACE:

EXTENSION class Jdb_bp
{
private:
  static Mword	dr7;
};

#define load_debug_register(num, val) \
    asm("movl %0, %%db" #num ";" : /* no output */ : "r" (val))

#define read_debug_register(num, val) \
    asm("movl %%db" #num ",%0;" : "=r"(val))


IMPLEMENTATION[ia32]:

#include "kmem.h"

Mword Jdb_bp::dr7;


PUBLIC inline NEEDS["kmem.h"]
void
Breakpoint::set(Address _addr, Mword _len, Mode _mode, Log _log)
{
  addr = _addr;
  mode = _mode;
  user = Kmem::is_kmem_page_fault(_addr, 0) ? KERNEL : USER;
  log  = _log;
  len  = _len;
}


IMPLEMENT
int
Jdb_bp::global_breakpoints()
{
  return 1;
}

PUBLIC static inline
Mword 
Jdb_bp::get_debug_status_register()
{
  Mword val; read_debug_register(6, val); return val;
}

PUBLIC static inline
Mword 
Jdb_bp::get_debug_control_register()
{ 
  Mword val; read_debug_register(7, val); return val;
}

static 
int
Jdb_bp::set_debug_address_register(int num, Mword addr, Mword len,
				   Breakpoint::Mode mode, Task_num /* task */)
{
  clr_dr7(num, dr7);
  set_dr7(num, len, mode, dr7);
  switch (num)
    {
    case 0: load_debug_register(0, addr); break;
    case 1: load_debug_register(1, addr); break;
    case 2: load_debug_register(2, addr); break;
    case 3: load_debug_register(3, addr); break;
    default:;
    }
  return 1;
}

static
void
Jdb_bp::clr_debug_address_register(int num)
{
  clr_dr7(num, dr7);
}

PUBLIC static inline
void 
Jdb_bp::reset_debug_status_register(Task_num /* task */)
{ 
  load_debug_register(6, 0);
}

PUBLIC static inline
void 
Jdb_bp::set_debug_control_register(Mword val)
{ 
  load_debug_register(7, val);
}

IMPLEMENT
void
Jdb_bp::at_jdb_enter(Thread *)
{
  dr7 = get_debug_control_register();
  // disable breakpoints while we are in kernel debugger
  set_debug_control_register(dr7 & ~0x0000ff00);
}

IMPLEMENT
void
Jdb_bp::at_jdb_leave(Thread *)
{
  reset_debug_status_register(/*task=*/0);
  set_debug_control_register(dr7);
}


INTERFACE:

EXTENSION class Jdb_bp
{
private:
  static int		Jdb_bp::test_sstep();
  static int		Jdb_bp::test_break(char *errbuf, size_t bufsize);
  static int		Jdb_bp::test_other(char *errbuf, size_t bufsize);
  static int		Jdb_bp::test_log_only();
  static Mword		dr7;
};

#define write_debug_register(num, val) \
    asm volatile("movl %0, %%db" #num ";" : /* no output */ : "r" (val))

#define read_debug_register(num) \
    ({Mword val; asm volatile("movl %%db" #num ",%0;" : "=r"(val)); val;})


IMPLEMENTATION[ia32]:

#include "kmem.h"

Mword Jdb_bp::dr7;

PUBLIC inline NEEDS["kmem.h"]
void
Breakpoint::set(Address _addr, Mword _len, Mode _mode, Log _log)
{
  addr = _addr;
  mode = _mode;
  user = Kmem::is_kmem_page_fault(_addr, 0) ? ADDR_KERNEL : ADDR_USER;
  log  = _log;
  len  = _len;
}

PUBLIC static
Mword
Jdb_bp::get_dr(Mword i)
{
  switch (i)
    {
    case 0: return read_debug_register(0);
    case 1: return read_debug_register(1);
    case 2: return read_debug_register(2);
    case 3: return read_debug_register(3);
    case 6: return read_debug_register(6);
    case 7: return dr7;
    default: return 0;
    }
}

IMPLEMENT
int
Jdb_bp::global_breakpoints()
{
  return 1;
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
    case 0: write_debug_register(0, addr); break;
    case 1: write_debug_register(1, addr); break;
    case 2: write_debug_register(2, addr); break;
    case 3: write_debug_register(3, addr); break;
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

IMPLEMENT
void
Jdb_bp::at_jdb_enter()
{
  dr7 = read_debug_register(7);
  // disable breakpoints while we are in kernel debugger
  write_debug_register(7, dr7 & 0x0000ff00);
}

IMPLEMENT
void
Jdb_bp::at_jdb_leave()
{
  write_debug_register(6, 0);
  write_debug_register(7, dr7);
}

/** @return 1 if single step occured */
IMPLEMENT
int
Jdb_bp::test_sstep()
{
  Mword dr6 = read_debug_register(6);
  if (!(dr6 & 0x00004000))
    return 0;

  // single step has highest priority, don't consider other conditions
  write_debug_register(6, 0);
  return 1;
}

/** @return 1 if breakpoint occured */
IMPLEMENT
int
Jdb_bp::test_break(char *errbuf, size_t bufsize)
{
  Mword dr6 = read_debug_register(6);
  if (!(dr6 & 0x000000f))
    return 0;

  int ret = test_break(dr6, errbuf, bufsize);
  write_debug_register(6, dr6 & ~0x0000000f);
  return ret;
}

/** @return 1 if other debug exception occured */
IMPLEMENT
int
Jdb_bp::test_other(char *errbuf, size_t bufsize)
{
  Mword dr6 = read_debug_register(6);
  if (!(dr6 & 0x0000e00f))
    return 0;

  snprintf(errbuf, bufsize, "unknown trap 1 (dr6="L4_PTR_FMT")", dr6);
  write_debug_register(6, 0);
  return 1;
}

/** @return 0 if only breakpoints were logged and jdb should not be entered */
IMPLEMENT
int
Jdb_bp::test_log_only()
{
  Mword dr6 = read_debug_register(6);

  if (dr6 & 0x0000000f)
    {
      test_log(dr6);
      write_debug_register(6, dr6);
      if (!(dr6 & 0x0000e00f))
	// don't enter jdb, breakpoints only logged
	return 1;
    }
  // enter jdb
  return 0;
}

IMPLEMENT
void
Jdb_bp::init_arch()
{
  Jdb::bp_test_log_only = test_log_only;
  Jdb::bp_test_break    = test_break;
  Jdb::bp_test_sstep    = test_sstep;
  Jdb::bp_test_other    = test_other;
}


INTERFACE:

#include "types.h"
#include "l4_types.h"

class Thread;
class Jdb_entry_frame;
class Jdb_at;

class Breakpoint
{
public:
  enum Mode { INSTRUCTION=0, WRITE=1, PORTIO=2, ACCESS=3 };
  enum Log  { BREAK=0, LOG=1 };

private:
  typedef struct 
    {
      int other;
      GThread_num thread; 
    } Bp_thread_res;

  typedef struct
    {
      int other;
      Task_num task;
    } Bp_task_res;

  typedef struct 
    {
      char reg;
      Address y, z;
    } Bp_reg_res;

  typedef struct 
    {
      unsigned char len;
      Address addr;
      Address y, z;
    } Bp_mem_res;
  
  typedef struct 
    {
      Bp_thread_res thread;
      Bp_task_res   task;
      Bp_reg_res    reg;
      Bp_mem_res    mem;
    } Restriction;

  Address      addr;
  Unsigned8    len;
  Address_type user;
  Mode         mode;
  Log          log;
  Restriction  restrict;
  static char const * const mode_names[4];
};

class Jdb_bp 
{
public:
  static int		global_breakpoints();

private:
  static void		at_jdb_enter(Thread *t);
  static void		at_jdb_leave(Thread *t);
  static Breakpoint	bps[4];
};


IMPLEMENTATION[ia32-ux]:

#include <cstdio>
#include <flux/x86/base_trap.h>

#include "jdb.h"
#include "jdb_input.h"
#include "jdb_module.h"
#include "kmem.h"
#include "l4_types.h"
#include "static_init.h"
#include "thread.h"

class Jdb_set_bp : public Jdb_module, public Jdb_input_task_addr
{
private:
  static char     breakpoint_cmd;
  static char     breakpoint_restrict_cmd;
  static Mword    breakpoint_number;
  static Mword    breakpoint_length;
  static Task_num breakpoint_restrict_task;
  static L4_uid   breakpoint_restrict_thread;
  typedef struct
    {
      char        reg;
      Mword       low;
      Mword       high;
    } Restrict_reg;
  static Restrict_reg breakpoint_restrict_reg;
  typedef struct
    {
      Address     addr;
      Mword       low;
      Mword       high;
    } Restrict_addr;
  static Restrict_addr breakpoint_restrict_addr;
  static int      state;
};

Breakpoint Jdb_bp::bps[4];

char const * const Breakpoint::mode_names[4] =
{
  "instruction", "write access", "i/o access", "r/w access"
};

char     Jdb_set_bp::breakpoint_cmd;
char     Jdb_set_bp::breakpoint_restrict_cmd;
Mword    Jdb_set_bp::breakpoint_number;
Mword    Jdb_set_bp::breakpoint_length;
Task_num Jdb_set_bp::breakpoint_restrict_task;
L4_uid   Jdb_set_bp::breakpoint_restrict_thread;
Jdb_set_bp::Restrict_reg  Jdb_set_bp::breakpoint_restrict_reg;
Jdb_set_bp::Restrict_addr Jdb_set_bp::breakpoint_restrict_addr;
int      Jdb_set_bp::state;

PUBLIC
Breakpoint::Breakpoint()
{
  restrict.thread.thread = (GThread_num)-1;
  restrict.task.task     = (Task_num)-1;
}

PUBLIC inline NOEXPORT
void
Breakpoint::kill()
{
  addr = 0;
}

PUBLIC inline NOEXPORT
int
Breakpoint::unused()
{
  return addr == 0;
}

PUBLIC inline NOEXPORT
int
Breakpoint::break_at_instruction()
{
  return mode == INSTRUCTION;
}

PUBLIC inline NOEXPORT
int
Breakpoint::match_addr(Address virt, Mode m)
{
  return !unused() && addr == virt && mode == m;
}

PUBLIC inline NOEXPORT
void
Breakpoint::set_logmode(char m)
{
  log = (m == '*') ? LOG : BREAK;
}

PUBLIC inline NOEXPORT
int
Breakpoint::is_break()
{
  return !unused() && log == BREAK;
}

PUBLIC inline
void
Breakpoint::restrict_task(int other, Task_num task)
{
  restrict.task.other = other;
  restrict.task.task  = task;
}

PUBLIC inline NOEXPORT
void
Breakpoint::restrict_thread(int other, GThread_num thread)
{
  restrict.thread.other  = other;
  restrict.thread.thread = thread;
}

PUBLIC inline NOEXPORT
void
Breakpoint::restrict_register(char reg, Mword y, Mword z)
{
  restrict.reg.reg = reg;
  restrict.reg.y   = y;
  restrict.reg.z   = z;
}

PUBLIC inline NOEXPORT
void
Breakpoint::restrict_memory(Mword addr, Mword len, Mword y, Mword z)
{
  restrict.mem.addr = addr;
  restrict.mem.len  = len;
  restrict.mem.y    = y;
  restrict.mem.z    = z;
}

PUBLIC inline NOEXPORT
void 
Breakpoint::clear_restriction()
{
  restrict.thread.thread = (GThread_num)-1;
  restrict.task.task     = (Task_num)-1;
  restrict.reg.reg       = 0;
  restrict.mem.len       = 0;
}

PUBLIC
void 
Breakpoint::show()
{
  if (addr) 
    {
      printf("%5s on %12s at %08x",
	     log ? "LOG" : "BREAK", mode_names[mode & 3], addr);
      if (mode != INSTRUCTION)
	printf(" len %d", len);
      else
	putstr("      ");

      if (   restrict.thread.thread == (GThread_num)-1 
	  && restrict.task.task == (Task_num)-1
	  && restrict.reg.reg == 0
	  && restrict.mem.len == 0)
	puts(" (not restricted)");
      else
	{
	  int j = 0;

	  printf("\n%32s", "restricted to ");
	  if (restrict.thread.thread != (GThread_num)-1)
	    {
	      j++;
	      printf("thread%s %x.%x\n",
		     restrict.thread.other ? " !=" : "",
		     restrict.thread.thread / L4_uid::threads_per_task(), 
		     restrict.thread.thread % L4_uid::threads_per_task());
	    }
	  if (restrict.task.task != (Task_num)-1)
	    {
	      if (j++)
		printf("%32s", "and ");
	      printf("task%s %x\n",
		     restrict.task.other ? " !=" : "",
		     restrict.task.task);
	    }
	  if (restrict.reg.reg != 0)
	    {
	      if (j++)
		printf("%32s", "and ");
	      printf("register %s in [%08x, %08x]\n",
		  (restrict.reg.reg > 0) && (restrict.reg.reg < 10) 
		      ? Jdb::reg_names[restrict.reg.reg-1] 
		      : "???",
		   restrict.reg.y, restrict.reg.z);
	    }
	  if (restrict.mem.len != 0)
	    {
	      if (j++)
		printf("%32s", "and ");
	      printf("%d-byte var at %08x in [%08x, %08x]\n", 
		  restrict.mem.len, restrict.mem.addr, 
		  restrict.mem.y,   restrict.mem.z);
	    }
	}
    }
  else
    puts("disabled");
}

// return TRUE  if the breakpoint does NOT match
// return FALSE if all restrictions do match
PUBLIC
int
Breakpoint::restricted(Jdb_entry_frame *e, Thread *t)
{
  // investigate for thread restriction
  if (restrict.thread.thread != (GThread_num)-1)
    {
      if (restrict.thread.other ^ (restrict.thread.thread != t->id().gthread()))
	return 1;
    }

  // investigate for task restriction
  if (restrict.task.task != (Task_num)-1)
    {
      if (restrict.task.other ^ (restrict.task.task != t->id().task()))
	return 1;
    }

  // investigate for register restriction
  if (restrict.reg.reg)
    { 
      Mword val = 0;
      Mword y   = restrict.reg.y;
      Mword z   = restrict.reg.z;

      switch (restrict.reg.reg)
	{
	case 1:  val = e->eax; break;
	case 2:  val = e->ebx; break;
	case 3:  val = e->ecx; break;
	case 4:  val = e->edx; break;
	case 5:  val = e->ebp; break;
	case 6:  val = e->esi; break;
	case 7:  val = e->edi; break;
	case 8:  val = e->eip; break;
	case 9:  val = e->esp; break;
	case 10: val = e->eflags; break;
	}

      // return true if rules do NOT match
      if (  ((y <= z) && ((val <  y) || (val >  z)))
	  ||((y >  z) && ((val >= z) || (val <= y))))
	return 1;
    }

  // investigate for variable restriction
  if (restrict.mem.len)
    { 
      Mword val = 0;
      Mword y   = restrict.mem.y;
      Mword z   = restrict.mem.z;

      if (restrict.mem.len == 1)
	val = *(Unsigned8  *)Kmem::phys_to_virt(restrict.mem.addr);
      else if (restrict.mem.len == 2)
	val = *(Unsigned16 *)Kmem::phys_to_virt(restrict.mem.addr);
      else if (restrict.mem.len == 4)
	val = *(Unsigned32 *)Kmem::phys_to_virt(restrict.mem.addr);

      // return true if rules do NOT match
      if ((y <= z) ^ ((val >= y) && (val <= z)))
	return 1;
    }

  return 0;
}

PUBLIC
int
Breakpoint::test_break(Jdb_entry_frame *e, Thread *t, char *&error_buffer)
{
  if (!restricted(e, t))
    {
      sprintf(error_buffer, "break on %s at %08x", mode_names[mode], addr);
      if (mode==WRITE || mode==ACCESS)
	{
 	  // If it's a write or access (read) breakpoint, we look at the
	  // appropriate place and print the bytes we find there. We do
	  // not need to look if the page is present because the x86 CPU
	  // enters the debug exception immediately _after_ the memory
	  // access was performed.
	  char *msgptr = error_buffer + strlen(error_buffer);
	  switch (len)
	    {
	    case 1: sprintf(msgptr, " [%02x]", 
			    t->peek((Unsigned8* )addr, user)); break;
	    case 2: sprintf(msgptr, " [%04x]", 
			    t->peek((Unsigned16*)addr, user)); break;
	    case 4: sprintf(msgptr, " [%08x]", 
			    t->peek((Unsigned32*)addr, user)); break;
	    }
	}
      return 1;
    }
  return 0;
}

// Create log entry if breakpoint matches
PUBLIC
void
Breakpoint::test_log(Jdb_entry_frame *e, Thread *t)
{
  if (log && !restricted(e, t))
    {
      // log breakpoint
      Mword value = 0;

      if (mode == WRITE || mode == ACCESS)
	{
	  // If it's a write or access (read) breakpoint, we look at the
	  // appropriate place and print the bytes we find there. We do
	  // not need to look if the page is present because the x86 CPU
	  // enters the debug exception immediately _after_ the memory
	  // access was performed.
	  switch (len)
    	    {
	    case 1: value = t->peek((Unsigned8* )addr, user); break;
	    case 2: value = t->peek((Unsigned16*)addr, user); break;
	    case 4: value = t->peek((Unsigned32*)addr, user); break;
	    }
	}

      // is called with disabled interrupts
      Tb_entry_bp *tb = static_cast<Tb_entry_bp*>(Jdb_tbuf::new_entry());
      tb->set(t, e->eip, mode, len, value, addr);
      Jdb_tbuf::commit_entry();
    }
}

static
void
Jdb_bp::init()
{
  static Jdb_at_entry enter(at_jdb_enter);
  static Jdb_at_entry leave(at_jdb_leave);

  Jdb::at_enter.add(&enter);
  Jdb::at_leave.add(&leave);
}

static inline
void
Jdb_bp::clr_dr7(int num, Mword &dr7)
{
  dr7 &= ~(((3 + (3 <<2)) << (16 + 4*num)) + (3 << (2*num)));
}

static inline
void
Jdb_bp::set_dr7(int num, Mword len, Breakpoint::Mode mode, Mword &dr7)
{
  dr7 |= ((((mode & 3) + ((len-1)<<2)) << (16 + 4*num)) + (2 << (2*num)));
}

PUBLIC static
int
Jdb_bp::set_breakpoint(int num, Address addr, Mword len,
		       Breakpoint::Mode mode, Breakpoint::Log log,
		       Task_num task)
{
  if (set_debug_address_register(num, addr, len, mode, task))
    {
      bps[num].set(addr, len, mode, log);
      return 1;
    }

  return 0;
}

PUBLIC static 
void 
Jdb_bp::clr_breakpoint(int num)
{
  clr_debug_address_register(num);
  bps[num].kill();
}

PUBLIC static inline NOEXPORT
void
Jdb_bp::logmode_breakpoint(int num, char mode)
{
  bps[num].set_logmode(mode);
}

PUBLIC static
int
Jdb_bp::first_unused()
{
  int i;

  for (i=0; i<4 && !bps[i].unused(); i++)
    ;

  return i;
}

// Check if an instruction breakpoint matches. If so, set the resume flag
// on the entry frame to prevent looping in the same breakpoint.
PUBLIC static
void
Jdb_bp::handle_instruction_breakpoint(Jdb_entry_frame *e, Mword dr6)
{
  for (int i=0; i<4; i++)
    if ((dr6 & (1<<i)) && bps[i].break_at_instruction())
      {
	e->eflags |= EFLAGS_RF;
	return;
      }
}

// Return 1 if a breakpoint hits
PUBLIC static
int
Jdb_bp::test_break(Jdb_entry_frame *e, Thread *t, Mword dr6,
		   char *&error_buffer)
{
  for (int i=0; i<4; i++)
    if (dr6 & (1<<i))
      if (bps[i].test_break(e, t, error_buffer))
	return 1;

  return 0;
}

// Create log entry if breakpoint matches
// Return 1 if debugger should stop
PUBLIC static
int
Jdb_bp::test_log(Jdb_entry_frame *e, Thread *t, Mword dr6)
{
  int do_break = 0;

  for (int i=0; i<4; i++)
    if (dr6 & (1<<i))
      {
	do_break |= bps[i].is_break();
	bps[i].test_log(e, t);
      }

  return do_break;
}

PUBLIC static
Mword
Jdb_bp::test_match(Address addr, Breakpoint::Mode mode)
{
  for (int i=0; i<4; i++)
    if (bps[i].match_addr(addr, mode))
      return i+1;

  return 0;
}

PUBLIC static inline NOEXPORT
void
Jdb_bp::restrict_task(int num, int other, Task_num task)
{
  bps[num].restrict_task(other, task);
}

PUBLIC static inline NOEXPORT
void
Jdb_bp::restrict_thread(int num, int other, GThread_num thread)
{
  bps[num].restrict_thread(other, thread);
}

PUBLIC static inline NOEXPORT
void
Jdb_bp::restrict_register(int num, char reg, Mword y, Mword z)
{
  bps[num].restrict_register(reg, y, z);
}

PUBLIC static inline NOEXPORT
void
Jdb_bp::restrict_memory(int num, Mword addr, Mword len, Mword y, Mword z)
{
  bps[num].restrict_memory(addr, len, y, z);
}

PUBLIC static inline NOEXPORT
void
Jdb_bp::clear_restriction(int num)
{
  bps[num].clear_restriction();
}

PUBLIC static
void 
Jdb_bp::list()
{
  putchar('\n');

  for(int i=0; i<4; i++)
    {
      printf("  #%d: ", i+1);
      bps[i].show();
    }

  putchar('\n');
}


//---------------------------------------------------------------------------//

PUBLIC
Jdb_module::Action_code
Jdb_set_bp::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  Jdb_module::Action_code code;
  Breakpoint::Mode mode;

  if (cmd == 0)
    {
      if (args == &breakpoint_cmd)
	{
	  switch (breakpoint_cmd)
	    {
	    case 'p':
	      if (!(Cpu::features() & FEAT_DE))
		{
		  puts(" I/O breakpoints not supported by this CPU");
		  return NOTHING;
		}
	      // fall through
	    case 'a':
	    case 'i':
	    case 'w':
	      if ((breakpoint_number = Jdb_bp::first_unused()) < 4)
		{
		  fmt   = " addr=%C";
		  args  = &Jdb_input_task_addr::first_char;
		  state = 1; // breakpoints are global for all tasks
		  return EXTRA_INPUT;
		}
	      puts(" No breakpoints available");
	      return NOTHING;
	    case 'l':
	      // show all breakpoints
	      Jdb_bp::list();
	      return NOTHING;
	    case '-':
	      // delete breakpoint
	    case '+':
	      // set logmode of breakpoint to <STOP>
	    case '*':
	      // set logmode of breakpoint to <LOG>
	    case 'r':
	      // restrict breakpoint
	      fmt   = " bpn=%1x";
	      args  = &breakpoint_number;
	      state = 2;
	      return EXTRA_INPUT;
	    case 't':
	      Jdb::execute_command("bt");
	      break;
	    }
	}
      else switch (state)
	{
	case 1:
	  code = Jdb_input_task_addr::action(args, fmt, next_char);
	  if (code == LEAVE)
	    // abort
	    return NOTHING;
	  if (code == NOTHING)
	    // ok, continue
	    goto got_address;
	  // more input for Jdb_input_task_addr
	  return code;
	case 2:
	  if (breakpoint_number < 1 || breakpoint_number > 4)
	    return NOTHING;
	  // input is 1..4 but numbers are 0..3
	  breakpoint_number -= 1;
	  // we know the breakpoint number
	  switch (breakpoint_cmd)
	    {
	    case '-':
	      Jdb_bp::clr_breakpoint(breakpoint_number);
	      putchar('\n');
	      return NOTHING;
	    case '+':
	    case '*':
	      Jdb_bp::logmode_breakpoint(breakpoint_number, breakpoint_cmd);
	      putchar('\n');
	      return NOTHING;
	    case 'r':
	      fmt   = " %C";
	      args  = &breakpoint_restrict_cmd;
	      state = 5;
	      return EXTRA_INPUT;
	    }
	  break;
	case 3:
got_address:
	  // address/task read
	  if (breakpoint_cmd != 'i')
	    {
	      fmt   = " len (1, 2 or 4)=%1x";
	      args  = &breakpoint_length;
	      state = 4;
	      return EXTRA_INPUT;
	    }
	  breakpoint_length = 1; // must be 1 for instruction breakpoints
	  // fall through
	case 4:
	  // length read
	  if (breakpoint_length != 1 && breakpoint_length != 2 && 
	      breakpoint_length != 4)
	    break;
      	  switch (breakpoint_cmd)
	    {
	    default :
	    case 'i': mode = Breakpoint::INSTRUCTION; break;
	    case 'w': mode = Breakpoint::WRITE;       break;
	    case 'p': mode = Breakpoint::PORTIO;      break;
	    case 'a': mode = Breakpoint::ACCESS;      break;
	    }
	  // abort if no address was given
	  if (Jdb_input_task_addr::addr == (Address)-1)
	    return NOTHING;
	  Jdb_bp::set_breakpoint(breakpoint_number, Jdb_input_task_addr::addr,
				 breakpoint_length, mode, Breakpoint::BREAK, 
				 Jdb_input_task_addr::task);
	  putchar('\n');
	  break;
	case 5:
	  // restrict command read
	  switch (breakpoint_restrict_cmd)
	    {
	    case 'a':
	    case 'A':
	      fmt   = (breakpoint_restrict_cmd=='A') 
			? "task!=%3x\n" : "task==%3x\n";
	      args  = &breakpoint_restrict_task;
	      state = 6;
	      return EXTRA_INPUT;
	    case 't':
	    case 'T':
	      fmt   = (breakpoint_restrict_cmd=='T')
			? "thread!=%t\n" : "thread==%t\n";
	      args  = &breakpoint_restrict_thread;
	      state = 7;
	      return EXTRA_INPUT;
	    case 'e':
	      return NOTHING;
	    case '1':
	    case '2':
	    case '4':
	      putchar(breakpoint_restrict_cmd);
	      fmt   = "-byte addr=%8x between[%8x-%8x]\n";
	      args  = &breakpoint_restrict_addr;
	      state = 9;
	      return EXTRA_INPUT;
	    case '-':
	      Jdb_bp::clear_restriction(breakpoint_number);
	      putchar('\n');
	      break;
	    }
	  break;
	case 6:
	  // breakpoint restrict task read
	  Jdb_bp::restrict_task(breakpoint_number,
				breakpoint_restrict_cmd == 'A',
				breakpoint_restrict_task);
	  break;
	case 7:
	  // breakpoint restrict thread read
	  Jdb_bp::restrict_thread(breakpoint_number,
				  breakpoint_restrict_cmd == 'T',
				  breakpoint_restrict_thread.gthread());
	  break;
	case 8:
	  // breakpoint restrict register in range
	  Jdb_bp::restrict_register(breakpoint_number,
				    breakpoint_restrict_reg.reg,
				    breakpoint_restrict_reg.low,
				    breakpoint_restrict_reg.high);
	  break;
	case 9:
	  // breakpoint restrict x-byte-value in range
	  Jdb_bp::restrict_memory(breakpoint_number,
				  breakpoint_restrict_addr.addr,
				  breakpoint_restrict_cmd - '0',
				  breakpoint_restrict_addr.low,
				  breakpoint_restrict_addr.high);
	  break;
	}
    }
  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_set_bp::cmds() const
{
  static Cmd cs[] = 
    {
      Cmd (0, "b", "bp", "%c",
	  "b{i|a|w|p}<addr>\tset breakpoint on instruction/access/write/io "
	  "access\n"
	  "b{-|+|*}<num>\tdisable/enable/log breakpoint\n"
	  "br<num>{t|T|a|A|e|1|2|4}\trestrict breakpoint to "
	  "(!)thread/(!)task/reg/mem",
	  &breakpoint_cmd),
    };

  return cs;
}

PUBLIC
int const
Jdb_set_bp::num_cmds() const
{
  return 1;
}

static Jdb_set_bp jdb_set_bp INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

// external callable
int
jdb_instruction_bp_at_addr(Address addr)
{ return Jdb_bp::test_match(addr, Breakpoint::INSTRUCTION); }


IMPLEMENTATION[ia32,ux]:

#include <cstdio>
#include <cctype>

#include "config.h"
#include "jdb.h"
#include "jdb_input.h"
#include "jdb_lines.h"
#include "jdb_module.h"
#include "jdb_symbol.h"
#include "mem_layout.h"
#include "keycodes.h"
#include "thread.h"

class Jdb_bt : public Jdb_module, public Jdb_input_task_addr
{
  static char     dummy;
  static char     first_char;
  static char     first_char_addr;
  static Address  addr;
  static Global_id tid;
  static Task_num task;
};

char      Jdb_bt::dummy;
char      Jdb_bt::first_char;
char      Jdb_bt::first_char_addr;
Address   Jdb_bt::addr;
Global_id Jdb_bt::tid;
Task_num  Jdb_bt::task;

// determine the user level ebp and eip considering the current thread state
static void
Jdb_bt::get_user_eip_ebp(Address &eip, Address &ebp)
{ 
  if (task == 0)
    {
      // kernel thread doesn't execute user code -- at least we hope so :-)
      ebp = eip = 0;
      return;
    }

  Thread *t        = Thread::lookup(tid);
  Address tcb_next = (Address)context_of(t->get_kernel_sp()) + Context::size;
  Mword *ktop      = (Mword *)tcb_next;
  Jdb::Guessed_thread_state state = Jdb::guess_thread_state(t);

  if (state == Jdb::s_ipc)
    {
      // If thread is in IPC, EBP lays on the stack (see C-bindings). EBP
      // location is by now dependent from ipc-type, maybe different for
      // other syscalls, maybe unstable during calls but only for user EBP.
      Mword entry_esp = ktop[-2];
      Mword entry_ss  = ktop[-1];

      if (entry_ss & 3)
	{
	  // kernel entered from user level

#ifdef CONFIG_ABI_X0
	  // see X0 C-bindings
	  entry_esp += sizeof(Mword);
#endif
	  if (Jdb::peek_addr_task(entry_esp, task, &ebp) == -1)
	    {
	      printf("\n esp page invalid");
	      ebp = eip = 0;
	      return;
	    }
	}
    }
  else if (state == Jdb::s_pagefault)
    {
      // see pagefault handler gate stack layout
      ebp = ktop[-6];
    }
  else if (state == Jdb::s_slowtrap)
    {
      // see slowtrap handler gate stack layout
      ebp = ktop[-13];
    }
  else
    {
      // Thread is doing (probaly) no IPC currently so we guess the
      // user ebp by following the kernel ebp upwards. Some kernel
      // entry pathes (e.g. timer interrupt) push the ebp register
      // like gcc.
      ebp = get_user_ebp_following_kernel_stack();
    }

  eip = ktop[-5];
}

static Mword
Jdb_bt::get_user_ebp_following_kernel_stack()
{
  if (!Config::Have_frame_ptr)
    return 0;

  Mword ebp, dummy;

  get_kernel_eip_ebp(dummy, dummy, ebp);

  for (int i=0; i<30 /* sanity check */; i++)
    {
      Mword m1, m2;

      if (  (ebp == 0)
	  ||(Jdb::peek_addr_task(ebp,   0 /*kernel*/, &m1) == -1)
	  ||(Jdb::peek_addr_task(ebp+4, 0 /*kernel*/, &m2) == -1))
	// invalid ebp -- leaving
	return 0;

      ebp = m1;

      if (!Mem_layout::in_kernel_code(m2))
	{
	  if (m2 < Kmem::mem_user_max)
	    // valid user ebp found
	    return m1;
	  else
	    // invalid ebp
	    return 0;
	}
    }

  return 0;
}

static void
Jdb_bt::get_kernel_eip_ebp(Mword &eip1, Mword &eip2, Mword &ebp)
{
  if (tid == Jdb::get_current_active()->id())
    {
      ebp  = (Mword)__builtin_frame_address(3);
      eip1 = eip2 = 0;
    }
  else
    {
      Mword *ksp = (Mword*) Thread::lookup(tid)->get_kernel_sp();
      Mword tcb  = Mem_layout::Tcbs + tid.gthread()*Context::size;
      Mword tcb_next = tcb + Context::size;

      // search for valid ebp/eip
      for (int i=0; (Address)(ksp+i+1)<tcb_next-20; i++)
	{
	  if (Mem_layout::in_kernel_code(ksp[i+1]) &&
	      ksp[i] >= tcb+0x180 && 
	      ksp[i] <  tcb_next-20 &&
	      ksp[i] >  (Address)(ksp+i))
	    {
	      // valid frame pointer found
	      ebp  = ksp[i  ];
	      eip1 = ksp[i+1];
	      eip2 = tid != Jdb::get_current_active()->id() ? ksp[0] : 0;
	      return;
	    }
	}
      ebp = eip1 = eip2 = 0;
    }
}

/** Show one backtrace item we found. Add symbol name and line info */
static void
Jdb_bt::show_item(int nr, Address addr, Address_type user)
{
  char buffer[74];

  printf(" %s#%d  "L4_PTR_FMT"", nr<10 ? " ": "", nr, addr);

  Address sym_addr = addr;
  if (Jdb_symbol::match_addr_to_symbol_fuzzy(&sym_addr, 
					     user == ADDR_KERNEL ? 0 : task, 
				     	     buffer,
					     60 < sizeof(buffer) ? 60 : sizeof(buffer))
      // if the previous symbol is to far away assume that there is no
      // symbol for that entry
      && (addr-sym_addr < 1024))
    {
      printf(" : %s", buffer);
      if (addr-sym_addr)
	printf(" %s+ 0x%lx\033[m", Jdb::esc_line, addr-sym_addr);
    }

  // search appropriate line backwards starting from addr-1 because we
  // don't want to see the line info for the next statement after the
  // call but the line info for the call itself
  Address line_addr = addr-1;
  if (Jdb_lines::match_addr_to_line_fuzzy(&line_addr, 
					  user == ADDR_KERNEL ? 0 : task,
				     	  buffer, sizeof(buffer)-1, 0)
      // if the previous line is to far away assume that there is no
      // line for that entry
      && (addr-line_addr < 128))
    printf("\n%6s%s%s\033[m", "", Jdb::esc_line, buffer);

  putchar('\n');
}

static void
Jdb_bt::show_without_ebp()
{
  Mword *ksp      = (Mword*) Thread::lookup(tid)->get_kernel_sp();
  Mword tcb_next  = Mem_layout::Tcbs + (tid.gthread()+1)*Context::size;

  // search for valid eip
  for (int i=0, j=1; (unsigned)(ksp+i)<tcb_next-20; i++)
    {
      if (Mem_layout::in_kernel_code(ksp[i])) 
	show_item(j++, ksp[i], ADDR_KERNEL);
    }
}

static void
Jdb_bt::show(Mword ebp, Mword eip1, Mword eip2, Address_type user)
{
  for (int i=0; i<40 /*sanity check*/; i++)
    {
      Mword m1, m2;

      if (i > 1)
	{
	  if (  (ebp == 0)
	      ||(Jdb::peek_addr_task(ebp,   task, &m1) == -1)
	      ||(Jdb::peek_addr_task(ebp+4, task, &m2) == -1))
	    // invalid ebp -- leaving
	    return;

	  ebp = m1;

	  if (  (user==ADDR_KERNEL && !Mem_layout::in_kernel_code(m2))
	      ||(user==ADDR_USER   && (m2==0 || m2>Kmem::mem_user_max)))
	    // no valid eip found -- leaving
	    return;
	}
      else if (i == 1)
	{
	  if (eip1 == 0)
	    continue;
	  m2 = eip1;
	}
      else
	{
	  if (eip2 == 0)
	    continue;
	  m2 = eip2;
	}

      show_item(i, m2, user);
    }
}

PUBLIC
Jdb_module::Action_code
Jdb_bt::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  if (cmd == 0)
    {
      Address eip, ebp;

      if (args == &dummy)
	{
	  // default value for thread
	  tid  = Jdb::get_current_active()->id();
	  fmt  = "%C";
	  args = &first_char;
	  return EXTRA_INPUT;
	}
      else if (args == &first_char)
	{
	  if (first_char == 't')
	    {
	      putstr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\033[K");
	      fmt  = " thread=%t";
	      args = &tid;
	      return EXTRA_INPUT;
	    }
	  else if (first_char == 'a')
	    {
	      putstr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\033[K");
	      fmt  = " addr=%C";
	      args = &Jdb_input_task_addr::first_char;
	      return EXTRA_INPUT;
	    }
	  else if (first_char != KEY_RETURN && first_char != ' ')
	    {
	      // ignore wrong input
	      fmt  = "%C";
	      return EXTRA_INPUT;
	    }
	  else
	    // backtrace from current thread
	    goto start_backtrace;
	}
      else if (args == &tid)
	{
	  if (!Thread::lookup(tid)->is_valid())
	    {
	      puts(" Invalid thread");
	      return NOTHING;
	    }

start_backtrace:
	  task = Thread::get_task (tid);
	  get_user_eip_ebp(eip, ebp);

start_backtrace_known_ebp:
	  printf("\n\nbacktrace (thread %x.%02x, fp="L4_PTR_FMT
	         ", pc="L4_PTR_FMT"):\n",
	      task, tid.d_thread(), ebp, eip);
	  if (task != 0)
	    show(ebp, eip, 0, ADDR_USER);
	  if (!Config::Have_frame_ptr)
	    {
	      puts("\n --kernel-bt-follows-- "
		   "(don't trust w/o frame pointer!!)");
	      task = 0;
	      show_without_ebp();
	    }
	  else
	    {
	      Mword eip2;
	      puts("\n --kernel-bt-follows--");
	      get_kernel_eip_ebp(eip, eip2, ebp);
	      task = 0;
	      show(ebp, eip, eip2, ADDR_KERNEL);
	    }
	  putchar('\n');
	}
      else 
	{
	  Jdb_module::Action_code code;
	  
	  switch ((code = Jdb_input_task_addr::action(args, fmt, next_char)))
	    {
	    case ERROR:
	      return ERROR;
	    case NOTHING:
	      task = Jdb_input_task_addr::task;
	      tid.d_task (task);
	      eip  = 0;
	      ebp  = Jdb_input_task_addr::addr;
	      goto start_backtrace_known_ebp;
	    default:
	      return code;
	    }
	}
    }
  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_bt::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "bt", "backtrace", " [a]ddr/[t]hread",
	  "bt[t<threadid>][<addr>]\tshow backtrace of current/given "
	  "thread/addr",
	  &dummy },
    };
  return cs;
}

PUBLIC
int const
Jdb_bt::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_bt::Jdb_bt()
  : Jdb_module("INFO")
{}

static Jdb_bt jdb_bt INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


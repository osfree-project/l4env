IMPLEMENTATION[ia32-ux]:

#include <cstdio>
#include <cctype>

#include "config.h"
#include "jdb.h"
#include "jdb_lines.h"
#include "jdb_module.h"
#include "jdb_symbol.h"
#include "keycodes.h"
#include "thread.h"
#include "threadid.h"

class Jdb_bt : public Jdb_module
{
  static char     dummy;
  static char     first_char;
  static Address  addr;
  static L4_uid   thread;
  static Task_num task;
};

char     Jdb_bt::dummy;
char     Jdb_bt::first_char;
Address  Jdb_bt::addr;
L4_uid   Jdb_bt::thread;
Task_num Jdb_bt::task;

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

  Thread *t        = Threadid(&thread).lookup();
  Address ksp      = (Mword) t->kernel_sp;
  Address tcb_next = (ksp & ~(Config::thread_block_size-1))
		   + Config::thread_block_size;
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
#ifndef CONFIG_NO_FRAME_PTR
  Mword ebp, dummy;

  get_kernel_eip_ebp(dummy, dummy, ebp);

  for (int i=0; i<30 /* sanity check */; i++)
    {
      Mword m1, m2;

      if (  (ebp == 0)
    	  ||(ebp == Kmem::mem_phys)
	  ||(Jdb::peek_addr_task(ebp,   0 /*kernel*/, &m1) == -1)
	  ||(Jdb::peek_addr_task(ebp+4, 0 /*kernel*/, &m2) == -1))
	// invalid ebp -- leaving
	return 0;

      ebp = m1;

      if ((m2<(Address)&_start || m2>(Address)&_ecode))
	{
	  if (m2 < Kmem::mem_user_max)
	    // valid user ebp found
	    return m1;
	  else
	    // invalid ebp
	    return 0;
	}
    }
#endif

  // no suitable ebp found
  return 0;
}

#ifndef CONFIG_NO_FRAME_PTR
static void
Jdb_bt::get_kernel_eip_ebp(Mword &eip1, Mword &eip2, Mword &ebp)
{
  if (thread == Jdb::get_current_active()->id())
    {
      ebp  = (Mword)__builtin_frame_address(3);
      eip1 = eip2 = 0;
    }
  else
    {
      Mword *ksp = (Mword*) Threadid(&thread).lookup()->kernel_sp;
      Mword tcb  = Kmem::mem_tcbs + thread.gthread()*Context::size;
      Mword tcb_next = tcb + Context::size;

      // search for valid ebp/eip
      for (int i=0; (Address)(ksp+i+1)<tcb_next-20; i++)
	{
	  if (ksp[i+1] >= (Address)&_start && ksp[i+1] <= (Address)&_ecode &&
	      ksp[i  ] >= tcb+0x180         && ksp[i  ] <  tcb_next-20 &&
	      ksp[i  ] > (Address)(ksp+i))
	    {
	      // valid frame pointer found
	      ebp  = ksp[i  ];
	      eip1 = ksp[i+1];
	      eip2 = (thread != Jdb::get_current_active()->id()) ? ksp[0] : 0;
	      return;
	    }
	}
      ebp = eip1 = eip2 = 0;
    }
}
#endif

/** Show one backtrace item we found. Add symbol name and line info */
static void
Jdb_bt::show_item(int nr, Address addr, Address_type user)
{
  char symbol[60];

  printf(" %s#%d  %08x", nr<10 ? " ": "", nr, addr);

  if (Jdb_symbol::match_eip_to_symbol(addr, user == KERNEL ? 0 : task, 
				      symbol, sizeof(symbol)))
    printf(" : %s", symbol);

  // search appropriate line backwards starting from addr-1 because we
  // don't want to see the line info for the next statement after the
  // call but the line info for the call itself
  if (Jdb_lines::match_address_to_line_fuzzy(addr-1, user == KERNEL ? 0 : task,
					     symbol, sizeof(symbol)-1, 0))
    printf("\n%18s%s", "", symbol);

  putchar('\n');
}

#ifdef CONFIG_NO_FRAME_PTR
static void
Jdb_bt::show_without_ebp()
{
  Mword *ksp      = (Mword*) Threadid(&thread).lookup()->kernel_sp;
  Mword tcb_next  = Kmem::mem_tcbs + (thread.gthread()+1)*Context::size;

  // search for valid eip
  for (int i=0, j=1; (unsigned)(ksp+i)<tcb_next-20; i++)
    {
      if (ksp[i] >= (unsigned)&_start && ksp[i] <= (unsigned)&_ecode)
	show_item(j++, ksp[i], KERNEL);
    }
}
#endif

static void
Jdb_bt::show(Mword ebp, Mword eip1, Mword eip2, Address_type user)
{
  for (int i=0; i<40 /*sanity check*/; i++)
    {
      Mword m1, m2;

      if (i > 1)
	{
	  if (  (ebp == 0)
	      ||(ebp == Kmem::mem_phys)
	      ||(Jdb::peek_addr_task(ebp,   task, &m1) == -1)
	      ||(Jdb::peek_addr_task(ebp+4, task, &m2) == -1))
	    // invalid ebp -- leaving
	    return;

	  ebp = m1;

	  if (  (user==KERNEL && (m2<(Mword)&_start || m2>(Mword)&_ecode))
	      ||(user==USER   && (m2==0             || m2>Kmem::mem_user_max)))
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
      if (args == &dummy)
	{
	  // default value for thread
	  thread = Jdb::get_current_active()->id();
	  fmt  = "%C";
	  args = &first_char;
	  return EXTRA_INPUT;
	}
      else if (args == &first_char)
	{
	  if (first_char == 't')
	    {
	      fmt  = " thread=%t";
	      args = &thread;
	      return EXTRA_INPUT;
	    }
	  else if (isxdigit(first_char))
	    {
	      next_char = first_char;
	      args = &addr;
	      fmt  = "%8x";
	      return EXTRA_INPUT_WITH_NEXTCHAR;
	    }
	  else if (first_char != KEY_RETURN && first_char != ' ')
	    {
	      fmt  = "%C";
	      return EXTRA_INPUT;
	    }
	  else
	    // backtrace from current thread
	    goto start_backtrace;
	}
      else if (args == &thread)
	{
	  Address eip, ebp;

	  if (!Threadid(thread).lookup()->is_valid())
	    {
	      puts(" Invalid thread");
	      return NOTHING;
	    }

start_backtrace:
	  task = thread.task();
	  get_user_eip_ebp(eip, ebp);
	  printf("\n\nbacktrace (thread %x.%02x, fp=%08x, pc=%08x):\n",
	      task, thread.lthread(), ebp, eip);
	  if (task != 0)
	    show(ebp, eip, 0, USER);
#ifdef CONFIG_NO_FRAME_PTR
	  puts("\n  --kernel-bt-follows-- (don't trust w/o frame pointer!!)");
	  show_without_ebp();
#else
	  Mword eip2;

	  puts("\n --kernel-bt-follows--");
	  get_kernel_eip_ebp(eip, eip2, ebp);
	  show(ebp, eip, eip2, KERNEL);
#endif
	  putchar('\n');
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
      Cmd (0, "bt", "backtrace", "",
	  "bt[t<threadid>][<addr>]\tshow backtrace of current/given "
	  "thread/addr",
	  &dummy),
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


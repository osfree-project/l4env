IMPLEMENTATION [debug]:

#include <cstdio>
#include "config.h"
#include "kmem.h"
#include "mem_layout.h"
#include "simpleio.h"

IMPLEMENTATION [{ia32,ux,amd64}-debug]:
// Note that we don't want to check for Thread_invalid since we don't want
// to raise page faults from inside the kernel debugger
PUBLIC inline
int
Thread::is_mapped()
{
  return Kmem::virt_to_phys((void*)this) != (Address)-1;
}

IMPLEMENTATION [!{ia32,ux,amd64}-debug]:

#include "kmem_space.h"
#include "pagetable.h"

PUBLIC inline NEEDS["kmem_space.h","pagetable.h"]
int
Thread::is_mapped()
{ 
  return Kmem_space::kdir()->walk(this,0,false,0).valid();
} 

IMPLEMENTATION [debug]:

// check if thread is valid (i.e. valid address, thread mapped)
PUBLIC
int
Thread::is_valid()
{
  return    this != 0
	 && Kmem::is_tcb_page_fault((Address)this, 0)
	 && ((Address)this & (Config::thread_block_size-1)) == 0
	 && is_mapped()
	 && state() != Thread_invalid;
}

PUBLIC
void
Thread::print_snd_partner(int task_format=0)
{
  if (state() & Thread_send_in_progress)
    lookup(static_cast<Thread*>(receiver()))->print_uid(task_format);
  else
    // receiver() not valid
    putstr("       ");
}

// Be robust if partner is invalid
PUBLIC
void
Thread::print_partner(int task_format=0)
{
  if (!(state() & (Thread_receiving | Thread_busy |
		   Thread_rcvlong_in_progress)))
    {
      printf("%*s    ", task_format, " ");
      return;
    }

  if (!partner())
    {
      printf("%*s.** ", task_format, "*");
      return;
    }

  if (Kmem::is_tcb_page_fault((Address)partner(), 0))
    {
      Thread *p = lookup (context_of (partner()));
      char flag = partner() == p->preemption() ? 'P' : ' ';
      p->print_uid(task_format);
      putchar(flag);
      return;
    }

#if 0
  // does not work with ARM
  if (Kmem::virt_to_phys((void*)partner()) != (Address)-1)
    // IRQ thread
    partner()->id().print(task_format);
  else
    // not mapped => bogus
    putstr("\033[31;1m???.??\033[m");
#else
  partner()->id().print(task_format);
#endif
  putchar(' ');
}

// Be robust if this object is invalid
PUBLIC
void
Thread::print_uid(int task_format=0)
{
  if (!this)
    {
      putstr("---.--");
      return;
    }

  if (is_valid())
    {
      id().print(task_format);
      return;
    }

  if (!Kmem::is_tcb_page_fault((Address)this, 0)
      || ((Address)this & (Config::thread_block_size-1)))
    {
      putstr("\033[31;1m???.??\033[m");
      return;
    }

  // address inside tcb
  putstr("\033[31;1m");
  Global_id (this, Mem_layout::Tcbs,
 	     Config::thread_block_size).print (task_format);
  putstr("\033[m");
}

PUBLIC
void
Thread::print_state_long (unsigned cut_on_len = 0)
{
  static char const * const state_names[] = 
    { 
      "ready",         "utcb",          "rcv",         "poll",
      "ipc_progr",     "snd_progr",     "busy",        "lipc_ok",
      "cancel",        "dead",          "poll_long",   "busy_long", 
      "rcvlong_progr", "delayed_deadl", "delayed_ipc", "fpu", 
      "alien",         "dealien",       "exc_progr",   "transfer"
    };

  Mword i, comma=0, chars=0, bits=state();
  
  for (i = 0; i < sizeof (state_names) / sizeof (char *); i++, bits >>= 1)
    {
      if (!(bits & 1))
        continue;
        
      if (cut_on_len)
        {
          unsigned add = strlen (state_names[i]) + comma;
          if (chars + add > cut_on_len)
            {
              if (chars < cut_on_len - 4)
                putstr(",...");
              break;
            }
          chars += add;
        }

      printf ("%s%s", "," + !comma, state_names[i]);

      comma = 1;
    }
}

PUBLIC static inline
Task_num
Thread::get_task (Global_id id)
{ return id.task(); }

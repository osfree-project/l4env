INTERFACE:

EXTENSION class Thread
{
};


IMPLEMENTATION[debug]:

#include <cstdio>
#include "simpleio.h"

PUBLIC void Thread::print_send_partner(int task_format=0)
{
  Thread::lookup(context_of(receiver()))->print_uid(task_format);
}

PUBLIC void Thread::print_partner(int task_format=0)
{
  if(partner() && (state() & (Thread_receiving | Thread_busy | 
                              Thread_rcvlong_in_progress)))
    partner()->id().print(task_format);
  else
    putstr("---.--");
}

PUBLIC void Thread::print_uid(int task_format=0)
{
  if (this)
    {
      if (is_valid())
	id().print(task_format);
      else
	{
	  putstr("\033[31;1m");
	  putstr("???.??\033[m");
	}
    }
  else
    putstr("---.--");
}

PUBLIC
void
Thread::print_state_long (unsigned cut_on_len = 0)
{
  static char * const state_names[] = 
    { 
      "ready", "wait", "receiving", "polling", 
      "ipc_in_progress", "send_in_progress", "busy", "",
      "cancel", "dead", "polling_long", "busy_long",
      "", "", "rcvlong_in_progress", "",
      "fpu_owner"
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

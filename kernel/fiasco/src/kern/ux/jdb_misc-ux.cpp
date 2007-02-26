IMPLEMENTATION[ux]:

#include <cstdio>
#include "config.h"
#include "cpu.h"
#include "jdb.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_tbuf.h"
#include "static_init.h"


class Jdb_misc_monitor : public Jdb_module
{
  static int  number;
  static char dummy;
  static char enable;
};

int  Jdb_misc_monitor::number;
char Jdb_misc_monitor::dummy;
char Jdb_misc_monitor::enable;

extern void jdb_trace_set_cpath(void) __attribute__((weak));

PUBLIC
Jdb_module::Action_code
Jdb_misc_monitor::action(int cmd, void *&args, char const *&fmt, int &)
{
  switch (cmd)
    {
    case 0:
#ifdef CONFIG_JDB_LOGGING
      // special log event
      if (args == &dummy)
	{
     	  if (Jdb::was_last_cmd() != 'O')
	    {
	      for (int i=0; i<LOG_EVENT_MAX_EVENTS; i++)
		if (Jdb_tbuf::log_events[i])
		  {
		    printf("\n    [%x] %s:\033[K",
			i, Jdb_tbuf::log_events[i]->get_name());
		    Jdb::cursor(Jdb_screen::height(), 29);
		    printf("%s",
			Jdb_tbuf::log_events[i]->enabled() ? " ON" : "off");
		  }
		else
		  printf("\n    [%x] <free>", i);
	      putchar('\n');
	    }

	  Jdb::cursor(Jdb_screen::height(), 1);
	  printf("  {0..%01x}{+|-}: ", LOG_EVENT_MAX_EVENTS-1);

	  args = &number;
	  fmt  = "%1x";
	  return EXTRA_INPUT;
	}
      else if (args == &number && Jdb_tbuf::log_events[number])
	{
	  printf(" %s:", Jdb_tbuf::log_events[number]->get_name());
	  args = &enable;
	  fmt  = "%C";
	  return EXTRA_INPUT;
	}
      else if (args == &enable)
	{
	  if (enable == '+' || enable == '-')
	    {
	      putchar(enable);
	      Jdb::cursor(Jdb_screen::height(), 1);
	      printf("  {0..%01x}{+|-}: ", LOG_EVENT_MAX_EVENTS-1);

	      if (Jdb_tbuf::log_events[number])
		{
		  Jdb_tbuf::log_events[number]->enable(enable=='+');
		  Jdb::cursor(Jdb_screen::height()-LOG_EVENT_MAX_EVENTS+number, 29);
		  printf("%s", Jdb_tbuf::log_events[number]->enabled() 
		      ? " ON" : "off");
		  if (jdb_trace_set_cpath != 0)
		    jdb_trace_set_cpath();
		}
	    }
	}
#else
      (void)args;
      (void)fmt;
      puts(" logging disabled");
#endif
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_misc_monitor::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "O", "monitor", "",
	  "O<number>{+|-}\ton/off special logging event",
	  &dummy),
    };
  return cs;
}

PUBLIC
int const
Jdb_misc_monitor::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_misc_monitor::Jdb_misc_monitor()
  : Jdb_module("MONITORING")
{
}

static Jdb_misc_monitor jdb_misc_monitor INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


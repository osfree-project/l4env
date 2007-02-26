IMPLEMENTATION[ia32]:

#include <cstdio>
#include "console_buffer.h"
#include "config.h"
#include "cpu.h"
#include "jdb.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_symbol.h"
#include "jdb_tbuf.h"
#include "static_init.h"


class Jdb_misc_general : public Jdb_module
{
  static char  first_char;
  static Mword output_lines;
};

char  Jdb_misc_general::first_char;
Mword Jdb_misc_general::output_lines;

PUBLIC
Jdb_module::Action_code
Jdb_misc_general::action(int cmd, void *&, char const *&, int &)
{
  switch (cmd)
    {
    case 0:
      // escape key
      if (first_char == '+' || first_char == '-')
	{
	  putchar(first_char);
	  Config::esc_hack = (first_char == '+');
	  putchar('\n');
	}
      break;
    case 1:
      // output buffer
      Jdb::console_buffer()->print_buffer(output_lines);
      break;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_misc_general::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "E", "esckey", "%C",
	  "E{+|-}\ton/off enter jdb by pressing <ESC>",
	  &first_char),
      Cmd (1, "B", "consolebuffer", "%4d\n",
	  "B[lines]\tshow console output buffer",
	  &output_lines),
    };
  return cs;
}

PUBLIC
int const
Jdb_misc_general::num_cmds() const
{
  return 2;
}

PUBLIC
Jdb_misc_general::Jdb_misc_general()
  : Jdb_module("GENERAL")
{
}

static Jdb_misc_general jdb_misc_general INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


//---------------------------------------------------------------------------//

class Jdb_misc_debug : public Jdb_module
{
  static char  first_char;
};

char  Jdb_misc_debug::first_char;

static void
Jdb_misc_debug::show_lbr_entry(const char *str, Address addr)
{
  char symbol[60];

  printf(str, addr);
  if (Jdb_symbol::match_eip_to_symbol(addr, 0, symbol, sizeof(symbol)))
    printf("(%s)", symbol);
}

PUBLIC
Jdb_module::Action_code
Jdb_misc_debug::action(int cmd, void *&, char const *&, int &)
{
  switch (cmd)
    {
    case 0:
      // single step
      if (first_char == '+' || first_char == '-')
	{
	  putchar(first_char);
	  Jdb::set_single_step(first_char == '+');
	  putchar('\n');
	}
      break;
    case 1:
      // lbr
      if (first_char == '+' || first_char == '-')
	{
	  Jdb::lbr_active = (first_char == '+');
	  putchar(first_char);
	  putchar('\n');
	}
      else
	{
	  Jdb::test_msr = 1;
	  if (Cpu::lbr_type() == Cpu::LBR_P4)
	    {
	      Unsigned64 msr;
	      Unsigned32 branch_tos;

	      msr = Cpu::rdmsr(0x1d7);
	      show_lbr_entry("\nbefore exc: %08x ", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1d8);
	      show_lbr_entry("\n         => %08x ", (Unsigned32)msr);

	      msr = Cpu::rdmsr(0x1da);
	      branch_tos = (Unsigned32)msr;

	      for (int i=0, j=branch_tos & 3; i<4; i++)
		{
		  j = (j+1) & 3;
		  msr = Cpu::rdmsr(0x1db+j);
		  show_lbr_entry("\nbranch/exc: %08x ",
		      (Unsigned32)(msr >> 32));
		  show_lbr_entry("\n         => %08x ",
		      (Unsigned32)msr);
		}
	    }
	  else if (Cpu::lbr_type() == Cpu::LBR_P6)
	    {
	      Unsigned64 msr;

	      msr = Cpu::rdmsr(0x1db);
	      show_lbr_entry("\nbranch: %08x ", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1dc);
	      show_lbr_entry("\n     => %08x ", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1dd);
	      show_lbr_entry("\n   int: %08x ", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1de);
	      show_lbr_entry("\n     => %08x ", (Unsigned32)msr);
	    }
	  else
	    {
	      printf("Last branch recording feature not available");
	    }

	  Jdb::test_msr = 0;
	  putchar('\n');
	  break;
	}
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_misc_debug::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "S", "singlestep", "%C",
	  "S{+|-}\ton/off permanent single step mode",
	  &first_char),
      Cmd (1, "L", "lbr", "",
	  "L\tshow last branch recording information",
	  &first_char),
    };
  return cs;
}

PUBLIC
int const
Jdb_misc_debug::num_cmds() const
{
  return 2;
}

PUBLIC
Jdb_misc_debug::Jdb_misc_debug()
  : Jdb_module("DEBUGGING")
{
}

static Jdb_misc_debug jdb_misc_debug INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


//---------------------------------------------------------------------------//

class Jdb_misc_info : public Jdb_module
{
  static char       first_char;
  static Address    addr;
  static Mword      value;
  static Unsigned64 value64;
};

char       Jdb_misc_info::first_char;
Address    Jdb_misc_info::addr;
Mword      Jdb_misc_info::value;
Unsigned64 Jdb_misc_info::value64;

PUBLIC
Jdb_module::Action_code
Jdb_misc_info::action(int cmd, void *&args, char const *&fmt, int &)
{
  switch (cmd)
    {
    case 0:
      // read/write physical memory
      if (args == &first_char)
	{
	  if (first_char == 'r' || first_char == 'w')
	    {
	      putchar(first_char);
	      fmt  = "%8x";
	      args = &addr;
	      return EXTRA_INPUT;
	    }
	}
      else if (args == &addr || args == &value)
	{
	  addr &= ~3;
	  if (args == &value)
	    {
	      for (int i=3; i>=0; i--)
		Jdb::poke_phys(addr + i, (value >> 8*i) & 0xff);
	    }
	  if (first_char == 'w' && (args == &addr))
	    putstr(" (");
	  else
	    putstr(" => ");
	  for (int i=3; i>=0; i--)
    	    printf("%02x", Jdb::peek_phys(addr + i));
	  if (first_char == 'w' && (args == &addr))
	    {
	      putstr(") new value=");
	      fmt  = "%08x";
	      args = &value;
	      return EXTRA_INPUT;
	    }
	  putchar('\n');
	}
      break;

    case 1:
      // read/write machine status register
      if (!(Cpu::features() & FEAT_MSR))
	{
	  puts("MSR not supported");
	  return NOTHING;
	}

      if (args == &first_char)
	{
	  if (first_char == 'r' || first_char == 'w')
	    {
	      putchar(first_char);
	      fmt  = "%8x";
	      args = &addr;
	      return EXTRA_INPUT;
	    }
	}
      else if (args == &addr || args == &value64)
	{
	  if (args == &value64)
	    {
	      Cpu::wrmsr(value64, addr);
	    }
	  if (first_char == 'w' && (args == &addr))
	    putstr(" (");
	  else
	    putstr(" => ");
	  value64 = Cpu::rdmsr(addr);
	  printf("%016llx", value64);
	  if (first_char == 'w' && (args == &addr))
	    {
	      putstr(") new value=");
	      fmt  = "%16llx";
	      args = &value64;
	      return EXTRA_INPUT;
	    }
	  putchar('\n');
	}
      break;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_misc_info::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "A", "adapter", "%C",
	  "A{r|w}<addr>\tread/write any physical address",
	  &first_char),
      Cmd (1, "M", "msr", "%C",
	  "M{r|w}<addr>\tread/write machine status register",
	  &first_char),
    };
  return cs;
}

PUBLIC
int const
Jdb_misc_info::num_cmds() const
{
  return 2;
}

PUBLIC
Jdb_misc_info::Jdb_misc_info()
  : Jdb_module("INFO")
{
}

static Jdb_misc_info jdb_misc_info INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


//---------------------------------------------------------------------------//

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


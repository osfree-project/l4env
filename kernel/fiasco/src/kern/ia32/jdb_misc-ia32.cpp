IMPLEMENTATION[ia32]:

#include <cstdio>
#include "config.h"
#include "cpu.h"
#include "jdb.h"
#include "jdb_ktrace.h"
#include "jdb_module.h"
#include "jdb_symbol.h"
#include "jdb_screen.h"
#include "jdb_tbuf_events.h"
#include "static_init.h"
#include "x86desc.h"

class Jdb_misc_general : public Jdb_module
{
  static char first_char;
};

char Jdb_misc_general::first_char;


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
	  return NOTHING;
	}
      return ERROR;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_misc_general::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "E", "esckey", "%C",
	  "E{+|-}\ton/off enter jdb by pressing <ESC>",
	  &first_char },
    };
  return cs;
}

PUBLIC
int const
Jdb_misc_general::num_cmds() const
{
  return 1;
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
  static char     first_char;
  static Task_num task;
};

char     Jdb_misc_debug::first_char;
Task_num Jdb_misc_debug::task;

static void
Jdb_misc_debug::show_lbr_entry(const char *str, Address addr)
{
  char symbol[60];

  printf("%s "L4_PTR_FMT" ", str, addr);
  if (Jdb_symbol::match_addr_to_symbol_fuzzy(&addr, 0, symbol, sizeof(symbol)))
    printf("(%s)", symbol);
}


// ----------------------------------------------------------------------------
IMPLEMENTATION[ia32-segments]:

static void
Jdb_misc_debug::show_ldt()
{
  Space *s = Space_index(task).lookup();
  Address addr, size;

  if (!s)
    {
      printf(" -- invalid task number");
      return;
    }

  addr = s->ldt_addr();
  size = s->ldt_size();

  if (!size)
    {
      printf(" -- no LDT active");
      return;
    }

  printf("\nLDT of space %x at %08lx-%08lx\n", task, addr, addr+size-1);

  X86desc *desc = (X86desc*)addr;

  for (; size>=Cpu::Ldt_entry_size; size-=Cpu::Ldt_entry_size, desc++)
    {
      X86desc d = s->peek_user(desc);
      if (d.present())
	{
	  printf(" %5d: ", desc-(X86desc*)addr);
	  d.show();
	}
    }
}


// ----------------------------------------------------------------------------
IMPLEMENTATION[ia32-!segments]:
static void
Jdb_misc_debug::show_ldt()
{
  printf(" -- support for segments not enabled in config");
}


// ----------------------------------------------------------------------------
IMPLEMENTATION[ia32]:

PUBLIC
Jdb_module::Action_code
Jdb_misc_debug::action(int cmd, void *&args, char const *&fmt, int &)
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
      // ldt
      if (args == &task)
	{
	  show_ldt();
	  putchar('\n');
	  return NOTHING;
	}

      // lbr/ldt
      if (first_char == '+' || first_char == '-')
	{
	  Jdb::lbr_active = (first_char == '+');
	  putchar(first_char);
	  putchar('\n');
	}
      else if (first_char == 'd')
	{
	  putchar('d');
	  fmt   = "%3x";
	  args  = &task;
	  return EXTRA_INPUT;
	}
      else
	{
	  Jdb::test_msr = 1;
	  if (Cpu::lbr_type() == Cpu::LBR_P4)
	    {
	      Unsigned64 msr;
	      Unsigned32 branch_tos;

	      msr = Cpu::rdmsr(0x1d7);
	      show_lbr_entry("\nbefore exc:", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1d8);
	      show_lbr_entry("\n         =>", (Unsigned32)msr);

	      msr = Cpu::rdmsr(0x1da);
	      branch_tos = (Unsigned32)msr;

	      for (int i=0, j=branch_tos & 3; i<4; i++)
		{
		  j = (j+1) & 3;
		  msr = Cpu::rdmsr(0x1db+j);
		  show_lbr_entry("\nbranch/exc:",
		      (Unsigned32)(msr >> 32));
		  show_lbr_entry("\n         =>",
		      (Unsigned32)msr);
		}
	    }
	  else if (Cpu::lbr_type() == Cpu::LBR_P6)
	    {
	      Unsigned64 msr;

	      msr = Cpu::rdmsr(0x1db);
	      show_lbr_entry("\nbranch:", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1dc);
	      show_lbr_entry("\n     =>", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1dd);
	      show_lbr_entry("\n   int:", (Unsigned32)msr);
	      msr = Cpu::rdmsr(0x1de);
	      show_lbr_entry("\n     =>", (Unsigned32)msr);
	    }
	  else
	    printf("Last branch recording feature not available");

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
	{ 0, "S", "singlestep", "%C",
	  "S{+|-}\ton/off permanent single step mode",
	  &first_char },
	{ 1, "L", "lbr", "%C",
	  "L\tshow last branch recording information\n"
	  "Ld<taskno>\tshow LDT of specific task",
	  &first_char },
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
	  Jdb::test_msr = 1;
	  if (args == &value64)
	    Cpu::wrmsr(value64, addr);
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
	  Jdb::test_msr = 0;
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
	{ 0, "A", "adapter", "%C",
	  "A{r|w}<addr>\tread/write any physical address",
	  &first_char },
        { 1, "M", "msr", "%C",
	  "M{r|w}<addr>\tread/write machine status register",
	  &first_char },
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

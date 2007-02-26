INTERFACE:

#include "types.h"
#include "l4_types.h"


IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include <cstdarg>

#include "alloca.h"
#include "disasm.h"
#include "jdb.h"
#include "jdb_input.h"
#include "jdb_lines.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_symbol.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "static_init.h"

class Jdb_disasm : public Jdb_module
{
  static char show_intel_syntax;
  static char show_lines;
};


char Jdb_disasm::show_intel_syntax;
char Jdb_disasm::show_lines = 2;


// available from the jdb_bp module
extern int jdb_instruction_bp_at_addr(Address addr)
  __attribute__((weak));


static bool
Jdb_disasm::disasm_line(char *buffer, int buflen, Address &addr,
			int show_symbols, Task_num task)
{
  int len;

  if (task>=L4_uid::MAX_TASKS)
    {
      printf("Invalid task %x\n", task);
      addr += 1;
      return false;
    }

  if ((len = disasm_bytes(buffer, buflen, addr, task, show_symbols, 
			  show_intel_syntax, Jdb::peek_task,
			  Jdb_symbol::match_addr_to_symbol)) < 0)
    {
      addr += 1;
      return false;
    }

  addr += len;
  return true;
}

static Address
Jdb_disasm::disasm_offset(Address &start, int offset, Task_num task)
{
  if (offset>0)
    {
      Address addr = start;
      while (offset--)
	{
	  if (!disasm_line(0, 0, addr, 0, task))
	    {
	      start = addr + offset;
	      return false;
	    }
	}
      start = addr;
      return true;
    }
  else
    {
      while (offset++)
	{
	  Address addr=start-64, va_start;
	  for (;;)
	    {
	      va_start = addr;
	      if (!disasm_line(0, 0, addr, 0, task))
		{
		  start += (offset-1);
		  return false;
		}
	      if (addr >= start)
		break;
	    }
	  start = va_start;
	}

      return true;
    }
}

PUBLIC
static bool
Jdb_disasm::show_disasm_line(int len, Address &addr, 
			     int show_symbols, Task_num task)
{
  int clreol = 0;
  if (len < 0)
    {
      len = -len;
      clreol = 1;
    }
  
  char *line = (char*)alloca(len);
  if (line && disasm_line(line, len, addr, show_symbols, task))
    {
      if (clreol)
	printf("%s\033[K\n", line);
      else
	printf("%-*s\n", len, line);
      return true;
    }
  
  if (clreol)
    puts("........\033[K");
  else
    printf("........%*s", len-8, "\n");
  return false;
}

PUBLIC static
Jdb_module::Action_code
Jdb_disasm::show(Address virt, Task_num task, int level)
{
  Address enter_addr = virt;

  if (level==0)
    Jdb::fancy_clear_screen();

  for (;;)
    {
      Jdb::cursor();

      Address addr;
      Mword   i;
      for (i=Jdb_screen::height()-1, addr=virt; i>0; i--)
	{
	  const char *symbol;
      	  char str[78], *nl;
	  char stat_str[6];

	  *(unsigned int*  ) stat_str    = 0x20202020;
	  *(unsigned short*)(stat_str+4) = 0x0020;

	  if ((symbol = Jdb_symbol::match_addr_to_symbol(addr, task)))
	    {
	      str[0] = '<';
    	      strncpy(str+1, symbol, sizeof(str)-3);
	      str[sizeof(str)-3] = '\0';
	      
	      // cut symbol at newline
	      for (nl=str; (*nl!='\0') && (*nl!='\n'); nl++)
		;
	      *nl++ = '>';
	      *nl++ = ':';
    	      *nl++ = '\0';
	      
	      printf("%s%s\033[m\033[K\n", Jdb::esc_symbol, str);
	      if (!--i)
		break;
	    }
	 
	  if (show_lines)
	    {
	      if (Jdb_lines::match_addr_to_line(addr, task, str, 
						sizeof(str)-1, show_lines==2))
		{
		  printf("%s%s\033[m\033[K\n", Jdb::esc_line, str);
		  if (!--i)
		    break;
		}
	    }
	
	  // show instruction breakpoint
	  if (jdb_instruction_bp_at_addr != 0)
	    {
	      if (Mword i = jdb_instruction_bp_at_addr(addr))
		{
		  stat_str[0] = '#';
		  stat_str[1] = '0'+i;
		}
	    }

	  if (addr == enter_addr)
	    printf("%s%08x\033[m %s", Jdb::esc_emph, addr, stat_str);
	  else
	    printf("%08x %s", addr, stat_str);
	  show_disasm_line(-64, addr, 1, task);
	}

      static char const * const line_mode[] = { "", "[Source]", "[Headers]" };
      static char const * const syntax_mode[] = { "[AT&T]", "[Intel]" };
      Jdb::printf_statline("u<%08x> task %-3x  %-9s  %-7s%34s",
		      virt, task, line_mode[(int)show_lines], 
		      syntax_mode[(int)show_intel_syntax],
		      "<Space>=lines mode");

      Jdb::cursor(Jdb_screen::height(), Jdb::LOGO+1);
      switch (int c = getchar())
	{
	case KEY_CURSOR_LEFT:
	  virt -= 1;
	  break;
	case KEY_CURSOR_RIGHT:
	  virt += 1;
	  break;
	case KEY_CURSOR_DOWN:
	  disasm_offset(virt, +1, task);
	  break;
	case KEY_CURSOR_UP:
	  disasm_offset(virt, -1, task);
	  break;
	case KEY_PAGE_UP:
	  disasm_offset(virt, -Jdb_screen::height()+2, task);
	  break;
	case KEY_PAGE_DOWN:
	  disasm_offset(virt, +Jdb_screen::height()-2, task);
	  break;
	case ' ':
	  show_lines = (show_lines+1) % 3;
	  break;
	case KEY_TAB:
	  show_intel_syntax ^= 1;
	  break;
	case KEY_CURSOR_HOME:
	  if (level > 0)
	    return GO_BACK;
	  break;
	case KEY_ESC:
	  Jdb::abort_command();
	  return NOTHING;
	default:
	  if (Jdb::is_toplevel_cmd(c)) 
	    return NOTHING;
	  break;
	}
    }
  
  return GO_BACK;
}

PUBLIC
Jdb_module::Action_code
Jdb_disasm::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  if (cmd == 0)
    {
      Jdb_module::Action_code code;

      code = Jdb_input_task_addr::action(args, fmt, next_char);
      if (code == Jdb_module::NOTHING 
	  && Jdb_input_task_addr::task != (Task_num)-1)
	{
	  Address addr  = Jdb_input_task_addr::addr;
	  Task_num task = Jdb_input_task_addr::task;
	  if (addr == (Address)-1)
	    addr = Jdb::get_entry_frame()->eip;
	  return show(addr, Jdb::translate_task(addr, task), 0) 
		    ? GO_BACK : NOTHING;
	}

      if (code != LEAVE)
	return code;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_disasm::cmds() const
{
  static Cmd cs[] =
    {
      Cmd (0, "u", "u", "%C",
	"u[t<taskno>]<addr>\tdisassemble bytes of given/current task addr",
	&Jdb_input_task_addr::first_char)
    };

  return cs;
}

PUBLIC
int const
Jdb_disasm::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_disasm::Jdb_disasm()
  : Jdb_module("INFO")
{}

static Jdb_disasm jdb_disasm INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

// function directly called by external modules
int
jdb_disasm_addr_task(Address addr, Task_num task, int level)
{ return Jdb_disasm::show(addr, task, level); }

int
jdb_disasm_one_line(int len, Address &addr, int show_symbols, Task_num task)
{ return Jdb_disasm::show_disasm_line(len, addr, show_symbols, task); }


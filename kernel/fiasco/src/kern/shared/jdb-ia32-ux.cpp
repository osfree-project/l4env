INTERFACE:

class Space;
class Thread;


EXTENSION class Jdb
{
public:
  static void    print_current_tid_statline (void);
  static void    printf_statline (const char *format, ...)
    __attribute__((format(printf, 1, 2)));

  static int     std_cursor_key(int c, Mword lines, Mword max_absy, 
				Mword *absy, Mword *addy, Mword *addx, 
				bool *redraw);

  static int     getchar(void);
  static int     was_last_cmd(void);
  static int     get_next_cmd(void);
  static void    set_next_cmd(char c);
  static void    flush_next_cmd(void);
  static bool    is_toplevel_cmd(char c);
  static int     execute_command(const char *s, int first_char = -1);
  static int     pt_entry_valid(Mword entry);
  static int     pd_entry_is_ptr_to_pt(Mword entry);
  static Address pt_entry_phys(Mword entry);
  static void    dump_pt_entry(Mword entry);
  static int     write_ll_ns(Signed64 ns, char *buf, int maxlen, bool sign);
  static int     write_ll_hex(Signed64 x, char *buf, int maxlen, bool sign);
  static int     write_ll_dec(Signed64 x, char *buf, int maxlen, bool sign);
  static int     write_tsc_s(Signed64 tsc, char *buf, int maxlen, bool sign);
  static int     write_tsc(Signed64 tsc, char *buf, int maxlen, bool sign);

  static Jdb_at  at_enter;
  static Jdb_at  at_leave;

private:
  static Thread  *current_active;
  static char    last_cmd;
  static char    next_cmd;
};


IMPLEMENTATION[ia32-ux]:

#include <flux/x86/base_paging.h>

Jdb_at  Jdb::at_enter;
Jdb_at  Jdb::at_leave;
Thread *Jdb::current_active;		// current running thread
char    Jdb::last_cmd;
char    Jdb::next_cmd;


IMPLEMENT inline NEEDS[<flux/x86/base_paging.h>]
int
Jdb::pt_entry_valid(Mword entry)
{
  return entry & INTEL_PDE_VALID;
}

IMPLEMENT inline NEEDS[<flux/x86/base_paging.h>]
int
Jdb::pd_entry_is_ptr_to_pt(Mword entry)
{
  return !(entry & INTEL_PDE_SUPERPAGE);
}

IMPLEMENT inline
Address
Jdb::pt_entry_phys(Mword entry)
{
  return pde_to_pa(entry);
}

IMPLEMENT
void
Jdb::dump_pt_entry(Mword entry)
{
  if (!(entry & INTEL_PDE_VALID))
    printf("    -   ");
  else
    {
      Address phys = pde_to_pa(entry);

      if (phys > 0x40000000)
	{
	  if (entry & INTEL_PDE_SUPERPAGE)
	    printf("%03X/4--", phys >> (Config::SUPERPAGE_SHIFT-2));
	  else
	    printf("%05X--", phys >> Config::PAGE_SHIFT);
	}
      else
	{
	  phys &= 0x0fffffff; // XXX
	  if (entry & INTEL_PDE_SUPERPAGE)
	    printf(" %02x/4--", phys >> Config::SUPERPAGE_SHIFT);
	  else
	    printf(" %04x--", phys >> Config::PAGE_SHIFT);
	}
      putchar(entry & INTEL_PDE_USER
		? (entry & INTEL_PDE_WRITE) ? 'w' : 'r'
		: (entry & INTEL_PDE_WRITE) ? 'W' : 'R');
    }
}

// handling of standard cursor keys (Up/Down/PgUp/PgDn)
IMPLEMENT
int
Jdb::std_cursor_key(int c, Mword lines, Mword max_absy, Mword *absy,
		    Mword *addy, Mword *addx, bool *redraw)
{
  switch (c)
    {
    case KEY_CURSOR_LEFT:
      if (addx)
	{
	  if (*addx > 0)
	    (*addx)--;
	  else if (*addy > 0)
	    {
	      (*addy)--;
	      *addx = 7;
	    }
	  else if (*absy > 0)
	    {
	      (*absy)--;
	      *addx = 7;
	      *redraw = true;
	    }
	}
      else
	return 0;
      break;
    case KEY_CURSOR_RIGHT:
      if (addx)
	{   
	  if (*addx < 7)
	    (*addx)++;
	  else if (*addy < lines-1)
	    {
	      (*addy)++; 
	      *addx = 0;
	    }
	  else if (*absy < max_absy)
	    {
	      (*absy)++;
	      *addx = 0;
	      *redraw = true;
	    }
	}
      else
	return 0;
      break;
    case KEY_CURSOR_UP:
      if (*addy > 0)
	(*addy)--;
      else if (*absy > 0)
	{
	  (*absy)--;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_DOWN:
      if (*addy < lines-1)
	(*addy)++;
      else if (*absy < max_absy)
	{
	  (*absy)++;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_HOME:
      *addy = 0;
      if (addx)
	*addx = 0;
      if (*absy > 0)
	{
	  *absy = 0;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_END:
      *addy = lines-1;
      if (addx)
	*addx = 7;
      if (*absy < max_absy)
	{
	  *absy = max_absy;
	  *redraw = true;
	}
      break;
    case KEY_PAGE_UP:
      if (*absy >= lines)
	{
	  *absy -= lines;
	  *redraw = true;
	}
      else
	{
	  if (*absy > 0)
	    {
	      *absy = 0;
	      *redraw = true;
	    }
	  else if (*addy > 0)
	    *addy = 0;
	  else if (addx)
	    *addx = 0;
	}
      break;
    case KEY_PAGE_DOWN:
      if (*absy+lines-1 < max_absy)
	{
	  *absy += lines;
	  *redraw = true;
	}
      else
	{
	  if (*absy < max_absy)
	    {
	      *absy = max_absy;
	      *redraw = true;
	    }
	  else if (*addy < lines-1)
      	    *addy = lines-1;
	  else if (addx)
	    *addx = 7;
	}
      break;
    default:
      return 0;
    }

  return 1;
}

IMPLEMENT inline
int
Jdb::was_last_cmd()
{
  return last_cmd;
}

IMPLEMENT inline
int
Jdb::get_next_cmd()
{
  return next_cmd;
}

IMPLEMENT inline
void
Jdb::set_next_cmd(char c)
{
  next_cmd = c;
}

IMPLEMENT
int
Jdb::getchar(void)
{
  return Kconsole::console()->getchar();
}

IMPLEMENT
bool
Jdb::is_toplevel_cmd(char c)
{
  char cm[] = { c, 0 };
  Jdb_core::Cmd cmd = Jdb_core::has_cmd (cm);

  if(cmd.cmd || (0 != strchr(toplevel_cmds, c)))
    {
      set_next_cmd(c);
      return true;
    }

  return false;
}


IMPLEMENT
int
Jdb::execute_command(const char *s, int first_char = -1)
{
  Jdb_core::Cmd cmd = Jdb_core::has_cmd(s);

  if (cmd.cmd)
    return Jdb_core::exec_cmd(cmd, first_char) == 2 ? 1 : 0;

  return 0;
}

// go to bottom of screen and print the current thread id
IMPLEMENT
void
Jdb::print_current_tid_statline()
{
  cursor(Jdb_screen::height(), 1);
  putstr(esc_prompt);

  if (current_active)
    printf("(%x.%02x) ",
	current_active->debug_taskno(), 
	current_active->debug_threadno());

  putstr("jdb: \033[m");
  clear_to_eol();
}

// go to bottom of screen and print some text in the form "jdb: ..."
// if no text follows after the prompt, prefix the current thread number
IMPLEMENT
void
Jdb::printf_statline(const char *format, ...)
{
  cursor(Jdb_screen::height(), 1);
  putstr(esc_prompt);

  putstr("jdb: \033[m");

  va_list list;
  va_start(list, format);
  vprintf(format, list);
  va_end(list);

  clear_to_eol();
}

IMPLEMENT
int
Jdb::write_ll_ns(Signed64 ns, char *buf, int maxlen, bool sign)
{
  Unsigned64 uns = (ns < 0) ? -ns : ns;
  int len = 0;

  if (uns >= 1000000000000ULL)
    {
      strncpy(buf, ">99     s ", maxlen);
      len = 10;
    }
  else
    {
      if (sign)
	{
	  *buf++ = (ns < 0) ? '-' : (ns == 0) ? ' ' : '+';
	  maxlen--;
	  len = 1;
	}
      for (; maxlen>11; maxlen--, len++)
	*buf++ = ' ';
      if (uns >= 1000000000)
	{
	  Mword _s  = uns / 1000000000ULL;
	  Mword _ms = (uns - 1000000000ULL * (Unsigned64)_s) / 1000000ULL;
	  len += snprintf(buf, maxlen, "%3u.%03u s ", _s, _ms);
	}
      else if (uns >= 1000000)
	{
	  Mword _ms = uns / 1000000UL;
	  Mword _us = (uns - 1000000UL * (Unsigned64)_ms) / 1000UL;
	  len += snprintf(buf, maxlen, "%3u.%03u ms", _ms, _us);
	}
      else if (uns == 0)
	{
	  strncpy(buf, "  0       ", maxlen);
	  len += 10;
	}
      else
	{
	  Mword _us = uns / 1000UL;
	  Mword _ns = (uns - 1000UL * (Unsigned64)_us);
	  len += snprintf(buf, maxlen, "%3u.%03u %c ", _us, _ns, 
#ifdef CONFIG_UX
	      1 ||
#endif
	      Kconsole::console()->gzip_enabled() ? 'µ' : '\346');
	}
    }

  return len;
}

IMPLEMENT
int
Jdb::write_ll_hex(Signed64 x, char *buf, int maxlen, bool sign)
{
  // display 40 bits
  Unsigned64 xu = (x < 0) ? -x : x;

  return snprintf(buf, maxlen, "%s%02x%08x",
		  sign ? (x < 0) ? "-" : (x == 0) ? " " : "+" : "",
		  (Mword)((xu >> 32) & 0xff), (Mword)xu);
}

IMPLEMENT
int
Jdb::write_ll_dec(Signed64 x, char *buf, int maxlen, bool sign)
{
  Unsigned64 xu = (x < 0) ? -x : x;

  // display decimal
  if (xu > 0xFFFFFFFFULL)
    {
      strncpy(buf, " >>       ", maxlen);
      return 10;
    }

  return snprintf(buf, maxlen, "%s%10u",
		  sign ? x < 0 ? "-" 
			       : (x == 0) ? " " 
					  : "+"  
		       : "", (Mword)xu);
}

IMPLEMENT
int
Jdb::write_tsc_s(Signed64 tsc, char *buf, int maxlen, bool sign)
{
  Unsigned64 uns = Cpu::tsc_to_ns(tsc < 0 ? -tsc : tsc);
  int len = 0;

  if (tsc < 0)
    uns = -uns;

  if (uns >= 1000000000000ULL)
    {
      strncpy(buf, ">99        s ", maxlen);
      len = 13;
    }
  else
    {
      if (sign)
	{
	  *buf++ = (tsc < 0) ? '-' : (tsc == 0) ? ' ' : '+';
	  maxlen--;
	  len = 1;
	}
      for (; maxlen>14; maxlen--, len++)
	*buf++ = ' ';
      if (uns >= 1000000000)
	{
	  Mword _s  = uns / 1000000000ULL;
	  Mword _us = (uns - 1000000000ULL * (Unsigned64)_s) / 1000ULL;
	  len += snprintf(buf, maxlen, "%3u.%06u s ", _s, _us);
	}
      else if (uns == 0)
	{
	  strncpy(buf, "  0          ", maxlen);
	  len += 13;
	}
      else
	{
	  Mword _ms = uns / 1000000UL;
	  Mword _ns = (uns - 1000000UL * (Unsigned64)_ms);
	  len += snprintf(buf, maxlen, "%3u.%06u ms", _ms, _ns);
	}
    }

  return len;
}

IMPLEMENT
int
Jdb::write_tsc(Signed64 tsc, char *buf, int maxlen, bool sign)
{
  Unsigned64 ns = Cpu::tsc_to_ns(tsc < 0 ? -tsc : tsc);
  if (tsc < 0)
    ns = -ns;
  return write_ll_ns(ns, buf, maxlen, sign);
}

static void
Jdb::get_current()
{
  current_active = get_thread();
  
  if (current_active->is_mapped())
    {
      const Mword mask 
	= (Config::thread_block_size * Kmem::info()->max_threads()) - 1;

      LThread_num lthread = ((Address)current_active & mask) 
			  / Config::thread_block_size;
      Task_num    task    = lthread / L4_uid::threads_per_task();

      lthread    -= task * L4_uid::threads_per_task();

      // sanity check for broken tcbs
      if (   current_active->id().task()    == task
	  && current_active->id().lthread() == lthread)
	{
	  return;
	}
    }

  current_active = 0;
}

PUBLIC static
Task_num
Jdb::get_current_task()
{
  return current_active ? current_active->id().task() : 0;
}

PUBLIC static
Space*
Jdb::get_current_space()
{
  return current_active ? current_active->space() : 0;
}

PUBLIC static inline
Thread*
Jdb::get_current_active()
{
  return current_active;
}

PUBLIC static int
Jdb::set_prompt_color(char x)
{
  unsigned pc = 32;
  unsigned ph = 0;
 
  switch(x) 
    {
    case 'N': ph = 1;
    case 'n': pc = 30; break;
    case 'R': ph = 1;
    case 'r': pc = 31; break;
    case 'G': ph = 1;
    case 'g': pc = 32; break;
    case 'Y': ph = 1;
    case 'y': pc = 33; break;
    case 'B': ph = 1;
    case 'b': pc = 34; break;
    case 'M': ph = 1; 
    case 'm': pc = 35; break;
    case 'C': ph = 1;
    case 'c': pc = 36; break;
    case 'W': ph = 1;
    case 'w': pc = 37; break;
    default : return 0;
    }

  if(ph>0)
    snprintf(esc_prompt,sizeof(esc_prompt)-1,"\033[%d;%dm",pc,ph);
  else
    snprintf(esc_prompt,sizeof(esc_prompt)-1,"\033[%dm",pc);

  return 1;
}


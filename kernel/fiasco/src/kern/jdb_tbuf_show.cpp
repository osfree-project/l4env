IMPLEMENTATION:

#include <cstdio>
#include <cstring>

#include "alloca.h"
#include "cpu.h"
#include "jdb.h"
#include "jdb_input.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_symbol.h"
#include "jdb_tbuf.h"
#include "jdb_tbuf_output.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "perf_cnt.h"
#include "static_init.h"
#include "thread.h"
#include "threadid.h"

class Jdb_tbuf_show : public Jdb_module
{
  static Mword show_tb_nr;
  static Mword show_tb_refy;
  static Mword show_tb_absy;

  enum
    {
      INDEX_MODE      = 0,
      DELTA_TSC_MODE  = 1,
      REF_TSC_MODE    = 2,
      START_TSC_MODE  = 3,
      DELTA_PMC1_MODE = 4,
      DELTA_PMC2_MODE = 5,
      REF_PMC1_MODE   = 6,
      REF_PMC2_MODE   = 7,
    };

  enum
    {
      SHOW_TRACEBUFFER_START = 3,
    };
};

Mword Jdb_tbuf_show::show_tb_nr;
Mword Jdb_tbuf_show::show_tb_refy;
Mword Jdb_tbuf_show::show_tb_absy;

// available from the jdb_disasm module
int jdb_disasm_addr_task (Address addr, Task_num task, int level)
  __attribute__((weak));

static int
Jdb_tbuf_show::get_string(char *string, int size)
{
  for (int pos=strlen(string); ; )
    {
      string[pos] = 0;
      switch(int c=getchar())
	{
	case KEY_BACKSPACE:
	  if (pos)
	    {
	      putstr("\b \b");
	      pos--;
	    }
	  break;
	case KEY_RETURN:
	  return 1;
	case KEY_ESC:
	  Jdb::abort_command();
	  return 0;
	default:
	  if (c > ' ')
    	    {
	      if (pos < size-1)
		{
		  putchar(c);
		  string[pos++] = c;
		}
	    }
	}
    }
}

static void
Jdb_tbuf_show::show_perf_event_unit_mask_entry(Mword nr, Mword idx,
					       Mword unit_mask, int exclusive)
{
  char *line = (char*)alloca(80);
  const char *desc;
  Mword value, sz_desc;

  Perf_cnt::get_unit_mask_entry(nr, idx, &value, &desc);
  if (!desc || !*desc)
    desc = "(no description?)";
  if ((sz_desc = strlen(desc)) > 59)
    sz_desc = 59;
  sprintf(line, "  %c %02x ",
      exclusive ? unit_mask == value ? '+' : ' '
                : unit_mask  & value ? '+' : ' ', 
      value);
  memcpy(line+7, desc, sz_desc);
  line[7+sz_desc] = '\0';
  printf("%s\033[K", line);
}

static void
Jdb_tbuf_show::show_perf_event(Mword nr)
{
  char *line = (char*)alloca(84);
  const char *name, *desc;
  Mword evntsel, sz_name, sz_desc;

  Perf_cnt::get_perf_event(nr, &evntsel, &name, &desc);
  if (!name || !*name)
    name = "(no name?)";
  if (!desc || !*desc)
    desc = "(no description)";
  if ((sz_name = strlen(name)) > 26)
    sz_name = 26;
  if ((sz_desc = strlen(desc)) > 49)
    sz_desc = 49;
  sprintf(line, "%02x ", evntsel);
  memcpy(line+3, name, sz_name);
  memset(line+3+sz_name, ' ', 27-sz_name);
  memcpy(line+30, desc, sz_desc);
  line[30+sz_desc] = '\0';
  printf("%s\033[K", line);
}

static Mword
Jdb_tbuf_show::select_perf_event_unit_mask(Mword nr, Mword unit_mask)
{
  Mword absy     = 0;
  Mword addy     = 0;
  Mword max_absy = 0;
  Mword lines    = 10;

  Mword default_value, nvalues, value;
  Perf_cnt::Unit_mask_type type;
  const char *desc;

  Perf_cnt::get_unit_mask(nr, &type, &default_value, &nvalues);
  int  exclusive = (type == Perf_cnt::Exclusive);

  if (type == Perf_cnt::None)
    return 0;

  if (type == Perf_cnt::Fixed || (nvalues < 1))
    return default_value;

  if (nvalues == 1)
    {
      Perf_cnt::get_unit_mask_entry(nr, 0, &value, &desc);
      return value;
    }

  if (nvalues < lines)
    lines = nvalues;

  Jdb::printf_statline("P?%72s", "<Space>=set mask <CR>=done");

  Jdb::cursor(SHOW_TRACEBUFFER_START, 1);
  putstr("\033[32m");
  show_perf_event(nr);
  printf("\033[m\033[K\n"
	 "\033[K\n"
	 "  \033[1;32mSelect Event Mask (%s):\033[m\033[K\n"
	 "\033[K\n", exclusive ? "exclusive" : "bitmap");

  for (;;)
    {
      Mword i;

      Jdb::cursor(SHOW_TRACEBUFFER_START+4, 1);
      for (i=0; i<lines; i++)
	{
	  show_perf_event_unit_mask_entry(nr, i, unit_mask, exclusive);
	  putchar('\n');
	}
      for (; i<Jdb_screen::height()-SHOW_TRACEBUFFER_START-5; i++)
	puts("\033[K");

      for (bool redraw=false; !redraw; )
	{
	  int c;
	  const char *dummy;
	  Mword value;

	  Jdb::cursor(addy+SHOW_TRACEBUFFER_START+4, 1);
	  putstr("\033[1;33m");
	  show_perf_event_unit_mask_entry(nr, absy+addy, unit_mask, exclusive);
	  putstr("\033[m");
	  Jdb::cursor(addy+SHOW_TRACEBUFFER_START+4, 1);
	  c = getchar();
	  show_perf_event_unit_mask_entry(nr, absy+addy, unit_mask, exclusive);
	  Perf_cnt::get_unit_mask_entry(nr, absy+addy, &value, &dummy);
	  if (!Jdb::std_cursor_key(c, lines, max_absy,
				   &absy, &addy, 0, &redraw))
	    {
	      switch (c)
		{
		case ' ':
		  if (exclusive)
		    {
		      unit_mask = value;
		      redraw = true;
		    }
		  else
		    unit_mask ^= value;
		  break;
		case KEY_RETURN:
	      	  return unit_mask;
		case KEY_ESC:
		  return (Mword)-1;
		default:
		  if (Jdb::is_toplevel_cmd(c)) 
		    return (Mword)-1;
		}
	    }
	}
    }
}

static Mword
Jdb_tbuf_show::select_perf_event(Mword event)
{
  Mword absy     = 0;
  Mword addy     = 0;
  Mword nevents  = Perf_cnt::get_max_perf_event();
  Mword lines    = (nevents < 19) ? nevents : 19;
  Mword max_absy = nevents-lines;

  if (nevents == 0)
    // libperfctr not linked
    return (Mword)-1;

  Mword evntsel, unit_mask;

  Jdb::printf_statline("P?%72s", "<CR>=select");

  Jdb::cursor(SHOW_TRACEBUFFER_START, 1);
  puts("\033[1;32mSelect performance counter\033[m\033[K\n"
       "\033[K");

  Perf_cnt::split_event(event, &evntsel, &unit_mask);
  addy = Perf_cnt::lookup_event(evntsel);
  if (addy == (Mword)-1)
    addy = 0;
  else if (addy > lines-1)
    {
      absy += (addy-lines+1);
      addy = lines-1;
    }

  for (;;)
    {
      Mword i;

      Jdb::cursor(SHOW_TRACEBUFFER_START+2, 1);
      for (i=0; i<lines; i++)
	{
	  show_perf_event(absy+i);
	  putchar('\n');
	}
      for (; i<Jdb_screen::height()-SHOW_TRACEBUFFER_START-6; i++)
	puts("\033[K");

      for (bool redraw=false; !redraw; )
	{
	  const char *dummy;

	  Jdb::cursor(addy+SHOW_TRACEBUFFER_START+2, 1);
	  putstr("\033[1;33m");
	  show_perf_event(absy+addy);
	  putstr("\033[m");
	  Jdb::cursor(addy+SHOW_TRACEBUFFER_START+2, 1);
	  int c = getchar();
	  show_perf_event(absy+addy);
	  if (!Jdb::std_cursor_key(c, lines, max_absy, 
				   &absy, &addy, 0, &redraw))
	    {
	      switch (c)
		{
		case KEY_RETURN:
		  Perf_cnt::get_perf_event(absy+addy, &evntsel, &dummy, &dummy);
		  unit_mask = select_perf_event_unit_mask(absy+addy, unit_mask);
		  if (unit_mask != (Mword)-1)
		    {
		      Perf_cnt::combine_event(evntsel, unit_mask, &event);
		      return event;
		    }
		  // else fall through
		case KEY_ESC:
		  return (Mword)-1;
		default:
		  if (Jdb::is_toplevel_cmd(c)) 
		    return (Mword)-1;
		}
	    }
	}
    }
}

static void
Jdb_tbuf_show::show_events(Mword start, Mword ref, Mword count, 
			   char mode, char time_mode, int long_output)
{
  Unsigned64 ref_tsc;
  Unsigned32 ref_pmc1, ref_pmc2;
  Mword dummy;

  Jdb_tbuf::event(ref, &dummy, &ref_tsc, &ref_pmc1, &ref_pmc2);

  for (Mword n=start; n<start+count; n++)
    {
      static char buffer[160];
      Mword number;
      Signed64 dtsc;
      Signed32 dpmc;
      Unsigned64 utsc;
      Unsigned32 upmc1, upmc2;

      Kconsole::console()->getchar_chance();

      if (!Jdb_tbuf::event(n, &number, &utsc, &upmc1, &upmc2))
	break;

      if (long_output)
	{
	  sprintf(buffer, "%10u.  ", number);
	  char *p = buffer + 13;
	  int maxlen = sizeof(buffer)-13, len;
	  
	  Jdb_tbuf_output::print_entry(n, p, 72);
	  p += 71;
	  *p++ = ' ';	// print_entry terminates with '\0'
	  maxlen -= 72;

	  if (!Jdb_tbuf::diff_tsc(n, &dtsc))
	    dtsc = 0;
	  
	  len = Jdb::write_ll_dec(dtsc, p, 12, false);
	  p += len;
	  *p++ = ' ';
	  *p++ = '(';
	  maxlen -= len+2;
	  len = Jdb::write_tsc_s(dtsc, p, 15, false);
	  p += len;
	  *p++ = ')';
	  *p++ = ' ';
	  *p++ = ' ';
	  maxlen -= len+3;
	  len = snprintf(p, maxlen, "%16lld", utsc);
	  p += len;
	  *p++ = ' ';
	  *p++ = '(';
	  maxlen -= len+2;
	  len = Jdb::write_tsc_s(utsc, p, 15, false);
	  p += len;
	  *p++ = ')';
	  *p++ = '\n';
	  maxlen -= len+2;

	  putstr(buffer);
	}
      else
	{
	  int maxlen = 12;
	  char *p = buffer + 80-maxlen;
	  int len = 0;

	  if (n == ref)
	    putstr(Jdb::esc_emph2);

	  Jdb_tbuf_output::print_entry(n, buffer, 80-maxlen);
	  p[-1] = ' ';	// print_entry terminates with '\0'
      
	  switch (mode)
	    {
	    case INDEX_MODE:
	      len = snprintf(p, maxlen, "%11u", number);
	      break;
	    case DELTA_TSC_MODE:
	      if (!Jdb_tbuf::diff_tsc(n, &dtsc))
		dtsc = 0;
	      switch (time_mode)
		{
		case 0: len = Jdb::write_ll_hex(dtsc, p, maxlen, false); break;
		case 1: len = Jdb::write_tsc(dtsc, p, maxlen, false); break;
		case 2: len = Jdb::write_ll_dec(dtsc, p, maxlen, false); break;
		}
	      break;
	    case REF_TSC_MODE:
	      dtsc = (n == ref) ? 0 : utsc - ref_tsc;
	      switch (time_mode)
		{
		case 0: len = Jdb::write_ll_hex(dtsc, p, maxlen, true); break;
		case 1: len = Jdb::write_tsc(dtsc, p, maxlen, true); break;
		case 2: len = Jdb::write_ll_dec(dtsc, p, maxlen, true); break;
		}
	      break;
	    case START_TSC_MODE:
	      dtsc = utsc;
	      switch (time_mode)
		{
		case 0: len = Jdb::write_ll_hex(dtsc, p, maxlen, true); break;
		case 1: len = Jdb::write_tsc(dtsc, p, maxlen, false); break;
		case 2: len = Jdb::write_ll_dec(dtsc, p, maxlen, true); break;
		}
	      break;
	    case DELTA_PMC1_MODE:
	    case DELTA_PMC2_MODE:
	      if (!Jdb_tbuf::diff_pmc(n, (mode-DELTA_PMC1_MODE), &dpmc))
		dpmc = 0;
	      len = Jdb::write_ll_dec((Signed64)dpmc, p, maxlen, false);
	      break;
	    case REF_PMC1_MODE:
	      dpmc = (n == ref) ? 0 : upmc1 - ref_pmc1;
	      len = Jdb::write_ll_dec((Signed64)dpmc, p, maxlen, true);
	      break;
	    case REF_PMC2_MODE:
	      dpmc = (n == ref) ? 0 : upmc2 - ref_pmc2;
	      len = Jdb::write_ll_dec((Signed64)dpmc, p, maxlen, true);
	      break;
	    }
	  
	  for (int i=len; i<maxlen; i++)
	    p[i] = ' ';
	  p[maxlen] = '\0';
	  putstr(buffer);
	  if (n == ref)
	    putstr("\033[m");
	  if (count != 1)
	    putchar('\n');
	}
    }
}

// search in tracebuffer
static Mword
Jdb_tbuf_show::search(Mword start, const char *str, Unsigned8 direction)
{
  int first = 1;

  if (!Jdb_tbuf::entries())
    return (Mword)-1;

  if (direction==1)
    start--;
  else
    start++;

  for (Mword n=start; ; (direction==1) ? n-- : n++)
    {
      static char buffer[120];

      if (first)
	first = 0;
      else if (n == start)
	break;

      if (!Jdb_tbuf::event_valid(n))
	n = (direction==1) ? Jdb_tbuf::entries()-1 : 0;

      Jdb_tbuf_output::print_entry(n, buffer, sizeof(buffer));
      if (strstr(buffer, str))
	return n;
    }

  return (Mword)-1;
}

static int
Jdb_tbuf_show::show()
{
  static char mode = INDEX_MODE;
  static char time_mode = 1;
  static char search_str[40];
  static Unsigned8 direction = 0;

  int have_tsc = Cpu::features() & FEAT_TSC;
  Mword fst_nr = 0;		// nr of first event seen on top of screen
  Mword absy  = show_tb_absy;	// idx or first event seen on top of screen
  Mword refy  = show_tb_refy;	// idx of reference event
  Mword addy  = 0;		// cursor position starting from top of screen
  Mword lines = Jdb_screen::height()-4;
  Mword entries = Jdb_tbuf::entries();

  if (Config::tbuf_entries < lines)
    lines = Config::tbuf_entries;

  if (entries < lines)
    lines = entries;
  if (lines < 1)
    lines = 1;
  if (refy > entries-1)
    refy = entries-1;

  Mword max_absy = entries > lines ? entries - lines : 0;

  if (entries)
    {
      Jdb_tbuf::event(absy, &fst_nr, 0, 0, 0);
      addy = fst_nr - show_tb_nr - 1;
      if ((show_tb_nr == (Mword)-1) || (addy > Jdb_screen::height()-4))
	addy = 0;
    }

  Jdb::fancy_clear_screen();

  for (;;)
    {
      if (entries)
	Jdb_tbuf::event(absy, &fst_nr, 0, 0, 0);

      Mword count, perf_event[2], perf_user[2], perf_kern[2], perf_edge[2];
      int have_perf=0;
      const char *perf_mode[2], *perf_name[2];

      for (Mword i=0; i<2; i++)
	have_perf |= Perf_cnt::perf_mode(i, 
					 &perf_mode[i],  &perf_name[i],
				         &perf_event[i], &perf_user[i], 
					 &perf_kern[i],  &perf_edge[i]);

      Jdb::cursor();
      printf("%3u%% of %-6u",
    	      entries*100/Config::tbuf_entries, Config::tbuf_entries);

      static const char * const mode_str[] =
	{ "index", "tsc diff", "tsc rel", "tsc start",
	  "pmc1 diff", "pmc2 diff", "pmc1 rel", "pmc2 rel" };

      if (have_perf)
	{
	  const char *perf_type = Perf_cnt::perf_type();

     	  printf(" Perf:%-4s,1=%06x(%s%s\033[m:%s)\033[K", perf_type,
	      perf_event[0], Jdb::esc_emph, perf_mode[0], perf_name[0]);
	  Jdb::cursor(1, 70);
      	  printf("%10s\n", mode_str[(int)mode]);
	  printf("%24s 2=%06x(%s%s\033[m:%s)\033[K\n", "",
	      perf_event[1], Jdb::esc_emph, perf_mode[1], perf_name[1]);
	}
      else
	{
	  printf(" (performance counters not available)\033[K");
	  Jdb::cursor(1, 70);
	  printf("%10s\n\033[K", mode_str[(int)mode]);
	}

      Jdb::cursor(3, 1);
      for (Mword i=3; i<SHOW_TRACEBUFFER_START; i++)
	puts("\033[K");
      
      show_events(absy, refy, lines, mode, time_mode, 0);

      for (Mword i=SHOW_TRACEBUFFER_START+lines; i<Jdb_screen::height(); i++)
	puts("\033[K");

 status_line:
      Jdb::printf_statline("T%73s",
       "/=search c=clear n=next r=ref D=dump P=perf <Space>=mode <CR>=select");
      
      for (bool redraw=false; !redraw;)
	{
	  Mword pair_event, pair_y;
	  Smword c;
	  Unsigned8 type;
	  Unsigned8 d = 0; // default search direction is forward
	  
	  // search for paired ipc event
	  pair_event = Jdb_tbuf::ipc_pair_event(absy+addy, &type);
	  if (pair_event == (Mword)-1)
	    // search for paired pagefault event
	    pair_event = Jdb_tbuf::pf_pair_event(absy+addy, &type);
	  if (pair_event > fst_nr || pair_event <= fst_nr-lines)
	    pair_event = (Mword)-1;
	  pair_y = fst_nr-pair_event;
	  if (pair_event != (Mword)-1)
	    {
	      Jdb::cursor(pair_y+SHOW_TRACEBUFFER_START, 1);
	      putstr(Jdb::esc_emph);
	      switch (type)
		{
		case Jdb_tbuf::RESULT: putstr("+++>"); break;
		case Jdb_tbuf::EVENT:  putstr("<+++"); break;
		}
	      putstr("\033[m");
	    }
	  Jdb::cursor(addy+SHOW_TRACEBUFFER_START, 1);
	  putstr(Jdb::esc_emph);
	  show_events(absy+addy, refy, 1, mode, time_mode, 0);
	  putstr("\033[m");
	  Jdb::cursor(addy+SHOW_TRACEBUFFER_START, 1);
	  c=getchar();
	  show_events(absy+addy, refy, 1, mode, time_mode, 0);
	  if (pair_event != (Mword)-1)
	    {
	      Jdb::cursor(pair_y+SHOW_TRACEBUFFER_START, 1);
	      show_events(absy+pair_y, refy, 1, mode, time_mode, 0);
	    }
	  if (!Jdb::std_cursor_key(c, lines, max_absy, 
				   &absy, &addy, 0, &redraw))
	    {
	      switch (c)
		{
		case 'D':
		  if (Kconsole::console()->gzip_available())
		    {
		      // dump to console
		      Jdb::cursor(Jdb_screen::height(), 17);
		      Jdb::clear_to_eol();
		      printf("Count=");

		      if (Jdb_input::get_mword(&count, 7, 10))
			{
			  if (count == 0)
			    count = lines;
			  Kconsole::console()->gzip_enable();
			  show_events(absy, refy, count, mode, time_mode, 1);
			  Kconsole::console()->gzip_disable();
			}
		    }
		  break;
		case 'P': // set performance counter
		  if (have_perf)
		    {
		      Mword event, e, nr;
		      int user, kern, edge;

		      Jdb::printf_statline("P%73s", "1..2=select counter");
		      Jdb::cursor(Jdb_screen::height(), Jdb::LOGO+1);
		      for (;;)
			{
			  nr = getchar();
			  if (nr == KEY_ESC)
			    return false;
			  if (nr >= '1' && nr <= '2')
			    {
			      nr -= '1';
			      break;
			    }
			}

		      event = perf_event[nr];
		      user  = perf_user[nr];
		      kern  = perf_kern[nr];
		      edge  = perf_edge[nr];

		      Jdb::printf_statline("P%c%72s", '1'+nr,
		      "d=duration e=edge u=user k=kern +=both -=none ?=event");
		      Jdb::cursor(Jdb_screen::height(), Jdb::LOGO+2);
		      switch(c=getchar())
			{
			case '+': user =    kern = 1; break;
			case '-': user =    kern = 0; break;
			case 'u': user = 1; kern = 0; break;
			case 'k': user = 0; kern = 1; break;
			case 'd': edge = 0;           break;
			case 'e': edge = 1;           break;
			case '?':
			case '_':
			  if ((e = select_perf_event(event)) == (Mword)-1)
			    {
			      redraw = true;
			      break;
			    }
			  event = e;
			  redraw = true;
			  break;
			default:
			  Jdb_input::get_mword(&event, 4, 16);
			  break;
			}
		  
		      Perf_cnt::setup_pmc(nr, event, user, kern, edge);
		      redraw = true;
		    }
		  break;
		case KEY_RETURN:
		  if (jdb_disasm_addr_task != 0)
		    {
		      L4_uid tid;
		      Mword eip;
		      if (   Jdb_tbuf_output::thread_eip(absy+addy, &tid, &eip)
			  && Threadid(&tid).lookup()->is_valid())
			{
			  if (!jdb_disasm_addr_task(eip, tid.task(), 1))
			    goto return_false;
			  redraw = true;
			}
		    }
		  break;
		case KEY_CURSOR_LEFT: // mode switch
		  mode--;
		  if (mode < 0)
		    {
		      if (!have_tsc)
			mode = INDEX_MODE;
		      else if (!have_perf)
			mode = START_TSC_MODE;
		      else
			mode = REF_PMC2_MODE;
		    }
		  redraw = true;
		  break;
		case KEY_CURSOR_RIGHT: // mode switch
		  mode++;
		  if (   (!have_tsc  && (mode>INDEX_MODE))
		      || (!have_perf && (mode>START_TSC_MODE))
		      || (              (mode>REF_PMC2_MODE)))
		    mode = INDEX_MODE;
		  redraw = true;
		  break;
		case ' ': // mode switch
		  if (mode != INDEX_MODE)
		    {
		      time_mode = (time_mode+1) % 3;
		      redraw = true;
		    }
		  break;
		case 'c': // clear tracebuffer
		  Jdb_tbuf::clear_tbuf();
		  Jdb::abort_command();
		  show_tb_absy = 0;
		  show_tb_refy = 0;
		  show_tb_nr   = (Mword)-1;
		  return true;
		case'r': // set reference entry
		  refy = absy + addy;
		  redraw = true;
		  break;
		case '?': // search bachward
		  d = 1;
		  // fall through
		case '/': // search forward
		  direction = d;
		  // search in tracebuffer events
		  Jdb::printf_statline("Search=%s", search_str);
		  Jdb::cursor(Jdb_screen::height(), 13+strlen(search_str));
		  get_string(search_str, sizeof(search_str)); 
		  // fall through
		case 'n': // search next
		    {
		      Mword n = search(absy+addy, search_str, direction);
		      if (n != (Mword)-1)
			{
			  // found
			  if ((n < absy) || (n > absy+lines-1))
			    {
			      // screen crossed
			      addy = 4;
			      absy = n - addy;
			      if (n < addy)
				{
				  addy = n;
				  absy = 0;
				}
			      if (absy > max_absy)
				{
				  absy = max_absy;
				  addy = n - absy;
				}
			      redraw = true;
			      break;
			    }
			  else
			    addy = n - absy;
			}
		    }
		  goto status_line;
		case KEY_ESC:
		  Jdb::abort_command();
		  goto return_false;
		default:
		  if (Jdb::is_toplevel_cmd(c)) 
		    {
		return_false:
		      show_tb_absy = absy;
		      show_tb_refy = refy;
		      show_tb_nr   = fst_nr-addy-1;
		      return false;
		    }
		}
	    }
	}
    }
}

PUBLIC
Jdb_module::Action_code
Jdb_tbuf_show::action(int cmd, void *&, char const *&, int &)
{
  switch (cmd)
    {
    case 0:
      show();
      break;

    case 1:
      if (Kconsole::console()->gzip_available())
	{
	  Kconsole::console()->gzip_enable();
	  show_events(0, 0, 1000000, 0, 0, 1);
	  Kconsole::console()->gzip_disable();
	}
      break;
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_tbuf_show::cmds() const
{
  static Cmd cs[] =
    { 
      Cmd(0, "T", "tbuf", "",
	  "T{P{+|-|k|u|<event>}}\tenter tracebuffer, on/off/kernel/user perf",
	  0),
      Cmd(1, "Tgzip", "", "", 0 /* invisible */, 0),
    };

  return cs;
}

PUBLIC
int const
Jdb_tbuf_show::num_cmds() const
{
  return 2;
}

PUBLIC
Jdb_tbuf_show::Jdb_tbuf_show()
    : Jdb_module("MONITORING")
{}

static Jdb_tbuf_show jdb_tbuf_show INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


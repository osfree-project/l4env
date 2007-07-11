IMPLEMENTATION [jdb_thread_names]:

#include "jdb_thread_names.h"

static inline
const char*
get_thread_name(Global_id id)
{ return Jdb_thread_names::lookup(id, true)->name(); }

IMPLEMENTATION [!jdb_thread_names]:

static inline
const char*
get_thread_name(Global_id)
{ return "<noname>"; }


IMPLEMENTATION:

#include <climits>
#include <cstring>
#include <cstdio>

#include "jdb.h"
#include "jdb_core.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "simpleio.h"
#include "thread.h"
#include "thread_state.h"
#include "static_init.h"

class Jdb_list_threads : public Jdb_module
{
public:
  Jdb_list_threads() FIASCO_INIT;
private:
  static char subcmd;
  static char long_output;
};

class Jdb_thread_list
{
private:
  static int _mode;
  static int _count;
  static char _pr;
  static Thread *_t_head, *_t_start;

  friend class _foo;

  enum { LIST_UNSORTED, LIST_SORT_PRIO, LIST_SORT_TID };
};

char Jdb_list_threads::subcmd;
char Jdb_list_threads::long_output;


// available from the jdb_tcb module
extern int jdb_show_tcb(L4_uid tid, int level)
  __attribute__((weak));

char Jdb_thread_list::_pr;
int  Jdb_thread_list::_mode = LIST_SORT_TID;
int  Jdb_thread_list::_count;

Thread *Jdb_thread_list::_t_head;
Thread *Jdb_thread_list::_t_start;

PUBLIC static
void
Jdb_thread_list::init(char pr, Thread *t_head)
{
  _pr = pr;
  _t_head = t_head;
}

// return string describing current sorting mode of list
PUBLIC static inline NOEXPORT
const char*
Jdb_thread_list::get_mode_str(void)
{
  static const char * const mode_str[] =
    { "(unsorted)", "(prio-sorted)", "(tid-sorted)" };

  return mode_str[_mode];
}

// switch to next sorting mode
PUBLIC static
void
Jdb_thread_list::switch_mode(void)
{
  if (++_mode > LIST_SORT_TID)
    _mode = LIST_UNSORTED;
}

// set _t_start element of list
PUBLIC static
void
Jdb_thread_list::set_start(Thread *t_start)
{
  _t_start = t_start;
  iter(+Jdb_screen::height()-3, &_t_start);
  iter(-Jdb_screen::height()+3, &_t_start);
}

// _t_start-- if possible
PUBLIC static
int
Jdb_thread_list::line_back(void)
{
  return iter(-1, &_t_start);
}

// _t_start++ if possible
PUBLIC static
int
Jdb_thread_list::line_forw(void)
{
  Thread *t = _t_start;
  iter(+Jdb_screen::height()-2, &_t_start);
  iter(-Jdb_screen::height()+3, &_t_start);
  return t != _t_start;
}

// _t_start -= 24 if possible
PUBLIC static
int
Jdb_thread_list::page_back(void)
{
  return iter(-Jdb_screen::height()+2, &_t_start);
}

// _t_start += 24 if possible
PUBLIC static
int
Jdb_thread_list::page_forw(void)
{
  Thread *t = _t_start;
  iter(+Jdb_screen::height()*2-5, &_t_start);
  iter(-Jdb_screen::height()  +3, &_t_start);
  return t != _t_start;
}

// _t_start = first element of list
PUBLIC static
int
Jdb_thread_list::goto_home(void)
{
  return iter(-9999, &_t_start);
}

// _t_start = last element of list
PUBLIC static
int
Jdb_thread_list::goto_end(void)
{
  Thread *t = _t_start;
  iter(+9999, &_t_start);
  iter(-Jdb_screen::height()+2, &_t_start);
  return t != _t_start;
}

// search index of t_search starting from _t_start
PUBLIC static
int
Jdb_thread_list::lookup(Thread *t_search)
{
  unsigned i;
  Thread *t;
  
  for (i=0, t=_t_start; i<Jdb_screen::height()-3; i++)
    {
      if (t == t_search)
	break;
      iter(+1, &t);
    }

  return i;
}

// get y'th element of thread list starting from _t_start
PUBLIC static
Thread*
Jdb_thread_list::index(int y)
{
  Thread *t = _t_start;

  iter(y, &t);
  return t;
}

// helper function for iter() -- use priority as sorting key
static
int
Jdb_thread_list::get_prio(Thread *t)
{
  return t->sched()->prio();
}

// helper function for iter() -- use thread id as sorting key
static
int
Jdb_thread_list::get_tid(Thread *t)
{
  return t->_id.gthread();
}

static inline NOEXPORT
Thread*
Jdb_thread_list::iter_prev(Thread *t)
{
  if (_pr == 'p')
    return t->present_prev;
  else
    {
      unsigned prio = t->sched()->prio();

      if (t != Context::_prio_next.cpu(0)[prio])
      	return Thread::lookup(t->_ready_prev);

      for (;;)
	{
	  if (++prio > Context::_prio_highest.cpu(0))
	    prio = 0;
	  if (Context::_prio_next.cpu(0)[prio])
	    return Thread::lookup(Context::_prio_next.cpu(0)[prio]->_ready_prev);
	}
    }
}

static inline NOEXPORT
Thread*
Jdb_thread_list::iter_next(Thread *t)
{
  if (_pr == 'p')
    return t->present_next;
  else
    {
      unsigned prio = t->sched()->prio();

      if (t->_ready_next != Context::_prio_next.cpu(0)[prio])
      	return Thread::lookup(t->_ready_next);

      for (;;)
	{
	  if (--prio > Context::_prio_highest.cpu(0)) // prio is unsigned
	    prio = Context::_prio_highest.cpu(0);
	  if (Context::_prio_next.cpu(0)[prio])
	    return Thread::lookup(Context::_prio_next.cpu(0)[prio]);
	}
    }
}

// walk though list <count> times
// abort walking if no more elements
// do iter if iter != 0
static
bool
Jdb_thread_list::iter(int count, Thread **t_start,
		      void (*iter)(Thread *t)=0)
{
  int i = 0;
  int forw = (count >= 0);
  Thread *t, *t_new = *t_start, *t_head = _t_head;
  int (*get_key)(Thread *t) = 0;

  if (count == 0)
    return false;  // nothing changed
  
  if (count < 0)
    count = -count;

  // if we are stepping backwards, begin at end-of-list
  if (!forw)
    t_head = iter_prev(t_head);

  switch (_mode)
    {
    case LIST_UNSORTED:
      // list threads in order of list
      if (iter)
	iter(*t_start);
	  
      t = *t_start;
      do
	{
	  t = forw ? iter_next(t) : iter_prev(t);
	  
	  if (t == t_head)
	    break;
	  
	  if (iter)
	    iter(t);
	  
	  t_new = t;
	  i++;
	  
	} while (i < count);
      break;
    
    case LIST_SORT_PRIO:
      // list threads sorted by priority
      if (!get_key)
	get_key = get_prio;

      // fall through
    
    case LIST_SORT_TID:
      // list threads sorted by thread id
	{
	  int key;
	  int start_skipped = 0;
	  
	  if (!get_key)
	    get_key = get_tid;
	  
	  int key_current = get_key(*t_start);
	  int key_next = (forw) ? INT_MIN : INT_MAX;
	  
      	  t = t_head;
	  if (iter)
	    iter(*t_start);
	  do
	    {
	      if (t == *t_start)
		start_skipped = 1;
	      
	      key = get_key(t);
	      
	      // while walking through the current list, look for next key
	      if (   ( forw && (key > key_next) && (key < key_current))
		  || (!forw && (key < key_next) && (key > key_current)))
		key_next = key;
	      
    	      if (t_head == (t = (forw) ? iter_next(t) : iter_prev(t)))
		{
		  if (   ( forw && (key_next == INT_MIN))
		      || (!forw && (key_next == INT_MAX)))
		    break;
		  key_current = key_next;
		  key_next = forw ? INT_MIN : INT_MAX;
		}
	      
	      if (start_skipped && (get_key(t) == key_current))
		{
		  if (iter)
		    iter(t);
		  
		  i++;
		  t_new = t;
		}
	    } while (i < count);
	}
      break;
    }

  _count = i;
  
  bool changed = (*t_start != t_new);
  *t_start = t_new;
  
  return changed;
}

// show complete page using show callback function
PUBLIC static
int
Jdb_thread_list::page_show(void (*show)(Thread *t))
{
  Thread *t = _t_start;
  
  iter(Jdb_screen::height()-3, &t, show);
  return _count;
}

// show complete list using show callback function
PUBLIC static
int
Jdb_thread_list::complete_show(void (*show)(Thread *t))
{
  Thread *t = _t_start;
  
  iter(9999, &t, show);
  return _count;
}

IMPLEMENT
Jdb_list_threads::Jdb_list_threads()
  : Jdb_module("INFO")
{}

PUBLIC
Jdb_module::Action_code
Jdb_list_threads::action(int cmd, void *&, char const *&, int &)
{
  if (cmd == 0)
    {
      Thread *t = Jdb::get_current_active();
      switch (subcmd)
	{
	case 'r': list_threads(t, 'r'); break;
	case 'p': list_threads(t, 'p'); break;
	case 't': Jdb::execute_command("lt"); break; // other module
	}
    }
  else if (cmd == 1)
    {
      Console *gzip = Kconsole::console()->find_console(Console::GZIP);
      if (gzip)
	{
	  Thread *t = Jdb::get_current_active();
	  gzip->state(gzip->state() | Console::OUTENABLED);
	  long_output = 1;
	  Jdb_thread_list::init('p', t);
	  Jdb_thread_list::set_start(t);
	  Jdb_thread_list::goto_home();
	  Jdb_thread_list::complete_show(list_threads_show_thread);
	  long_output = 0;
	  gzip->state(gzip->state() & ~Console::OUTENABLED);
	}
      else
	puts(" gzip module not available");
    }

  return NOTHING;
}

static void
Jdb_list_threads::list_threads_show_thread(Thread *t)
{
  char to[24];
  int  waiting_for = 0;
  
  *to = '\0';

  Kconsole::console()->getchar_chance();

  if (t->has_privileged_iopl() && !long_output)
    printf("%s", Jdb::esc_emph);
  t->print_uid(3);
  if (t->has_privileged_iopl() && !long_output)
    putstr("\033[m");

  const char *name = get_thread_name(t->id());
  printf(" %-18.18s", name);

  printf("  %2x ", t->sched()->prio());
  if (t->state() & (Thread_receiving | Thread_rcvlong_in_progress))
    {
      t->print_partner(3);
      waiting_for = 1;
    } 
  else
    putstr("       ");

  if (waiting_for)
    {
      if (t->_timeout && t->_timeout->is_set())
	{
	  Signed64 diff = (t->_timeout->get_timeout());
	  if (diff < 0)
	    strcpy(to, " over");
	  else if (diff >= 100000000LL)
	    strcpy(to, " >99s");
	  else
	    {
	      int us = (int)diff;
	      if (us < 0)
		us = 0;
	      if (us >= 1000000)
		snprintf(to, sizeof(to), " %3us", us / 1000000);
	      else if (us >= 1000)
		snprintf(to, sizeof(to), " %3um", us / 1000);
	      else
		snprintf(to, sizeof(to), " %3u%c", us, Config::char_micro);
	    }
	}
    }

  printf("%-6s", to);

  if (long_output)
    {
      t->print_state_long(50);
      putchar('\n');
    }
  else
    {
      if (Config::stack_depth)
	{
	  Mword i, stack_depth;
	  char *c  = (char*)t + sizeof(Thread);
	  for (i=sizeof(Thread), stack_depth=Config::thread_block_size; 
	      i<Config::thread_block_size; 
	      i++, stack_depth--, c++)
	    if (*c != '5')
	      break;

	  printf("(%4ld) ", stack_depth-sizeof(Thread));
	  t->print_state_long(29);
	}
      else
	t->print_state_long(36);
      putstr("\033[K\n");
    }
}

static void
Jdb_list_threads::show_header()
{
  Jdb::cursor();
  printf("%s    id name                pr   wait    to%s state\033[m\033[K",
      Jdb::esc_emph, Config::stack_depth ? "  stack" : "");
}

static void
Jdb_list_threads::list_threads(Thread *t_start, char pr)
{
  unsigned y, y_max;
  Thread *t, *t_current = t_start;

  // sanity check
  if (!t_current->is_valid())
    {
      printf(" No threads\n");
      return;
    }

  // enqueue current, which may not be in the ready list due to lazy queueing
  if (!t_current->in_ready_list())
    t_current->ready_enqueue();

  Jdb::clear_screen();
  show_header();
  Jdb_thread_list::init(pr, t_current);

  for (;;)
    {
      Jdb_thread_list::set_start(t_current);

      // set y to position of t_current in current displayed list
      y = Jdb_thread_list::lookup(t_current);

      for (bool resync=false; !resync;)
	{
	  Jdb::cursor(2, 1);
	  y_max = Jdb_thread_list::page_show(list_threads_show_thread);

	  // clear rest of screen (if where less than 24 lines)
	  for (unsigned i=y_max; i < Jdb_screen::height()-3; i++)
       	    putstr("\033[K\n");

	  Jdb::printf_statline(pr=='r' ? "ready list" : "present list",
			       "<Space>=mode <Tab>=partner <CR>=select",
			       "%-15s", Jdb_thread_list::get_mode_str());
	  
	  // key event loop
	  for (bool redraw=false; !redraw; )
	    {
	      Jdb::cursor(y+2, 6);
	      switch (int c=Jdb_core::getchar())
		{
		case KEY_CURSOR_UP:
		case 'k':
		  if (y > 0)
		    y--;
		  else
		    redraw = Jdb_thread_list::line_back();
		  break;
		case KEY_CURSOR_DOWN:
		case 'j':
		  if (y < y_max)
		    y++;
		  else
		    redraw = Jdb_thread_list::line_forw();
		  break;
		case KEY_PAGE_UP:
		case 'K':
		  if (!(redraw = Jdb_thread_list::page_back()))
		    y = 0;
		  break;
		case KEY_PAGE_DOWN:
		case 'J':
		  if (!(redraw = Jdb_thread_list::page_forw()))
		    y = y_max;
		  break;
		case KEY_CURSOR_HOME:
		case 'H':
		  redraw = Jdb_thread_list::goto_home();
		  y = 0;
		  break;
		case KEY_CURSOR_END:
		case 'L':
		  redraw = Jdb_thread_list::goto_end();
		  y = y_max;
		  break;
		case ' ': // switch mode
		  t_current = Jdb_thread_list::index(y);
		  Jdb_thread_list::switch_mode();
		  redraw = true;
		  resync = true;
		  break;
		case KEY_TAB: // goto thread we are waiting for
		  t = Jdb_thread_list::index(y);
		  if (t->partner()
		      && (t->state() & (Thread_receiving |
					Thread_busy  |
					Thread_rcvlong_in_progress))
		      && (!t->partner()->id().is_irq() ||
		           t->partner()->id().irq() > Config::Max_num_irqs))
		    {
		      t_current = static_cast<Thread*>(t->partner());
		      redraw = true;
		      resync = true;
		    }
		  break;
		case KEY_RETURN: // show current tcb
		  if (jdb_show_tcb != 0)
		    {
		      t = Jdb_thread_list::index(y);
		      if (!jdb_show_tcb(t->id(), 1))
			return;
		      show_header();
		      redraw = 1;
		    }
		  break;
		case KEY_ESC:
		  Jdb::abort_command();
		  return;
		default:
		  if (Jdb::is_toplevel_cmd(c)) 
		    return;
		}
	    }
	}
    }
}

PUBLIC
static void
Jdb_list_threads::show_thread_list()
{
  Jdb_core::getchar(); // ignore r/p

  Thread *t = Jdb::get_thread();

  // sanity check
  if (!t->is_valid())
    {
      printf(" No threads\n");
      return;
    }
}

PUBLIC
Jdb_module::Cmd const *
Jdb_list_threads::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "l", "list", "%c\n", "l{r|p}\tshow ready/present list", &subcmd },
        { 1, "lgzip", "", "", 0 /* invisible */, 0 },
    };

  return cs;
}

PUBLIC
int
Jdb_list_threads::num_cmds() const
{
  return 2;
}

static Jdb_list_threads jdb_list_threads INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

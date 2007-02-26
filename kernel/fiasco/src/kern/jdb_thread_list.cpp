INTERFACE:

#include "thread.h"

class jdb_thread_list
{
private:
  static int _mode;
  static int _count;
  static char _pr;
  
  static Thread *_t_head;
  static Thread *_t_start;

  friend class _foo;
};

IMPLEMENTATION:

#include <limits.h>


#define LIST_UNSORTED	0
#define LIST_SORT_PRIO	1
#define LIST_SORT_TID	2

char jdb_thread_list::_pr;
int jdb_thread_list::_mode = LIST_SORT_TID;
int jdb_thread_list::_count;

Thread *jdb_thread_list::_t_head;
Thread *jdb_thread_list::_t_start;


PUBLIC static
void
jdb_thread_list::init(char pr, Thread *t_head)
{
  _pr = pr;
  _t_head = t_head;
}

// return string describing current sorting mode of list
PUBLIC static
const char*
jdb_thread_list::get_mode_str(void)
{
  static const char *mode_str[] =
    { "(unsorted)", "(prio-sorted)", "(tid-sorted)" };
  
  return mode_str[_mode];
}

// switch to next sorting mode
PUBLIC static
void
jdb_thread_list::switch_mode(void)
{
  if (++_mode > LIST_SORT_TID)
    _mode = LIST_UNSORTED;
}

// set _t_start element of list
PUBLIC static
void
jdb_thread_list::set_start(Thread *t_start)
{
  _t_start = t_start;
  iter(+23, &_t_start);
  iter(-23, &_t_start);
}

// _t_start-- if possible
PUBLIC static
int
jdb_thread_list::line_back(void)
{
  return iter(-1, &_t_start);
}

// _t_start++ if possible
PUBLIC static
int
jdb_thread_list::line_forw(void)
{
  Thread *t = _t_start;
  iter(+24, &_t_start);
  iter(-23, &_t_start);
  return t != _t_start;
}

// _t_start -= 24 if possible
PUBLIC static
int
jdb_thread_list::page_back(void)
{
  return iter(-24, &_t_start);
}

// _t_start += 24 if possible
PUBLIC static
int
jdb_thread_list::page_forw(void)
{
  Thread *t = _t_start;
  iter(+47, &_t_start);
  iter(-23, &_t_start);
  return t != _t_start;
}

// _t_start = first element of list
PUBLIC static
int
jdb_thread_list::goto_home(void)
{
  return iter(-9999, &_t_start);
}

// _t_start = last element of list
PUBLIC static
int
jdb_thread_list::goto_end(void)
{
  Thread *t = _t_start;
  iter(+9999, &_t_start);
  iter(-23, &_t_start);
  return t != _t_start;
}

// search index of t_search starting from _t_start
PUBLIC static
int
jdb_thread_list::lookup(Thread *t_search)
{
  int i;
  Thread *t;
  
  for (i=0, t=_t_start; i<23; i++)
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
jdb_thread_list::index(int y)
{
  Thread *t = _t_start;

  iter(y, &t);
  return t;
}

// helper function for iter() -- use priority as sorting key
static
int
jdb_thread_list::get_prio(Thread *t)
{
  return t->sched()->prio();
}

// helper function for iter() -- use thread id as sorting key
static
int
jdb_thread_list::get_tid(Thread *t)
{
  return t->_id.gthread();
}

static inline
Thread*
jdb_thread_list::iter_prev(Thread *t)
{
  switch (_pr)
    {
    case 'p':
      return t->present_prev;
    default:
    case 'r':
      return Thread::lookup(t->ready_prev);
    }
}

static inline
Thread*
jdb_thread_list::iter_next(Thread *t)
{
  switch (_pr)
    {
    case 'p':
      return t->present_next;
    default:
    case 'r':
      return Thread::lookup(t->ready_next);
    }
}

// walk though list <count> times
// abort walking if no more elements
// do iter if iter != 0
static
bool
jdb_thread_list::iter(int count, Thread **t_start,
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
jdb_thread_list::page_show(void (*show)(Thread *t))
{
  Thread *t = _t_start;
  
  iter(23, &t, show);
  return _count;
}

// show complete list using show callback function
PUBLIC static
int
jdb_thread_list::complete_show(void (*show)(Thread *t))
{
  Thread *t = _t_start;
  
  iter(9999, &t, show);
  return _count;
}


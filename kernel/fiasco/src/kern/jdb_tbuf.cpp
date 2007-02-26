INTERFACE:

#include "l4_types.h"
#include "jdb_ktrace.h"
#include "tb_entry.h"

class Log_event;
class Context;
class Observer;
class Sys_ipc_frame;

class Jdb_tbuf
{
public:
  static Log_event * const log_events[];

  static void (*direct_log_entry)(Tb_entry*, const char*);

  /** @brief check if event is valid
   * @param idx position of event in tracebuffer
   * @return 0 if not valid, 1 if valid */
  static int event_valid(Mword idx);
  
  /** @brief return pointer to tracebuffer event
   * @param  position of event in tracebuffer: 
   *         0 is last event, 1 the event before and so on
   * @return pointer to tracebuffer event */
  static Tb_entry *lookup(Mword idx);

  /** @brief return number of entries in tracebuffer
   * @return number of entries */
  static Mword entries (void);

  /** @brief return some information about log event
   * @param idx number of event to determine the info
   * @retval number event number
   * @retval tsc event value of CPU cycles
   * @retval pmc event value of perf counter cycles
   * @return 0 if something wrong, 1 if everything ok */
  static int event (Mword idx, Mword *number, 
		    Unsigned64 *tsc, Unsigned32 *pmc1, Unsigned32 *pmc2);

  /** @brief search the paired event to an ipc event or ipc result event
   * @param idx number of event to search the pair event for
   * @retval type type of pair event
   * @return number of pair event */
  static Mword ipc_pair_event(Mword idx, Unsigned8 *type);

  /** @brief search the paired event to a pagefault / result event
   * @param idx position of event in tracebuffer to search the pair event for
   * @retval type type of pair event
   * @return number of pair event */
  static Mword pf_pair_event(Mword idx, Unsigned8 *type);

  /** @brief get difference CPU cycles between event idx and event idx+1
   * @param idx position of first event in tracebuffer
   * @retval difference in CPU cycles
   * @return 0 if something wrong, 1 if everything ok */ 
  static int diff_tsc(Mword idx, Signed64 *delta);

  /** @brief get difference perfcnt cycles between event idx and event idx+1
   * @param idx position of first event in tracebuffer
   * @param nr  number of perfcounter (0=first, 1=second)
   * @retval difference in perfcnt cycles
   * @return 0 if something wrong, 1 if everything ok */ 
  static int diff_pmc(Mword idx, Mword nr, Signed32 *delta);

  enum
    { 
      EVENT  = 1,
      RESULT = 2
    };

protected:
  static Tb_entry_fit64	*tbuf;		// entry buffer
  static Tb_entry_fit64	*tbuf_act;	// current entry
  static Tb_entry_fit64 *tbuf_max;
  static Tracebuffer_status *status;
  static Mword		_entries;	// number of entries
  static Mword		number;		// current event number
  static Mword		count_mask1;
  static Mword		count_mask2;

  static Observer	*observer;
};

class Log_patch
{
  friend class Log_event;

private:
  Log_patch		*next;
  Unsigned8		*activator;
};

class Log_event
{
  const char		*event_name;
  Log_patch		*patch;
  int                   event_type;
};


#ifndef CONFIG_JDB_LOGGING

#define BEGIN_LOG_EVENT(name)	if (0) {
#define END_LOG_EVENT(name)	}

#else

#define BEGIN_LOG_EVENT(name)						\
  do									\
    {									\
      Unsigned8 __do_log__;						\
      asm volatile (".globl  patch_"#name"		\n\t"		\
		    "patch_"#name":			\n\t"		\
		    "movb    $0,%0			\n\t"		\
		    : "=q"(__do_log__) );				\
      if (EXPECT_FALSE( __do_log__ ))					\
	{

#define END_LOG_EVENT(name)						\
	}								\
    } while (0)

#endif


IMPLEMENTATION:

#include <cstring>
#include <cstdarg>

#include "boot_info.h"
#include "config.h"
#include "initcalls.h"
#include "observer.h"
#include "std_macros.h"

Tb_entry_fit64 *Jdb_tbuf::tbuf;
Tb_entry_fit64 *Jdb_tbuf::tbuf_act;
Tb_entry_fit64 *Jdb_tbuf::tbuf_max;
Tracebuffer_status *Jdb_tbuf::status;
Mword Jdb_tbuf::_entries;
Mword Jdb_tbuf::number;
Mword Jdb_tbuf::count_mask1;
Mword Jdb_tbuf::count_mask2;
Observer *Jdb_tbuf::observer;

static void direct_log_dummy(Tb_entry*, const char*)
{}

void (*Jdb_tbuf::direct_log_entry)(Tb_entry*, const char*) = &direct_log_dummy;

#ifdef CONFIG_JDB_LOGGING
#define DECLARE_PATCH(VAR, NAME)		\
  extern "C" char patch_##NAME;			\
  static Log_patch VAR(&patch_##NAME)


// logging of context switch not implemented in Assembler shortcut but we
// can switch to C shortcut
DECLARE_PATCH (lp1, log_context_switch);
static Log_event le1("context switch",
		      LOG_EVENT_CONTEXT_SWITCH, 1, &lp1);

// logging of shortcut succeeded/failed not implemented in Assembler shortcut
// but we can switch to C shortcut
DECLARE_PATCH (lp2, log_shortcut_failed_1);
DECLARE_PATCH (lp3, log_shortcut_failed_2);
DECLARE_PATCH (lp4, log_shortcut_succeeded);
static Log_event le2("ipc shortcut",
		      LOG_EVENT_IPC_SHORTCUT, 3, &lp2, &lp3, &lp4);

DECLARE_PATCH (lp5, log_irq);
static Log_event le3("irq raised",
		      LOG_EVENT_IRQ_RAISED, 1, &lp5);

DECLARE_PATCH (lp6, log_timer_irq);
static Log_event le4("timer irq raised",
		      LOG_EVENT_TIMER_IRQ, 1, &lp6);

#ifndef CONFIG_ABI_V4
DECLARE_PATCH (lp7, log_thread_ex_regs);
static Log_event le5("thread_ex_regs",
		      LOG_EVENT_THREAD_EX_REGS, 1, &lp7);
#endif

#ifdef CONFIG_IA32
DECLARE_PATCH (lp8, log_trap);
DECLARE_PATCH (lp9, log_trap_n);
static Log_event le6("trap raised",
		      LOG_EVENT_TRAP, 2, &lp8, &lp9);
#else
DECLARE_PATCH (lp8, log_trap);
static Log_event le6("trap raised",
		      LOG_EVENT_TRAP, 1, &lp8);
#endif

DECLARE_PATCH (lp10, log_pf_res);
static Log_event le7("pagefault result",
		      LOG_EVENT_PF_RES, 1, &lp10);

DECLARE_PATCH (lp11, log_sched_load);
DECLARE_PATCH (lp12, log_sched_save);
static Log_event le8("scheduling event",
                      LOG_EVENT_SCHED, 2, &lp11, &lp12);

Log_event * const Jdb_tbuf::log_events[LOG_EVENT_MAX_EVENTS] =
{ 
  &le1,	// context switch
  &le2, // ipc shortcut
  &le3, // irq raised
  &le4, // timer irq raised
#ifndef CONFIG_ABI_V4
  &le5, // thread_ex_regs
#else
  0,
#endif
  &le6, // trap raised
  &le7, // pagefault result
  &le8,	// scheduling event
  0,    // terminate list
};
#endif


// clear tracebuffer
PUBLIC static
void
Jdb_tbuf::clear_tbuf()
{
  Mword i;

  for (i=0; i<Config::tbuf_entries; i++)
    tbuf[i].clear();

  tbuf_act = tbuf;
  _entries = 0;
}

// return pointer to new tracebuffer entry
PUBLIC static
Tb_entry*
Jdb_tbuf::new_entry()
{
  Tb_entry *tb = tbuf_act;
 
  if (++tbuf_act >= tbuf_max)
    tbuf_act = tbuf;

  if (_entries < Config::tbuf_entries)
    _entries++;

  tb->number(++number);
  tb->rdtsc();
  tb->rdpmc1();
  tb->rdpmc2();

  return tb;
}

// commit tracebuffer entry
PUBLIC static
void
Jdb_tbuf::commit_entry()
{
  if (EXPECT_FALSE( (number & count_mask2) == 0 ))
    {
      if (number & count_mask1)
	status->version0++;
      else
	status->version1++;

      // fire the virtual 'buffer full' irq
      if (observer)
	observer->notify();
    }
}

// return number of tracebuffer entries
IMPLEMENT inline
Mword
Jdb_tbuf::entries()
{
  return _entries;
}

IMPLEMENT inline
int
Jdb_tbuf::event_valid(Mword idx)
{
  return (idx < _entries);
}

// event with idx == 0 is the last event queued in
// event with idx == 1 is the event before
IMPLEMENT
Tb_entry*
Jdb_tbuf::lookup(Mword idx)
{
  if (!event_valid(idx))
    return 0;

  Tb_entry_fit64 *e = tbuf_act - idx - 1;
  
  if (e < tbuf)
    e += Config::tbuf_entries;
  
  return static_cast<Tb_entry*>(e);
}

IMPLEMENT
int
Jdb_tbuf::event(Mword idx, Mword *number, 
		Unsigned64 *tsc, Unsigned32 *pmc1, Unsigned32 *pmc2)
{
  Tb_entry *e = lookup(idx);

  if (!e)
    return false;

  *number = e->number();
  if (tsc)
    *tsc = e->tsc();
  if (pmc1)
    *pmc1 = e->pmc1();
  if (pmc2)
    *pmc2 = e->pmc2();
  return true;
}

IMPLEMENT
Mword
Jdb_tbuf::ipc_pair_event(Mword idx, Unsigned8 *type)
{
  Tb_entry *e = lookup(idx);

  if (e)
    {
      if (e->type() == TBUF_IPC_RES)
	{
	  *type = EVENT;
	  return static_cast<Tb_entry_ipc_res*>(e)->pair_event();
	}
      if (e->type() == TBUF_IPC)
	{
	  Tb_entry_ipc     *e0 = static_cast<Tb_entry_ipc*>(e);
	  Tb_entry_ipc_res *e1;

	  // start at e and go until future until current event
	  while (   (idx > 0) 
		 && (0 != (e1 = static_cast<Tb_entry_ipc_res*>(lookup(--idx)))))
	    {
	      if (   (e1->type()       == TBUF_IPC_RES)
		  && (e1->pair_event() == e0->number()))
		{
		  *type = RESULT;
		  return e1->number();
		}
	    }
	}
    }
  return (Mword)-1;
}

// search the paired event to an pagefault or pagefault result
IMPLEMENT
Mword
Jdb_tbuf::pf_pair_event(Mword idx, Unsigned8 *type)
{
  Tb_entry *e = lookup(idx);

  if (e)
    {
      if (e->type() == TBUF_PF_RES)
	{
	  // we have a pf result event and we search the paired pf event
	  Tb_entry_pf_res *e0 = static_cast<Tb_entry_pf_res*>(e);
	  Tb_entry_pf     *e1;

	  // start at e and go into past until oldest event
	  while (0 != (e1 = static_cast<Tb_entry_pf*>(lookup(++idx))))
	    {
	      if (   (e1->type() == TBUF_PF)
		  && (e1->tid()  == e0->tid())
		  && (e1->eip()  == e0->eip())
		  && (e1->pfa()  == e0->pfa()))
		{
		  *type = EVENT;
		  return e1->number();
		}
	    }
	}
      else if (e->type() == TBUF_PF)
	{
	  // we have a pf event and we search the paired pf result event
	  Tb_entry_pf     *e0 = static_cast<Tb_entry_pf*>(e);
	  Tb_entry_pf_res *e1;

	  // start at e and go until future until current event
	  while (   (idx > 0)
	         && (0 != (e1 = static_cast<Tb_entry_pf_res*>(lookup(--idx)))))
	    {
	      if (   (e1->type() == TBUF_PF_RES)
		  && (e1->tid()  == e0->tid())
		  && (e1->eip()  == e0->eip())
		  && (e1->pfa()  == e0->pfa()))
		{
		  *type = RESULT;
		  return e1->number();
		}
	    }
	}
    }
  return (Mword)-1;
}

IMPLEMENT
int
Jdb_tbuf::diff_tsc(Mword idx, Signed64 *delta)
{
  Tb_entry *e      = lookup(idx);
  Tb_entry *e_prev = lookup(idx+1);
  
  if (!e || !e_prev)
    return false;

  *delta = e->tsc() - e_prev->tsc();
  return true;
}

IMPLEMENT
int
Jdb_tbuf::diff_pmc(Mword idx, Mword nr, Signed32 *delta)
{
  Tb_entry *e      = lookup(idx);
  Tb_entry *e_prev = lookup(idx+1);
  
  if (!e || !e_prev)
    return false;

  switch (nr)
    {
    case 0: *delta = e->pmc1() - e_prev->pmc1(); break;
    case 1: *delta = e->pmc2() - e_prev->pmc2(); break;
    }

  return true;
}

PUBLIC
explicit
Log_event::Log_event(const char *name, int type,
		     int count, Log_patch *lp, ...)
{
  va_list args;
  Log_patch **p = &patch;

  va_start(args, lp);
  *p = lp;
  for (int i=0; i<count-1; i++)
    {
      p  = &(*p)->next;
      *p = va_arg(args, Log_patch*);
    }
  va_end(args);

  event_name = name;
  event_type = type;
}

// enable all patch entries of log event
PUBLIC
void
Log_event::enable(int enable)
{
  Log_patch *p = patch;

  while (p)
    {
      p->enable(enable);
      p = p->next;
    }
}

// log event enabled if first entry is enabled
PUBLIC inline
int
Log_event::enabled(void) const
{
  return patch->enabled();
}

// get name of log event
PUBLIC inline
const char*
Log_event::get_name(void) const
{
  return event_name;
}

// get type of log event
PUBLIC inline
int
Log_event::get_type(void) const
{
  return event_type;
}

PUBLIC
explicit
Log_patch::Log_patch(void *code)
  : next(0), activator((Unsigned8*)code+1)
{
}

PUBLIC
void
Log_patch::enable(int enable)
{
  *activator = enable;

  // checksum has changed
  Boot_info::reset_checksum_ro();
}

PUBLIC inline
int
Log_patch::enabled(void)
{
  return *activator;
}

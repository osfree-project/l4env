INTERFACE:

#include "l4_types.h"

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

class Jdb_tbuf_events
{
public:
  static Log_event *const log_events[];
};


IMPLEMENTATION:

#include <cstdarg>

#include "boot_info.h"
#include "config.h"
#include "jdb_ktrace.h"

// The following headers are only included here to reflect the module
// dependencies.
// 
// patch_log_sched_invalidate
// patch_log_sched_save
// patch_log_sched_load
// patch_log_context_switch
#include "context.h"
// patch_log_irq
#include "dirq.h"
// patch_log_pf_res
// patch_log_trap
// patch_log_thread_ex_regs
// patch_log_timer_irq
// patch_log_shortcut_failed1
// patch_log_shortcut_failed2
// patch_log_shortcut_succeeded
#include "thread.h"
// patch_show_log_pf_res
#include "jdb_trace.h"
// patch_show_log_context_switch
// patch_show_log_shortcut
#include "jdb_trace_set.h"
// patch_log_send_preemption
#include "preemption.h"


#ifdef CONFIG_JDB_LOGGING

#define DECLARE_PATCH(VAR, NAME)		\
  extern "C" char patch_##NAME;			\
  static Log_patch VAR(&patch_##NAME)

// logging of context switch not implemented in Assembler shortcut but we
// can switch to C shortcut
DECLARE_PATCH (lp1, log_context_switch);
DECLARE_PATCH (lp2, show_log_context_switch);
static Log_event le1("context switch",
		      Log_event_context_switch, 2, &lp1, &lp2);

// logging of shortcut succeeded/failed not implemented in Assembler shortcut
// but we can switch to C shortcut
DECLARE_PATCH (lp3, log_shortcut_failed_1);
DECLARE_PATCH (lp4, log_shortcut_failed_2);
DECLARE_PATCH (lp5, log_shortcut_succeeded);
DECLARE_PATCH (lp6, show_log_shortcut);
static Log_event le2("ipc shortcut",
		      Log_event_ipc_shortcut, 4, &lp3, &lp4, &lp5, &lp6);

DECLARE_PATCH (lp7, log_irq);
static Log_event le3("irq raised",
		      Log_event_irq_raised, 1, &lp7);

DECLARE_PATCH (lp8, log_timer_irq);
static Log_event le4("timer irq raised",
		      Log_event_timer_irq, 1, &lp8);

DECLARE_PATCH (lp9, log_thread_ex_regs);
#ifdef CONFIG_SYSCALL_ITER
DECLARE_PATCH (lp10, log_thread_ex_regs_failed);
static Log_event le5("thread_ex_regs",
		      Log_event_thread_ex_regs, 2, &lp9, &lp10);
#else
static Log_event le5("thread_ex_regs",
		      Log_event_thread_ex_regs, 1, &lp9);
#endif

#ifdef CONFIG_PF_UX
DECLARE_PATCH (lp11, log_trap);
static Log_event le6("trap raised",
		      Log_event_trap, 1, &lp11);
#else
DECLARE_PATCH (lp11, log_trap);
DECLARE_PATCH (lp12, log_trap_n);
static Log_event le6("trap raised",
		      Log_event_trap, 2, &lp11, &lp12);
#endif

DECLARE_PATCH (lp13, log_pf_res);
DECLARE_PATCH (lp14, show_log_pf_res);
static Log_event le7("pagefault result",
		      Log_event_pf_res, 2, &lp13, &lp14);

DECLARE_PATCH (lp15, log_sched_load);
DECLARE_PATCH (lp16, log_sched_save);
DECLARE_PATCH (lp17, log_sched_invalidate);
static Log_event le8("scheduling event",
                      Log_event_sched, 3, &lp15, &lp16, &lp17);

DECLARE_PATCH (lp18, log_preemption);
static Log_event le9("preemption sent",
		      Log_event_preemption, 1, &lp18);

#ifdef	CONFIG_LOCAL_IPC
DECLARE_PATCH (lp19, log_lipc_rollback);
DECLARE_PATCH (lp20, log_lipc_rollforward);
DECLARE_PATCH (lp21, log_lipc_copy);
DECLARE_PATCH (lp22, log_lipc_setup_iret_stack);
static Log_event le10("lipc fixupcode",
                      Log_event_lipc, 4, &lp19, &lp20, &lp21, &lp22);
#endif

DECLARE_PATCH (lp23, log_task_new);
static Log_event le11("task_new",
		      Log_event_task_new, 1, &lp23);

Log_event * const Jdb_tbuf_events::log_events[Log_event_max] =
{ 
  &le1,	 // context switch
  &le2,  // ipc shortcut
  &le3,  // irq raised
  &le4,  // timer irq raised
  &le5,  // thread_ex_regs
  &le6,  // trap raised
  &le7,  // pagefault result
  &le8,	 // scheduling event
  &le9,  // preemption sent
#ifdef	CONFIG_LOCAL_IPC
  &le10, // lipc fixup code
#else
  0,
#endif
  &le11, // task new
  0,     // terminate list
};

#endif


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
  : next(0), activator((Unsigned8*)code)
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

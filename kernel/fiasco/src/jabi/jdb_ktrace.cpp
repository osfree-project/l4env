INTERFACE:

#include "types.h"

enum {
  Log_event_context_switch   = 0,
  Log_event_ipc_shortcut     = 1,
  Log_event_irq_raised       = 2,
  Log_event_timer_irq        = 3,
  Log_event_thread_ex_regs   = 4,
  Log_event_trap             = 5,
  Log_event_pf_res           = 6,
  Log_event_sched            = 7,
  Log_event_preemption       = 8,
  Log_event_lipc             = 9,
  Log_event_task_new         = 10,
  Log_event_max              = 16,
};

enum {
  Kern_cnt_context_switch    = 0,
  Kern_cnt_addr_space_switch = 1,
  Kern_cnt_shortcut_failed   = 2,
  Kern_cnt_shortcut_success  = 3,
  Kern_cnt_irq               = 4,
  Kern_cnt_ipc_long          = 5,
  Kern_cnt_page_fault        = 6,
  Kern_cnt_io_fault          = 7,
  Kern_cnt_task_create       = 8,
  Kern_cnt_schedule          = 9,
  Kern_cnt_iobmap_tlb_flush  = 10,
  Kern_cnt_max               = 11,
};


struct Tracebuffer_status
{
  Address    tracebuffer0;
  Address    size0;
  Unsigned64 version0;
  Address    tracebuffer1;
  Address    size1;
  Unsigned64 version1;
  Address    current;
  Unsigned32 logevents[Log_event_max];

  Unsigned32 scaler_tsc_to_ns;
  Unsigned32 scaler_tsc_to_us;
  Unsigned32 scaler_ns_to_tsc;

  Unsigned32 kerncnts[Kern_cnt_max];
};

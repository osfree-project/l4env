/* IA32 */

INTERFACE:

#include "types.h"

enum {
  LOG_EVENT_MAX_EVENTS = 16,
};

struct Tracebuffer_status
{
  Unsigned32 tracebuffer0;
  Unsigned32 size0;
  Unsigned32 version0;
  Unsigned32 tracebuffer1;
  Unsigned32 size1;
  Unsigned32 version1;
  Unsigned32 logevents[LOG_EVENT_MAX_EVENTS]; 
  Unsigned32 scaler_tsc_to_ns;
  Unsigned32 scaler_tsc_to_us;
  Unsigned32 scaler_ns_to_tsc;
};

enum {
  LOG_EVENT_CONTEXT_SWITCH = 0,
  LOG_EVENT_IPC_SHORTCUT   = 1,
  LOG_EVENT_IRQ_RAISED     = 2,
  LOG_EVENT_TIMER_IRQ      = 3,
  LOG_EVENT_THREAD_EX_REGS = 4,
  LOG_EVENT_TRAP           = 5,
  LOG_EVENT_PF_RES         = 6,
  LOG_EVENT_SCHED          = 7
};

IMPLEMENTATION [arch]:
//-

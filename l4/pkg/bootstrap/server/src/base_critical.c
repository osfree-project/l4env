
#include <l4/util/irq.h>
#include "base_critical.h"

static unsigned flags;
static unsigned entry_count;

void
base_critical_enter(void)
{
  if (entry_count == 0)
    {
      l4util_flags_save(&flags);
      l4util_cli();
    }
  entry_count++;
}

void
base_critical_leave(void)
{
  entry_count--;
  if (entry_count == 0)
    l4util_flags_restore(&flags);
}


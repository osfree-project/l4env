INTERFACE:

#include "types.h"

class Watchdog
{
public:
  static void init();
};


IMPLEMENTATION[disabled]:

#include "initcalls.h"
#include <panic.h>

PUBLIC static inline
void
Watchdog::touch()
{
}

// user enables Watchdog
PUBLIC static inline
void
Watchdog::user_enable()
{
}

// user disables Watchdog
PUBLIC static inline
void
Watchdog::user_disable()
{
}

// user takes over control of Watchdog
PUBLIC static inline
void
Watchdog::user_takeover_control()
{
}

// user gives back control of Watchdog
PUBLIC static inline
void
Watchdog::user_giveback_control()
{
}

PUBLIC static inline
void
Watchdog::disable()
{
}

PUBLIC static inline
void
Watchdog::enable()
{
}

IMPLEMENT FIASCO_INIT
void
Watchdog::init()
{
}

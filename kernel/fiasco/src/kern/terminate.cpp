INTERFACE:
#include "initcalls.h"

IMPLEMENTATION:

#include <cstdio>
#include <cstdlib>

#include "globals.h"
#include "helping_lock.h"

FIASCO_NORETURN void terminate( int exit_value )
{
  Helping_lock::threading_system_active = false;

  puts ("\nShutting down...");

  exit ( exit_value );
}

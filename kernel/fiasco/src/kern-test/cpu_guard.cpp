INTERFACE:


/**
 *  Locks the current cpu. test_and_set and clear from the node
 *  interface are used. 
 */
class cpu_guard_t 
{
};

extern cpu_guard_t cpu_guard;

IMPLEMENTATION:

#include "node.h"


PUBLIC bool
cpu_guard_t::test_and_set()
{
  return node::lock_exec_cpu();
}

PUBLIC void
cpu_guard_t::clear()
{
  node::clear_exec_cpu();
}

cpu_guard_t cpu_guard;


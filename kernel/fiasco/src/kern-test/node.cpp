INTERFACE:

class context_t;

/** 
 *  Wrapper class for local cpu access.  Node provides an interface
 *  for local cpu access. It implements the cpu lock. Once the cpu
 *  lock is set access to the (now locked) cpu object is permitted.
 */
class node
{
};

IMPLEMENTATION:

#include "irq.h"
#include "cpu.h"


/**
 *  Disable interrupts and return prior state.
 *  @return true if the cpu lock was already set, false otherwise.
 */
PUBLIC static bool
node::lock_exec_cpu()
{
  bool iflag = eflags_save() & EFL_IF;
  cli();
  return !iflag;
}

/** 
 * Report the current state.
 * @return true if cpu lock is set
 * (i.e. interrupts are disabled) , false otherwise
 */
PUBLIC static bool
node::test_exec_cpu()
{
  return ! (eflags_save() & EFL_IF);
}

/**
 *  Clear cpu lock. Interrupts must be disabled at that time otherwise
 *  a consistency problem is present.
 */
PUBLIC static void
node::clear_exec_cpu()
{
  assert(! (eflags_save() & EFL_IF));
  sti();
}


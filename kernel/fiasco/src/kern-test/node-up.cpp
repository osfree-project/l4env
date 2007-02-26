INTERFACE:

#include "cpu.h"

EXTENSION class node 
{
  static cpu_t _cpu;
};

IMPLEMENTATION[up]:

#include "globals.h"
#include "thread.h"
#include "context.h"

cpu_t node::_cpu;

PUBLIC static cpu_t*
node::curr_cpu()
{
  assert(!(get_eflags() & EFL_IF));
  return &_cpu;
}

PUBLIC static unsigned
node::id()
{
  assert(!(get_eflags() & EFL_IF));
  return 0;
}

PUBLIC static unsigned
node::id_ll()
{
  assert(!(get_eflags() & EFL_IF));
  return 0;
}

PUBLIC static cpu_t*
node::get_cpu(unsigned int id)
{
  assert(id == 0);
  return &_cpu;
}

PUBLIC static unsigned
node::cpu_to_id(cpu_t *cpu)
{
  return 0;
}

PUBLIC static int
node::init_node()
{
  return 0;
}

PUBLIC static context_t *
node::get_idle_context()
{
  assert(test_exec_cpu());

  l4_threadid_t idle_id = config::kernel_id;

  idle_id.id.lthread = 0;

  return threadid_t(&idle_id).lookup();
}


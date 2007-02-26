IMPLEMENTATION[syscall-v4]:

#include "l4_types.h"
#include "entry_frame.h"
#include "thread.h"
#include "thread_util.h"
#include "task.h"

void (*syscall_table[])() = 
{ 
  sys_space_control_wrapper,
  sys_thread_control_wrapper,
  sys_processor_control_wrapper,
  sys_memory_control_wrapper,
  sys_ipc_wrapper,
  sys_lipc_wrapper,
  sys_fpage_unmap_wrapper,
  sys_thread_ex_regs_wrapper,
  sys_clock_wrapper,
  sys_thread_switch_wrapper,
  sys_thread_schedule_wrapper
};

extern "C" void sys_space_control_wrapper()	{ assert(0); }
extern "C" void sys_thread_control_wrapper()	{ assert(0); }
extern "C" void sys_processor_control_wrapper()	{ assert(0); }
extern "C" void sys_memory_control_wrapper()	{ assert(0); }
//extern "C" void sys_ipc_wrapper()		{ assert(0); }
extern "C" void sys_lipc_wrapper()		{ assert(0); }
extern "C" void sys_thread_ex_regs_wrapper()	{ assert(0); }
extern "C" void sys_clock_wrapper()		{ assert(0); }

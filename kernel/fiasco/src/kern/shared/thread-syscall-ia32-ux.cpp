INTERFACE:

extern void (*syscall_table[])();

IMPLEMENTATION:

void (*syscall_table[])() = 
{ 
  sys_ipc_wrapper,
  sys_id_nearest_wrapper,
  sys_fpage_unmap_wrapper,
  sys_thread_switch_wrapper,
  sys_thread_schedule_wrapper,
  sys_thread_ex_regs_wrapper,
  sys_task_new_wrapper,
#ifdef CONFIG_PL0_HACK
  sys_priv_control_wrapper,
#endif
};


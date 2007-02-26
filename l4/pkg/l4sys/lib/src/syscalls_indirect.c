
#include <l4/sys/syscalls.h>
#include <l4/sys/kernel.h>

typedef void (*scp_t)(void);

static void __L4sys_lib_init(void) __attribute__ ((unused));

#define DEFINE_SYSCALL(syscall)               \
void __L4sys_##syscall##_init(void);          \
asm ( ".text                    \n"           \
      "__L4sys_"#syscall"_init: \n"           \
      " pusha                   \n"           \
      " call __L4sys_lib_init   \n"           \
      " popa                    \n"           \
      " jmp *__L4_"#syscall"    \n"           \
      " .previous               \n" );        \
scp_t __L4_##syscall                          \
  = __L4sys_##syscall##_init;


DEFINE_SYSCALL(ipc)
DEFINE_SYSCALL(id_nearest)
DEFINE_SYSCALL(fpage_unmap)
DEFINE_SYSCALL(thread_switch)
DEFINE_SYSCALL(thread_schedule)
DEFINE_SYSCALL(lthread_ex_regs)
DEFINE_SYSCALL(task_new)

static void __L4sys_lib_init(void)
{
  l4_kernel_info_t *ki = (l4_kernel_info_t*)l4_kernel_interface();
  switch(ki->reserved[3]) 
    {
    case 1:
      __L4_ipc = (scp_t)((char*)ki + ki->sys_ipc);
      __L4_id_nearest = (scp_t)((char*)ki + ki->sys_id_nearest);
      __L4_fpage_unmap = (scp_t)((char*)ki + ki->sys_fpage_unmap);
      __L4_thread_switch = (scp_t)((char*)ki + ki->sys_thread_switch);
      __L4_thread_schedule = (scp_t)((char*)ki + ki->sys_thread_schedule);
      __L4_lthread_ex_regs = (scp_t)((char*)ki + ki->sys_lthread_ex_regs);
      __L4_task_new = (scp_t)((char*)ki + ki->sys_task_new);
      break;
    case 2:
      __L4_ipc = (scp_t)ki->sys_ipc;
      __L4_id_nearest = (scp_t)ki->sys_id_nearest;
      __L4_fpage_unmap = (scp_t)ki->sys_fpage_unmap;
      __L4_thread_switch = (scp_t)ki->sys_thread_switch;
      __L4_thread_schedule = (scp_t)ki->sys_thread_schedule;
      __L4_lthread_ex_regs = (scp_t)ki->sys_lthread_ex_regs;
      __L4_task_new = (scp_t)ki->sys_task_new;
      break;
    default:
      __L4_ipc = (scp_t)0;
      __L4_id_nearest = (scp_t)0;
      __L4_fpage_unmap = (scp_t)0;
      __L4_thread_switch = (scp_t)0;
      __L4_thread_schedule = (scp_t)0;
      __L4_lthread_ex_regs = (scp_t)0;
      __L4_task_new = (scp_t)0;
      break;
    }
     
}

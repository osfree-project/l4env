
#include <l4/sys/syscalls.h>
#include <l4/sys/kernel.h>

typedef void (*scp_t)(void);

L4_STICKY(static void __l4sys_lib_init(void));

#ifdef ARCH_x86
#ifdef __PIC__
#define DEFINE_SYSCALL(syscall)                 \
void __l4sys_##syscall##_init(void);            \
asm ( ".text                      \n"           \
      "__l4sys_"#syscall"_init:   \n"           \
      " pusha                     \n"           \
      " call __l4sys_lib_init     \n"           \
      " popa                      \n"           \
      " jmp  1f                   \n"           \
      " .data                     \n"           \
      " 1:                        \n"           \
      " jmp *__l4sys_"#syscall"   \n"           \
      " .previous                 \n"           \
      " .hidden __l4sys_"#syscall"\n");         \
scp_t __l4sys_##syscall = __l4sys_##syscall##_init;
#else
#define DEFINE_SYSCALL(syscall)                 \
void __l4sys_##syscall##_init(void);            \
asm ( ".text                      \n"           \
      "__l4sys_"#syscall"_init:   \n"           \
      " pusha                     \n"           \
      " call __l4sys_lib_init     \n"           \
      " popa                      \n"           \
      " jmp *__l4sys_"#syscall"   \n"           \
      " .previous                 \n"           \
      " .hidden __l4sys_"#syscall"\n");         \
scp_t __l4sys_##syscall  = __l4sys_##syscall##_init;
#endif
#endif

#ifdef ARCH_amd64
#ifdef __PIC__
#define DEFINE_SYSCALL(syscall)                 \
void __l4sys_##syscall##_init(void);            \
asm ( ".text                    \n"             \
      "__l4sys_"#syscall"_init: \n"             \
      " push %rax		\n"		\
      " push %rbx		\n"		\
      " push %rcx		\n"		\
      " push %rdx		\n"		\
      " push %rdi		\n"		\
      " push %rsi		\n"		\
      " push %rbp		\n"		\
      " push %r8		\n"		\
      " push %r9		\n"		\
      " push %r10		\n"		\
      " push %r11		\n"		\
      " push %r12		\n"		\
      " push %r13		\n"		\
      " push %r14		\n"		\
      " push %r15		\n"		\
      " call __l4sys_lib_init   \n"             \
      " pop %r15		\n"		\
      " pop %r14		\n"		\
      " pop %r13		\n"		\
      " pop %r12		\n"		\
      " pop %r11		\n"		\
      " pop %r10		\n"		\
      " pop %r9			\n"		\
      " pop %r8			\n"		\
      " pop %rbp		\n"		\
      " pop %rsi		\n"		\
      " pop %rdi		\n"		\
      " pop %rdx                \n"		\
      " pop %rcx                \n"		\
      " pop %rbx                \n"		\
      " pop %rax                \n"		\
      " call 1f                 \n"             \
      " .data                   \n"             \
      " 1:                      \n"             \
      " jmp *__l4sys_"#syscall" \n"             \
      " .previous               \n" );          \
scp_t __l4sys_##syscall = __l4sys_##syscall##_init;
#else
#define DEFINE_SYSCALL(syscall)                 \
void __l4sys_##syscall##_init(void);            \
asm ( ".text                    \n"             \
      "__l4sys_"#syscall"_init: \n"             \
      " push %rax		\n"		\
      " push %rbx		\n"		\
      " push %rcx		\n"		\
      " push %rdx		\n"		\
      " push %rdi		\n"		\
      " push %rsi		\n"		\
      " push %rbp		\n"		\
      " push %r8		\n"		\
      " push %r9		\n"		\
      " push %r10		\n"		\
      " push %r11		\n"		\
      " push %r12		\n"		\
      " push %r13		\n"		\
      " push %r14		\n"		\
      " push %r15		\n"		\
      " call __l4sys_lib_init   \n"             \
      " pop %r15		\n"		\
      " pop %r14		\n"		\
      " pop %r13		\n"		\
      " pop %r12		\n"		\
      " pop %r11		\n"		\
      " pop %r10		\n"		\
      " pop %r9			\n"		\
      " pop %r8			\n"		\
      " pop %rbp		\n"		\
      " pop %rsi		\n"		\
      " pop %rdi		\n"		\
      " pop %rdx                \n"		\
      " pop %rcx                \n"		\
      " pop %rbx                \n"		\
      " pop %rax                \n"		\
      " jmp *__l4sys_"#syscall" \n"             \
      " .previous               \n" );          \
scp_t __l4sys_##syscall = __l4sys_##syscall##_init;
#endif
#endif

DEFINE_SYSCALL(ipc)
DEFINE_SYSCALL(id_nearest)
DEFINE_SYSCALL(fpage_unmap)
DEFINE_SYSCALL(thread_switch)
DEFINE_SYSCALL(thread_schedule)
DEFINE_SYSCALL(lthread_ex_regs)
DEFINE_SYSCALL(task_new)
DEFINE_SYSCALL(privctrl)

static void
__l4sys_lib_init(void)
{
  l4_kernel_info_t *ki = (l4_kernel_info_t*)l4_kernel_interface();
  switch(ki->kip_sys_calls) 
    {
    case 1:
      __l4sys_ipc             = (scp_t)((char*)ki + ki->sys_ipc);
      __l4sys_id_nearest      = (scp_t)((char*)ki + ki->sys_id_nearest);
      __l4sys_fpage_unmap     = (scp_t)((char*)ki + ki->sys_fpage_unmap);
      __l4sys_thread_switch   = (scp_t)((char*)ki + ki->sys_thread_switch);
      __l4sys_thread_schedule = (scp_t)((char*)ki + ki->sys_thread_schedule);
      __l4sys_lthread_ex_regs = (scp_t)((char*)ki + ki->sys_lthread_ex_regs);
      __l4sys_task_new        = (scp_t)((char*)ki + ki->sys_task_new);
      __l4sys_privctrl        = (scp_t)((char*)ki + ki->sys_privctrl);
      break;
    case 2:
      __l4sys_ipc             = (scp_t)(l4_addr_t)ki->sys_ipc;
      __l4sys_id_nearest      = (scp_t)(l4_addr_t)ki->sys_id_nearest;
      __l4sys_fpage_unmap     = (scp_t)(l4_addr_t)ki->sys_fpage_unmap;
      __l4sys_thread_switch   = (scp_t)(l4_addr_t)ki->sys_thread_switch;
      __l4sys_thread_schedule = (scp_t)(l4_addr_t)ki->sys_thread_schedule;
      __l4sys_lthread_ex_regs = (scp_t)(l4_addr_t)ki->sys_lthread_ex_regs;
      __l4sys_task_new        = (scp_t)(l4_addr_t)ki->sys_task_new;
      __l4sys_privctrl        = (scp_t)(l4_addr_t)ki->sys_privctrl;
      break;
    default:
      __l4sys_ipc             = (scp_t)0;
      __l4sys_id_nearest      = (scp_t)0;
      __l4sys_fpage_unmap     = (scp_t)0;
      __l4sys_thread_switch   = (scp_t)0;
      __l4sys_thread_schedule = (scp_t)0;
      __l4sys_lthread_ex_regs = (scp_t)0;
      __l4sys_task_new        = (scp_t)0;
      __l4sys_privctrl        = (scp_t)0;
      break;
    }
}

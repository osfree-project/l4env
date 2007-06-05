/* $Id: */
/**
 * \file    l4sys/include/ARCH-x86/syscall-invoke.h
 * \brief   L4 System Call Invoking in Assembler
 * \ingroup api_calls
 *
 * These code fragments can be shared between v2 and x0, so they're just in
 * the x86 directory. When adding another L4 API we'll need a solution here.
 * This file can also be used in asm-files, so don't include C statements.
 */
#ifndef __L4_SYSCALL_INVOKE_H__
#define __L4_SYSCALL_INVOKE_H__

typedef struct __attribute__((packed))
{
  unsigned char opcode;
  unsigned long address;
  unsigned char nop[3];
} l4_syscall_patch_t;

void l4sys_fixup_abs_syscalls(void);

#define L4_ABS_syscall_page              0xeacff000
#define L4_ABS_ipc                       (0x000)
#define L4_ABS_id_nearest                (0x100)
#define L4_ABS_fpage_unmap               (0x200)
#define L4_ABS_thread_switch             (0x300)
#define L4_ABS_thread_schedule           (0x400)
#define L4_ABS_lthread_ex_regs           (0x500)
#define L4_ABS_task_new                  (0x600)
#define L4_ABS_privctrl                  (0x700)
#define L4_ABS_ulock                     (0x800)

#ifndef CONFIG_L4_CALL_SYSCALLS

# define L4_SYSCALL_id_nearest            "int $0x31 \n\t"
# define L4_SYSCALL_fpage_unmap           "int $0x32 \n\t"
# define L4_SYSCALL_thread_switch         "int $0x33 \n\t"
# define L4_SYSCALL_thread_schedule       "int $0x34 \n\t"
# define L4_SYSCALL_lthread_ex_regs       "int $0x35 \n\t"
# define L4_SYSCALL_task_new              "int $0x36 \n\t"
# define L4_SYSCALL_privctrl              "int $0x37 \n\t"
# define L4_SYSCALL_ulock                 "int $0x39 \n\t"
# define L4_SYSCALL(name)                 L4_SYSCALL_ ## name

#else

# ifdef CONFIG_L4_ABS_SYSCALLS

#  ifdef __PIC__
    asm (".globl l4sys_fixup_abs_syscalls");
#   define L4_SYSCALL(s)           "call __l4sys_abs_"#s"_fixup \n\t"
#  else
#   define L4_SYSCALL(s)           "call __l4sys_"#s"_direct    \n\t"
#  endif

# else /* CONFIG_L4_ABS_SYSCALLS */

#  ifdef __PIC__
#   error -fPIC with L4_REL_SYSCALLS not possible!
#  else
#   define L4_SYSCALL(s)           "call *__l4sys_"#s"          \n\t"
#  endif

# endif /* !CONFIG_L4_ABS_SYSCALLS */

#endif /* CONFIG_L4_CALL_SYSCALLS */

#endif

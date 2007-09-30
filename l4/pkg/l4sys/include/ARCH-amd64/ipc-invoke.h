/**
 * \file    l4sys/include/ARCH-amd64/ipc-invoke.h
 * \brief   L4 IPC System Call Invoking in Assembler
 * \ingroup api_calls
 *
 * This file can also be used in asm-files, so don't include C statements.
 */
#ifndef __L4_IPC_INVOKE_H__
#define __L4_IPC_INVOKE_H__

/**
 * Some words about the sysenter entry frame: Since the sysenter instruction
 * automatically reloads the instruction pointer (eip) and the stack pointer
 * (esp) after kernel entry, we have to save both registers preliminary to
 * that instruction. We use ecx to store the user-level esp and save eip onto
 * the stack. The ecx register contains the IPC timeout and has to be saved
 * onto the stack, too. The ebp register is saved for compatibility reasons
 * with the Hazelnut kernel. Both the esp and the ss register are also pushed
 * onto the stack to be able to return using the "lret" instruction from the
 * sysexit trampoline page if Small Address Spaces are enabled.
 */

#if 1
//#ifndef CONFIG_L4_CALL_SYSCALLS

# ifndef L4V2_IPC_SYSENTER

#  define IPC_SYSENTER       "int  $0x30              \n\t"
#  define IPC_SYSENTER_ASM    int  $0x30;

# else

#  ifdef __PIC__
# error no PIC support for AMD64
#  else
#   define IPC_SYSENTER            \
     "push   %%rcx           \n\t"  \
     "push   %%r11	     \n\t"  \
     "push   %%r15	     \n\t"  \
     "syscall                \n\t"  \
     "pop    %%r15	     \n\t"  \
     "pop    %%r11	     \n\t"  \
     "pop    %%rcx	     \n\t"  \
     "0:                     \n\t"
#   define IPC_SYSENTER_ASM	 \
     push    %rcx		; \
     push    %r11		; \
     push    %r15		; \
     syscall			; \
     pop    %r15		; \
     pop    %r11		; \
     pop    %rcx		; \
     0:
#  endif

# endif

#else

# ifdef CONFIG_L4_ABS_SYSCALLS
#  define IPC_SYSENTER           "call __l4sys_ipc_direct@plt \n\t"
#  define IPC_SYSENTER_ASM        call __l4sys_ipc_direct@plt   ;
# else
#  define IPC_SYSENTER           "call *__l4sys_ipc           \n\t"
#  define IPC_SYSENTER_ASM        call *__l4sys_ipc            ;
# endif

#endif

#endif /* ! __L4_IPC_INVOKE_H__ */

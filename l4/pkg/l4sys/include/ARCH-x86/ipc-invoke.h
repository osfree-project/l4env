/* $Id$ */
/**
 * \file    l4sys/include/ARCH-x86/ipc-invoke.h
 * \brief   L4 IPC System Call Invoking in Assembler
 * \ingroup api_calls
 *
 * These code fragments can be shared between v2 and x0, so they're just in
 * the x86 directory. When adding another L4 API we'll need a solution here.
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

#ifndef CONFIG_L4_CALL_SYSCALLS

# if !defined(L4V2_IPC_SYSENTER) && !defined(L4X0_IPC_SYSENTER)
#  define IPC_SYSENTER \
   "int    $0x30          \n\t"
#  define IPC_SYSENTER_ASM \
   int $0x30;

# else
#  ifdef __PIC__
#   define IPC_SYSENTER \
     "push   %%ecx           \n\t" \
     "push   %%ebp           \n\t" \
     "push   $0x1b           \n\t" \
     "call   0f              \n\t" \
     "0:                     \n\t" \
     "addl   $(1f-0b),(%%esp)\n\t" \
     "mov    %%esp,%%ecx     \n\t" \
     "sysenter               \n\t" \
     "mov    %%ebp,%%edx     \n\t" \
     "1:                     \n\t"
#   define IPC_SYSENTER_ASM	\
     push    %ecx		; \
     push    %ebp		; \
     push    $0x1b		; \
     call    0f			; \
     0:				; \
     addl    $(1f-0b),(%esp)	; \
     mov     %esp,%ecx		; \
     sysenter			; \
     mov     %ebp,%edx		; \
     1:
#  else
#   define IPC_SYSENTER \
     "push   %%ecx           \n\t"  \
     "push   %%ebp           \n\t"  \
     "push   $0x1b           \n\t"  \
     "push   $0f             \n\t"  \
     "mov    %%esp,%%ecx     \n\t"  \
     "sysenter               \n\t"  \
     "mov    %%ebp,%%edx     \n\t"  \
     "0:                     \n\t"
#   define IPC_SYSENTER_ASM	\
     push    %ecx		; \
     push    %ebp		; \
     push    $0x1b		; \
     push    $0f		; \
     mov     %esp,%ecx		; \
     sysenter			; \
     mov     %ebp,%edx		; \
     0:
#  endif
# endif

#else

# ifdef CONFIG_L4_ABS_SYSCALLS
#  define IPC_SYSENTER \
  "call __L4_ipc_direct  \n\t"
#  define IPC_SYSENTER_ASM \
  call __L4_ipc_direct;
# else
#  define IPC_SYSENTER \
  "call *__L4_ipc        \n\t"
#  define IPC_SYSENTER_ASM \
  call *__L4_ipc;
# endif

#endif

#endif /* ! __L4_IPC_INVOKE_H__ */

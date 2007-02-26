#ifndef __ASM_OFFSETS_H__
#define __ASM_OFFSETS_H__
/*
 * DO NOT MODIFY.
 *
 * This file was generated by arch/i386/Makefile
 *
 */

#define SIGCONTEXT_eax 44 /* offsetof (struct sigcontext, eax) */
#define SIGCONTEXT_ebx 32 /* offsetof (struct sigcontext, ebx) */
#define SIGCONTEXT_ecx 40 /* offsetof (struct sigcontext, ecx) */
#define SIGCONTEXT_edx 36 /* offsetof (struct sigcontext, edx) */
#define SIGCONTEXT_esi 20 /* offsetof (struct sigcontext, esi) */
#define SIGCONTEXT_edi 16 /* offsetof (struct sigcontext, edi) */
#define SIGCONTEXT_ebp 24 /* offsetof (struct sigcontext, ebp) */
#define SIGCONTEXT_esp 28 /* offsetof (struct sigcontext, esp) */
#define SIGCONTEXT_eip 56 /* offsetof (struct sigcontext, eip) */

#define RT_SIGFRAME_sigcontext 164 /* offsetof (struct rt_sigframe, uc.uc_mcontext) */
#define PAGE_SIZE_asm 4096 /* PAGE_SIZE */

#endif

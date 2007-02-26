/*!
 * \file   util/include/ARCH-x86/L4API-l4x0/setjmp.h
 * \brief  Inter-thread setjmp/longjmp
 *
 * \date   11/26/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __UTIL_INCLUDE_ARCH_X86_L4API_L4V2_SETJMP_H_
#define __UTIL_INCLUDE_ARCH_X86_L4API_L4V2_SETJMP_H_
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

typedef struct{
    l4_umword_t ebx;		/* 0 */
    l4_umword_t esi;		/* 4 */
    l4_umword_t edi;		/* 8 */
    l4_umword_t ebp;		/* 12 */
    l4_umword_t esp;		/* 16 */
    l4_umword_t eip;		/* 20 */
    l4_umword_t eip_caller;	/* 24 */
    l4_umword_t eflags;		/* 28 */
    l4_umword_t stack[40];
} l4_thread_jmp_buf_s;
typedef int l4_thread_jmp_buf[sizeof(l4_thread_jmp_buf_s)/sizeof(l4_umword_t)];

typedef union{
    l4_thread_jmp_buf_s s;
    l4_thread_jmp_buf raw;
} l4_thread_jmp_buf_u;

/*\brief inter-thread setjmp
 *
 * \param 	env	jump buffer
 * \retval 	0	returned directly
 * \retval	!0	returned from longjmp
 *
 * Use this function to prepare a longjmp from another thread for this thread.
 *
 * \see setjmp(3)
 */
extern int l4_thread_setjmp(l4_thread_jmp_buf env);

/*!\brief inter-thread longjmp
 *
 * \param	thread	thread to apply the longjmp to
 * \param	env	jump buffer
 * \param	val	0: setjmp returns with 1
 * \param	val	!0: return value of setjmp
 *
 * This function sets #thread to the location obtained by its former
 * l4_thread_setjump on #env.
 *
 * \see  longjmp(3)
 * \note In contrast to longjmp(3), this function returns.
 */
void l4_thread_longjmp(l4_threadid_t thread, l4_thread_jmp_buf env, int val);


#endif

#ifndef __DICE_DICE_L4_X2_TESTSUITE_H__
#define __DICE_DICE_L4_X2_TESTSUITE_H__

/*
 * Most of the code is taken from the L4Ka pingpong example.
 * Thus it is restricted to their copyright:
 *
 * Copyright (C) 2002-2004,  Karlsruhe University
 *                
 * File path:     bench/pingpong/pingpong.cc
 * Description:   Pingpong test application
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* This testsuite header only handles IA32 */
#if !defined(L4_ARCH_IA32) && !defined(ARCH_x86)
#error unsupported architecture for testsuite
#endif

//#include <config.h>
#include <l4/types.h>
#include <l4/kip.h>
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/kdebug.h>
#include <l4/message.h>

#define START_ADDR(func)	((L4_Word_t) func)


/*
 * Default arch dependent definitions and function stubs.
 */

#define HAVE_HANDLE_ARCH_PAGEFAULT
#define HAVE_READ_CYCLES
#define HAVE_ARCH_IPC

#define UTCB_ADDRESS    (0x00800000UL)
#define KIP_ADDRESS     (0x00C00000UL)

extern L4_Word_t __L4_syscalls_start;
extern L4_Word_t __L4_syscalls_end;
#if defined(__cpluplus)
extern "C"
#else
extern
#endif
void __L4_copy_syscalls_in (L4_Word_t dest);

extern char syscall_stubs[4096];

/* send startup ipc to task */
static void send_startup_ipc (L4_ThreadId_t tid, L4_Word_t ip, L4_Word_t sp)
{
//     printf ("%s: sending startup message to %08lx, (ip=%08lx, sp=%08lx)\n",
// 	__FUNCTION__, (long) tid.raw, (long) ip, (long) sp);
    L4_Msg_t msg;
    L4_MsgClear (&msg);
    L4_MsgAppendWord (&msg, ip);
    L4_MsgAppendWord (&msg, sp);
    L4_MsgLoad (&msg);
    L4_Send (tid);
}

static void pager (void) __attribute__((unused));
static void pager (void)
{
    L4_ThreadId_t tid;
    L4_MsgTag_t tag;
    L4_Msg_t msg;
    L4_Word_t faddr;
    L4_Word_t fip;
    L4_Word_t mapaddr;
    L4_Word_t page_bits;
    L4_Fpage_t page;
    
    L4_KernelInterfacePage_t * kip =
	(L4_KernelInterfacePage_t *) L4_GetKernelInterface ();

    tid = L4_Myself();
//     printf ("%s: I am %08lx\n", __FUNCTION__, tid.raw);

    /* Find smallest supported page size. There's better at least one
     * bit set. */
    for (page_bits = 0;  
         !((1 << page_bits) & L4_PageSizeMask(kip)); 
         page_bits++);

    for (;;)
    {
	tag = L4_Wait (&tid);

	for (;;)
	{
	    L4_MsgStore (tag, &msg);

// 	    printf ("%s: Pager got msg from %08lx (%08lx, %lx, %lx)\n", 
// 		__FUNCTION__, tid.raw, tag.raw, 
// 		L4_MsgWord (&msg, 0), L4_MsgWord (&msg, 1));

	    if (L4_Label (tag) == 0)
	    {
	        L4_Word_t ip = L4_MsgWord(&msg, 0);
		L4_Word_t sp = L4_MsgWord(&msg, 1);
		L4_ThreadId_t thread = { .raw = L4_MsgWord(&msg, 2) };

		send_startup_ipc ( thread, ip, sp );
		break;
	    }
	    
	    if (L4_UntypedWords (tag) != 2 || L4_TypedWords (tag) != 0 ||
		!L4_IpcSucceeded (tag))
	    {
		printf ("%s: malformed pagefault IPC from %08lx (tag=%08lx)\n",
		    __FUNCTION__, tid.raw, tag.raw);
		L4_KDB_Enter ("malformed pf");
		break;
	    }

	    faddr = L4_MsgWord (&msg, 0);
	    fip   = L4_MsgWord (&msg, 1);

	    // If pagefault is in the syscall stubs, create a copy of the
	    // original stubs and map in this copy.
	    if (faddr >= (L4_Word_t) &__L4_syscalls_start &&
		faddr <  (L4_Word_t) &__L4_syscalls_end)
	    {
		__L4_copy_syscalls_in ((L4_Word_t) syscall_stubs);
		mapaddr = (L4_Word_t) syscall_stubs;
      	    }
	    else
  		mapaddr = faddr;
	    
	    L4_MsgClear (&msg);
	    page = L4_FpageLog2 (mapaddr, page_bits);
	    page = L4_FpageAddRights (page, L4_FullyAccessible);
	    L4_MsgAppendMapItem (&msg, L4_MapItem (page, faddr));
	    L4_MsgLoad (&msg);
	    tag = L4_ReplyWait (tid, &tid);
	}
    }
}

#define UTCB(x) ((void*)(L4_Address(utcb_area) + (x) * utcb_size))
#define NOUTCB	((void*)-1)


#endif /* __DICE_DICE_L4_X2_TESTSUITE_H__ */

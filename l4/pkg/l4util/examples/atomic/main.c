/*!
 * \file   l4util/examples/atomic/main.c
 * \brief  Testcase for l4util_{add,sub}{8,16,32}()
 *
 * \date   04/02/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/util/atomic.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/thread.h>
#include <l4/sys/syscalls.h>
#include <stdio.h>

static volatile l4_uint32_t value32;
static volatile l4_uint16_t value16;
static volatile l4_uint8_t value8;

static l4_threadid_t thread1, thread2;
static char stack[64*1024];

static inline void send(l4_threadid_t t){
    l4_msgdope_t result;
    l4_ipc_send(t, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_NEVER, &result);
}
static inline void wait(l4_threadid_t t){
    l4_msgdope_t result;
    l4_umword_t d;
 
    l4_ipc_receive(t, L4_IPC_SHORT_MSG, &d, &d, L4_IPC_NEVER, &result);
}

static void func_1(void){
    int i;

    while(1){
	/* 32 bit */
	/* start non-atomic version w/o res */
	value32=0;
	send(thread2);
	for(i=0; i<100*1000*1000; i++){
	    value32 += i;
	}
	/* wait for func2 */
	wait(thread2);
	
	LOG("Nonatomic 32-bit version, value=%u (should be 0)", value32);
	
	/* start atomic version */
	value32=0;
	send(thread2);
	for(i=0; i<100*1000*1000; i++){
	    l4util_add32(&value32, i);
	}
	/* wait for func2 */
	wait(thread2);
	LOG("   Atomic 32-bit-version, value=%u (should be 0)", value32);
	printf("\n");

	/* 16 bit */
	/* start non-atomic version */
	value16=0;
	send(thread2);
	for(i=0; i<100*1000*1000; i++){
	    value16 += i;
	}
	/* wait for func2 */
	wait(thread2);
	
	LOG("Nonatomic 16-bit version, value=%u (should be 0)", value16);
	
	/* start atomic version */
	value16=0;
	send(thread2);
	for(i=0; i<100*1000*1000; i++){
	    l4util_add16(&value16, i);
	}
	/* wait for func2 */
	wait(thread2);
	LOG("   Atomic 16-bit-version, value=%u (should be 0)", value16);
	printf("\n");

	/* 8 bit */
	/* start non-atomic version */
	value8=0;
	send(thread2);
	for(i=0; i<100*1000*1000; i++){
	    value8 += i;
	}
	/* wait for func2 */
	wait(thread2);
	
	LOG("Nonatomic  8-bit version, value=%u (should be 0)", value8);
	
	/* start atomic version */
	value8=0;
	send(thread2);
	for(i=0; i<100*1000*1000; i++){
	    l4util_add8(&value8, i);
	}
	/* wait for func2 */
	wait(thread2);
	LOG("   Atomic  8-bit-version, value=%u (should be 0)", value8);
	printf("\n");
    }
}

static void func_2(void){
    int i;
	
    while(1){
	/* 32 bit */
	wait(thread1);
	/* non-atomic version */
	for(i=0; i<100*1000*1000; i++){
	    value32 -= i;
	}
	send(thread1);

	wait(thread1);
	/* atomic version */
	for(i=0; i<100*1000*1000; i++){
	    l4util_sub32(&value32, i);
	}
	send(thread1);

	/* 16 bit */
	wait(thread1);
	/* non-atomic version */
	for(i=0; i<100*1000*1000; i++){
	    value16 -= i;
	}
	send(thread1);

	wait(thread1);
	/* atomic version */
	for(i=0; i<100*1000*1000; i++){
	    l4util_sub16(&value16, i);
	}
	send(thread1);

	/* 8 bit */
	wait(thread1);
	/* non-atomic version */
	for(i=0; i<100*1000*1000; i++){
	    value8 -= i;
	}
	send(thread1);

	wait(thread1);
	/* atomic version */
	for(i=0; i<100*1000*1000; i++){
	    l4util_sub8(&value8, i);
	}
	send(thread1);
    }
}

int main(int argc, char**argv){
    thread1 = l4_myself();
    thread2 = l4util_create_thread(1, func_2, (int*)(stack+sizeof(stack)));
    func_1();
    return 0;
}

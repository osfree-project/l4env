#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

#ifdef FIASCO_UX
#define SCRATCH_MEM_SIZE	(16*1024*1024)
#else
#define SCRATCH_MEM_SIZE	(32*1024*1024)
#endif

#define STACKSIZE		8192
#define DO_DEBUG		0

extern void create_thread(l4_threadid_t id, l4_umword_t eip, l4_umword_t esp, 
			  l4_threadid_t new_pager);
extern void test_mem_bandwidth(void);

extern void dummy_exception13_handler(void);
extern void exception6_handler(void);
extern void exception6_c_handler(void);

extern void* memchr(const void *s, int c, unsigned n);

extern inline void call(l4_threadid_t tid);
extern inline void send(l4_threadid_t tid);
extern inline void recv(l4_threadid_t tid);
extern inline void __attribute__((noreturn)) sleep_forever(void);

extern l4_threadid_t main_id;
extern l4_threadid_t pager_id;
extern l4_threadid_t memcpy_id;
extern l4_umword_t scratch_mem;

extern char _stext, _etext, _edata, _end;

/** shake hands with tid */
extern inline void
call(l4_threadid_t tid)
{
  l4_msgdope_t result;
  l4_umword_t dummy;

  l4_i386_ipc_call(tid,
		   L4_IPC_SHORT_MSG,1,1,
		   L4_IPC_SHORT_MSG,&dummy,&dummy,
		   L4_IPC_NEVER,&result);
}

/** send short message to tid */
extern inline void
send(l4_threadid_t tid)
{
  l4_msgdope_t result;

  l4_i386_ipc_send(tid,L4_IPC_SHORT_MSG,1,1,L4_IPC_NEVER,&result);
}

/** receive short message from tid */
extern inline void
recv(l4_threadid_t tid)
{
  l4_msgdope_t result;
  l4_umword_t dummy;

  l4_i386_ipc_receive(tid,L4_IPC_SHORT_MSG,&dummy,&dummy,L4_IPC_NEVER,&result);
}

extern inline void __attribute__((noreturn))
sleep_forever(void)
{
  for (;;)
    {
      asm volatile
	  ("movl $-1,%%eax    \n\t"
	   "sub  %%ecx,%%ecx  \n\t"
	   "sub  %%esi,%%esi  \n\t"
	   "sub  %%edi,%%edi  \n\t"
	   "sub  %%ebp,%%ebp  \n\t"
	   "int  $0x30        \n\t"
	   : : : "memory");
    }
}

#endif

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#include <l4/util/thread.h>

#if defined(__i386__) || defined(ARCH_arm)
l4_threadid_t
l4util_create_thread (int thread_no, void (*function)(void), void *stack)
{
  l4_threadid_t preempter, pager;
  l4_umword_t dummy;

  /* get thread parameters */
  l4_threadid_t ret = l4_myself();

  preempter = L4_INVALID_ID;
  pager = L4_INVALID_ID;

  l4_thread_ex_regs(ret, 
		    (l4_umword_t) -1, (l4_umword_t) -1,
		    &preempter,
		    &pager,
		    &dummy,
		    &dummy,
		    &dummy);

  /* save thread id */
  ret.id.lthread = thread_no;

  /* create thread */
  l4_thread_ex_regs(ret,
		    (l4_umword_t) function, /* eip */
		    (l4_umword_t) stack, /* esp */
		    &preempter, /* preempter */
		    &pager,	/* pager */
		    &dummy,	/* old_flags */
		    &dummy,	/* old_eip */
		    &dummy);	/* old_esp */
  return ret;
}

l4_threadid_t
l4util_attach_interrupt (int irq)
{
  l4_threadid_t irq_id;
  l4_umword_t dummy, code;
  l4_msgdope_t dummydope;

  l4_make_taskid_from_irq(irq, &irq_id);

  code = l4_ipc_receive(irq_id,
			     0, /* receive descriptor */
			     &dummy,
			     &dummy,
			     L4_IPC_RECV_TIMEOUT_0,
			     &dummydope);

  return ((code != L4_IPC_RETIMEOUT) ? L4_INVALID_ID : irq_id);
}

void
l4util_detach_interrupt (void)
{
  l4_umword_t dummy, code;
  l4_msgdope_t dummydope;

  code = l4_ipc_receive(L4_NIL_ID,
			     0, /* receive descriptor */
			     &dummy,
			     &dummy,
			     L4_IPC_RECV_TIMEOUT_0,
			     &dummydope);
}


#endif

#ifdef __alpha__
l4_threadid_t
l4util_create_thread (int thread_no, void (*function)(void), void *stack)
{
  l4_threadid_t preempter, pager;
  qword_t dummy;

  /* get thread parameters */
  l4_threadid_t ret = l4_myself();

  preempter = L4_INVALID_ID;
  pager = L4_INVALID_ID;

  l4_thread_ex_regs(ret.id.lthread, 
		    -1, -1,
		    &preempter,
		    &pager,
		    &dummy,
		    &dummy);

  /* save thread id */
  ret.id.lthread = thread_no;

  /* create thread */
  l4_thread_ex_regs(thread_no,
		    (qword_t) function, /* eip */
		    (qword_t) stack, /* esp */
		    &preempter, /* preempter */
		    &pager,	/* pager */
		    &dummy,	/* old_eip */
		    &dummy);	/* old_esp */
  return ret;
}

l4_threadid_t
l4util_attach_interrupt (int irq)
{
  l4_msgdope_t ret;
  
  /* Associate irq */
  ret = l4_alpha_ipc_receive((l4_threadid_t)({thread_id:(irq|0x80)}),
			     NULL, NULL, L4_IPC_TIMEOUT (0, 0, 62500, 1, 0, 0));
  return (ret.msgdope ? L4_INVALID_ID : (l4_threadid_t)({thread_id:(irq)}));
}

void
l4util_detach_interrupt (void)
{
}

#endif


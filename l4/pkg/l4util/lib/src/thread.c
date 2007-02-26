/**
 * \file
 */

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#include <l4/util/thread.h>

/**
 * Start new local thread. Use pager of caller as pager for new thread.
 * Don't use this function when linked against the thread library!
 * \param thread_no  number of local thread
 * \param function   thread ip
 * \param stack      thread sp
 * \return           thread ID of new thread, L4_NIL_ID on failure.
 */
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
                    (l4_umword_t)-1, (l4_umword_t)-1,
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

/**
 * Attach to hardware IRQ
 * \param irq   IRQ number
 * \return      thread ID of interrupt thread on success,
 *              L4_INVALID_ID otherwise */
l4_threadid_t
l4util_attach_interrupt(int irq)
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

  return (code != L4_IPC_RETIMEOUT) ? L4_INVALID_ID : irq_id;
}

/**
 * Detach from hardware IRQ
 * \param irq   IRQ number */
void
l4util_detach_interrupt(void)
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

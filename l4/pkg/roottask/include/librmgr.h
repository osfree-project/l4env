/*!
 * \file    roottask/include/librmgr.h
 * \ingroup rmgr_api
 * \brief   Client Roottask API interface
 *
 * \date    15/10/2002
 * \author  Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
#ifndef __L4_RMGR_H
#define __L4_RMGR_H

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>
#include <l4/sys/l4int.h>

/**
 * \defgroup rmgr_api
 * \brief contains types, macros and function for the Roottask API
 *
 * The roottask is the initial task handling hardware resource access, such as
 * memory pages, IRQ, or I/O ports.  It is also responsible to monitor the
 * creation and deletion of tasks.  For these functions it implements a simple
 * API.
 */

extern int have_rmgr;
extern l4_threadid_t rmgr_id;
extern l4_threadid_t rmgr_pager_id;

EXTERN_C_BEGIN

/**
 * \brief   Initialize the roottask lib.
 * \ingroup rmgr_api
 *
 * \return  0 on success, 1 otherwise.
 *
 * Sets up the client lib and tests the availability of the roottask service.
 */
int rmgr_init(void);

/**
 * \brief   Move a task into an allocated small space.
 * \ingroup rmgr_api
 *
 * \param   dest  ID of the corresponding L4 task
 * \param   num   number of the target small space
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_set_small_space(l4_threadid_t dest, int num);

/**
 * \brief   Set the priority of a thread
 * \ingroup rmgr_api
 *
 * \param   dest target thread ID
 * \param   num  new thread priority
 *
 * \retval 0		Ok, prio was set
 * \retval -1		some error: thread does not exist or mcp of caller
 *			is below old or new prio of dest. Or prio support
 *			is disabled in librmgr.
 * \retval 16		L4_IPC_ENOT_EXISTENT: IPC error.
 *			Probably rmgr_init() was not called.
 */
int rmgr_set_prio(l4_threadid_t dest, int num);

/**
 * \brief   Query prio without rmgr involvement.
 * \ingroup rmgr_api
 *
 * \param   dest  target thread ID
 * \retval  num   the thread's current prio
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_get_prio(l4_threadid_t dest, int *num);

/**
 * \brief   Request roottask to transfer right for a task to the caller.
 * \ingroup rmgr_api
 *
 * \param   num  task number
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_get_task(int num);

/**
 * \brief   Pass right for a task back to roottask.
 * \ingroup rmgr_api
 *
 * \param   num  task number
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_free_task(int num);

/**
 * \brief   Free all L4 tasks occupied for a specific task.
 * \ingroup rmgr_api
 *
 * \param   client  target task ID
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_free_task_all(l4_threadid_t client);

/**
 * \brief   Get right to receive from specified IRQ
 * \ingroup rmgr_api
 *
 * \param   num  the number of the IRQ
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_get_irq(int num);

/**
 * \brief   Return right to receive from IRQ back to roottask.
 * \ingroup rmgr_api
 *
 * \param   num  the irq number
 *
 * \return  0 on success, 1 otherwise
 */
int rmgr_free_irq(int num);

/**
 * \brief   Free all IRQs occupied for a task
 * \ingroup rmgr_api
 *
 * \param   client  task number
 *
 * \return  0 on success, 1 otherwise
 */
int rmgr_free_irq_all(l4_threadid_t client);

/**
 * \brief   Dump the memory allocation map for all its clients.
 * \ingroup rmgr_api
 *
 * \return  0 on success, 1 otherwise
 */
int rmgr_dump_mem(void);

/**
 * \brief   Request the first physical page (roottask does not hand out \
 *          this page on pagefaults).
 * \ingroup rmgr_api
 *
 * \param   address  address where to receive that page to
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_get_page0(void *address);

/**
 * \brief   Request the task ID of the boot modules named by modname.
 * \ingroup rmgr_api
 *
 * \param   module_name  the name of the modules
 * \retval  thread_id    the ID of the module
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_get_task_id(const char *module_name, l4_threadid_t *thread_id);

/**
 * \brief   Set the task ID of a module
 * \ingroup rmgr_api
 *
 * \param   module_name  the name of the module
 * \param   thread_id    the new task ID
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_set_task_id(const char *module_name, l4_threadid_t thread_id);

/**
 * \brief   Create an L4 task
 * \ingroup rmgr_api
 *
 * \param   dest              the intended new task ID
 * \param   mcp_or_new_chief  Master Control Prio
 * \param   esp               new stack pointer
 * \param   eip               new instruction pointer
 * \param   pager             pager of the new task
 *
 * \return  a valid task ID on success, L4_NIL_ID otherwise
 */
l4_taskid_t rmgr_task_new(l4_taskid_t dest, l4_umword_t mcp_or_new_chief,
			  l4_umword_t esp, l4_umword_t eip,
			  l4_threadid_t pager);

/**
 * \brief   Create an L4 task with capability handler
 * \ingroup rmgr_api
 *
 * \param   dest              the intended new task ID
 * \param   mcp_or_new_chief  Master Control Prio
 * \param   esp               new stack pointer
 * \param   eip               new instruction pointer
 * \param   pager             pager of the new task
 * \param   caphandler        the capability fault handler
 *
 * \return  a valid task ID on success, L4_NIL_ID otherwise
 */
l4_taskid_t rmgr_task_new_with_cap(l4_taskid_t dest,
                                   l4_umword_t mcp_or_new_chief,
                                   l4_umword_t esp, l4_umword_t eip,
                                   l4_threadid_t pager,
                                   l4_threadid_t caphandler);

/**
 * \brief   Create an L4 task with capability handler
 * \ingroup rmgr_api
 *
 * \param   dest              the intended new task ID
 * \param   mcp_or_new_chief  Master Control Prio
 * \param   esp               new stack pointer
 * \param   eip               new instruction pointer
 * \param   pager             pager of the new task
 * \param   sched_param       Scheduling parameters determining the prio
 *
 * \return  a valid task ID on success, L4_NIL_ID otherwise
 */
l4_taskid_t rmgr_task_new_with_prio(l4_taskid_t dest,
				    l4_umword_t mcp_or_new_chief,
				    l4_umword_t esp, l4_umword_t eip,
				    l4_threadid_t pager,
				    l4_sched_param_t sched_param);

/**
 * \brief   Free an I/O memory page
 * \ingroup rmgr_api
 * 
 * \param   fp  the flexpage describing the I/O memory page
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_free_fpage(l4_fpage_t fp);

/**
 * \brief   Free a memory page.
 * \ingroup rmgr_api
 *
 * \param   address  the address in the memory page to free
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_free_page(l4_umword_t address);

/**
 * \brief   Free all memory a task
 * \ingroup rmgr_api
 *
 * \param   client  task ID to take memory from
 *
 * \return  0 on success, 1 otherwise.
 */
int rmgr_free_mem_all(l4_threadid_t client);

/**
 * \brief   reserve some memory chunk in an area
 * \ingroup rmgr_api
 *
 * \param   size        the size of the chunk to reserve
 * \param   align       alignment of the reserved chunk
 * \param   flags       flags for memory reservation
 * \param   range_low   the lower range of the area to check
 * \param   range_high  the upper range of the area to check
 * 
 * \return  ~0U if no area was found
 */
l4_umword_t rmgr_reserve_mem(l4_umword_t size, l4_umword_t align,
			     l4_umword_t flags, l4_umword_t range_low,
			     l4_umword_t range_high);

/**
 * \brief   elevates the calling process to PL0
 * \ingroup rmgr_api
 *
 * \param   cmd    ignored
 * \param   param  ignored
 *
 * \return  0 on success, 1 otherwise.
 *
 * This interface is for internal use only. Its unsafe, unstable, 
 * unsupported, and undocumented.
 */
int rmgr_privctrl(l4_umword_t cmd, l4_umword_t param);

EXTERN_C_END

#endif /* ! __L4_RMGR_H */

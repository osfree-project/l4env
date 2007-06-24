/*!
 * \file    roottask/include/proto.h
 * \ingroup rmgr_api
 *
 * \date    15/10/2002
 * \author  Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
#ifndef __L4_RMGR_PROTO_H__
#define __L4_RMGR_PROTO_H__

#include <l4/sys/types.h>

#define RMGR_TASK_ID       (4)  /*!< The task ID of the roottask */
#define RMGR_LTHREAD_PAGER (0)  /*!< The thread ID of the pager thread */
#define RMGR_LTHREAD_SUPER (1)  /*!< The thread ID of the super thread */

#define RMGR_RMGR (0)		/*!< PROTOCOL: rmgr meta protocol */
#define RMGR_RMGR_PING (0xf3)	/*!< ping -- returns ~arg in d2 */

#define RMGR_MEM (1)		/*!< PROTOCOL: memory operations */
#define RMGR_MEM_FREE (1)	/*!< free physical page */
#define RMGR_MEM_FREE_FP (2)	/*!< free an fpage */

/*! shortcut for messages belonging to MEM protocol class */
#define RMGR_MEM_MSG(action) L4_PROTO_MSG(RMGR_MEM, (action), 0)

#define RMGR_TASK (2)		/*!< PROTOCOL: task operations */
#define RMGR_TASK_ALLOC (1)	/*!< allocate task number */
#define RMGR_TASK_GET (2)	/*!< allocate specific task number */
#define RMGR_TASK_FREE (3)	/*!< free task number */
#define RMGR_TASK_CREATE (4)	/*!< create a task XXX */
#define RMGR_TASK_DELETE (5)	/*!< delete a task */
#define RMGR_TASK_SET_SMALL (6)	/*!< set a task's small address space number */
#define RMGR_TASK_SET_PRIO (7)	/*!< set a task's priority */
#define RMGR_TASK_GET_ID (8)	/*!< get task id by module name */
#define RMGR_TASK_CREATE_WITH_PRIO (9)	/*!< create a task and set prio XXX  */
#define RMGR_TASK_SET_ID (10)	/*!< get task id by module name */
#define RMGR_TASK_FREE_ALL (11)	/*!< free all tasks belonging to a client */

/*! shortcut for messages belonging to TASK protocol class */
#define RMGR_TASK_MSG(action, taskno) \
  L4_PROTO_MSG(RMGR_TASK, (action), (taskno))

#define RMGR_IRQ (3)            /*!< PROTOCOL: irq operations */
#define RMGR_IRQ_GET (2)	/*!< allocate an interrupt number */
#define RMGR_IRQ_FREE (3)	/*!< free an interrupt number */
#define RMGR_IRQ_FREE_ALL (4)	/*!< free all irqs belonging to a client */

#define RMGR_IRQ_MAX 18		/*!< max number of IRQs rmgr can handle */
#define RMGR_IRQ_LTHREAD 16	/*!< base thread for IRQ threads */

#define RMGR_MEM_RES_FLAGS_MASK	0x0FC0  /*!< bit-mask for memory reservation */
#define RMGR_MEM_RES_DMA_ABLE	0x0040	/*!< memory has to lay below 16 MB */
#define RMGR_MEM_RES_UPWARDS	0x0080	/*!< search upwards */
#define RMGR_MEM_RES_DOWNWARDS	0x0000	/*!< search downwards (default) */

#endif /* ! __L4_RMGR_PROTO_H__ */

/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__sync.h
 * \brief  Synchronization stuff.
 *
 * \date   07/09/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _DSI___SYNC_H
#define _DSI___SYNC_H

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>

/* library includes */
#include <l4/dsi/types.h>
#include "__debug.h"

/*****************************************************************************
 * data structures
 *****************************************************************************/

/**
 * Message buffer for IPC to synchronization thread 
 */
typedef struct dsi_sync_msg
{
  l4_threadid_t sync_th;   /* synchronization thread id */
  int           packet;    /* packet index */
  void *        rcv;       /* receive message type descriptor, either */
} dsi_sync_msg_t;

/* sync thread commands */
#define DSI_SYNC_CONNECTED     1   /* socket connect to remote component,
			            * socket->remote_socket is valid */
#define DSI_SYNC_WAIT          2   /* wait for packet commit */
#define DSI_SYNC_COMMITED      3   /* packet commited */
#define DSI_SYNC_RELEASE       4   /* packet release notification */
#define DSI_SYNC_MAP           5   /* map packet data */
#define DSI_SYNC_COPY          6   /* copy packet data */

/*****************************************************************************
 * prototypes
 *****************************************************************************/

extern inline int
dsi_down(dsi_semaphore_t * semaphore, dsi_sync_msg_t msg);

extern inline int
dsi_up(dsi_semaphore_t * semaphore, dsi_sync_msg_t msg);

extern inline int
dsi_trylock(dsi_semaphore_t * semaphore);

void
dsi_sync_thread_send(void * data);
void
dsi_sync_thread_receive(void * data);

int
dsi_start_sync_thread(dsi_socket_t * socket);

/* semaphore initializers */
#define DSI_SEMAPHORE_LOCKED   ((dsi_semaphore_t) 0)
#define DSI_SEMAPHORE_UNLOCKED ((dsi_semaphore_t) 1)

/*****************************************************************************
 * implementation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Lock semaphore counter.
 * 
 * \param semaphore      Packet semaphore counter
 * \param msg            Synchronization message
 *	
 * \retval 0		Success. the semaphore was unlocked and no call the
 *			the sync-thread was necessary.
 *
 * \retval !=0		Getting the semaphore required blocking. Return-
 *			value is the IPC error code of the L4-IPC operation.
 *
 * Lock packet semaphore counter. If packet is still locked (either the
 * receiver did not released the previous content yet if called by sender or
 * the sender did not commited the new content yet if called by receiver),
 * call the remote components synchronization thread to wait.
 *
 * \note If this function returns an error (>0), the behaviour of further
 * 	 operations on the socket is undefined. The socket should be
 * 	 closed. */
/*****************************************************************************/ 
extern inline int
dsi_down(dsi_semaphore_t * semaphore, dsi_sync_msg_t msg)
{
  l4_int8_t old,tmp;
  int ret;
  l4_msgdope_t result;
  l4_umword_t dummy;

  /* decrement semaphore counter */
  do 
    {
      old = *semaphore;
      tmp = old - 1;
    }
  while (!l4util_cmpxchg8((l4_uint8_t *)semaphore,old,tmp));

  LOGdL(DEBUG_MAP_PACKET,"rcv = 0x%08x",(unsigned)msg.rcv);

  /* check new value of semaphore counter */
  if (tmp >= 0)
    /* got semaphore counter, return immediately */
    return -1;
  else
    {
      /* semaphore already locked (counter < 0), call sync thread */
      ret = l4_ipc_call(msg.sync_th,L4_IPC_SHORT_MSG,DSI_SYNC_WAIT,
			     msg.packet,msg.rcv,&dummy,&dummy,
			     L4_IPC_NEVER,&result);

      LOGdL(DEBUG_MAP_PACKET,"result 0x%08lx",result.msgdope);

      if (ret)
        LOG_Error("DSI sync message IPC error (0x%02x)!\n",ret);

      /* return error code */
      return ret;
    }
}

/*****************************************************************************/
/**
 * \brief Unlock semaphore counter.
 * 
 * \param semaphore	Packet semaphore counter
 * \param msg		message
 *
 * \retval 0		success
 * \retval !0		tried to wakeup peer, which resulted in error
 *
 * Unlock packet semaphore counter. If remote component is waiting on this
 * packet, call its synchronization thread.
 *
 * \note If this function returns an error (!0), the behaviour of further
 * 	 operations on the socket is undefined. The socket should be
 * 	 closed.
 */
/*****************************************************************************/ 
extern inline int
dsi_up(dsi_semaphore_t * semaphore, dsi_sync_msg_t msg)
{
  l4_int8_t old,tmp;
  int ret;
  l4_msgdope_t result;

  /* increment semaphore counter */
  do
    {
      old = *semaphore;
      tmp = old + 1;
    }
  while (!l4util_cmpxchg8((l4_uint8_t *)semaphore,old,tmp));

  /* check new value of semaphore counter */
  if (tmp <= 0)
    {
      /* remote work thread is waiting, wakeup our sync thread to do reply */
      ret = l4_ipc_send(msg.sync_th,L4_IPC_SHORT_MSG,DSI_SYNC_COMMITED,
			     msg.packet,L4_IPC_NEVER,&result);
      if (ret)
        //Panic("DSI sync message IPC error (0x%02x)!\n",ret);
        ;
      return ret;
    } 
  return 0;
}

/*****************************************************************************/
/**
 * \brief Try to lock semaphore counter.
 * 
 * \param semaphore      Packet semaphore counter
 *	
 * \return 1 on success (got lock), 0 if failed
 *
 * Try to get packet semaphore without blocking if lock failed.
 */
/*****************************************************************************/ 
extern inline int
dsi_trylock(dsi_semaphore_t * semaphore)
{
  l4_int8_t old,tmp;

  /* try to get lock */
  do
    {
      old = *semaphore;

      if (old <= 0)
	/* semaphore locked */
	return 0;
	  
      /* set new semaphore value */
      tmp = old - 1;
    }
  while (!l4util_cmpxchg8((l4_uint8_t *)semaphore,old,tmp));

  /* got the lock */
  return 1;
}

#endif /* !_DSI___SYNC_H */

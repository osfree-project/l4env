/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/include/__sync.h
 * \brief  Synchronization stuff.
 *
 * \date   07/09/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>, 
 *	   Jork Loeser: 1-ipc stuff
 *
 * This include is an imlpementation with only 1 IPC. This relies on the
 * peers to cooperate, and the worker thread should not change.
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

#define DSI_SYNC_1IPC_TIMEOUT L4_IPC_TIMEOUT(1,10,1,10,0,0)

/*****************************************************************************
 * data structures
 *****************************************************************************/

/**
 * Message buffer for IPC to synchronization thread 
 */
typedef struct dsi_sync_msg
{
  l4_threadid_t work_th;   /* synchronization thread id */
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
  l4_umword_t dw0, dw1;

  /* decrement semaphore counter */
  do 
    {
      old = *semaphore;
      tmp = old - 1;
    }
  while (!cmpxchg8((l4_uint8_t *)semaphore,old,tmp));

#if DEBUG_PACKET_MAP
  INFO("rcv = 0x%08x\n",(unsigned)msg.rcv);
#endif

  /* check new value of semaphore counter */
  if (tmp >= 0)
    /* got semaphore counter, return immediately */
    return -1;
  else
    {
      /* semaphore already locked (counter < 0), wait for someone, hopefully
	 our peer. */
      ret = l4_i386_ipc_wait(&msg.work_th,
			     msg.rcv,
			     &dw0, &dw1,
			     L4_IPC_NEVER,
			     &result);
      if(dw0!=0xffee3489){
	  LOG_Error("Received IPC (%x,%x) from %x.%x, expected DSI-1 waktup",
		    dw0, dw1, msg.work_th.id.task,msg.work_th.id.lthread);
      }

#if DEBUG_PACKET_MAP
      INFO("result 0x%08x\n",result.msgdope);
#endif

      if (ret){
	  LOG_Error("DSI sync message IPC error (0x%02x)!\n",ret);
	  enter_kdebug("ipc error");;
      }

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
 * packet, send it a wakeup immediately.
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
  while (!cmpxchg8((l4_uint8_t *)semaphore,old,tmp));

  /* check new value of semaphore counter */
  if (tmp <= 0)
    {
      /* remote work thread is hopefully waiting, wake it up */
      ret = l4_i386_ipc_send(msg.work_th,
			     L4_IPC_SHORT_MSG,
			     0xffee3489,
			     msg.packet,DSI_SYNC_1IPC_TIMEOUT,
			     &result);
      if (ret){
	  LOG_Error("DSI sync message IPC error (0x%02x)!\n",ret);
      }
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
  while (!cmpxchg8((l4_uint8_t *)semaphore,old,tmp));

  /* got the lock */
  return 1;
}

#endif /* !_DSI___SYNC_H */

/* $Id$ */

/*	 con/server/include/ipc.h
 *
 *	 server ipc macros/functions
 */

#ifndef _IPC_H
#define _IPC_H

/* L4 includes */
#include <l4/sys/ipc.h>
#include <l4/util/util.h>

/*
 * some words about timeouts ...
 *
 * L4_IPC_TIMEOUT(sman, sexp, rman, rexp, spflt, rpflt)
 *
 * - timeout = man * 4^(15-exp) (�s)
 * - there are send (s*), receive (r*) and pagefault (spflt, rpflt) timeouts
 * - "Note that for efficiency reasons the highest bit of any mantissa 'man'
 *    must be 1, ..." (L4 RefMan)
 *
 * e.g.: 10 ms send timeout     -> L4_IPC_TIMEOUT(156,12,0,0,0,0),
 *       250 �s receive timeout -> L4_IPC_TIMEOUT(0,0,250,15,0,0)
 */

#define WAIT_TIMEOUT		L4_IPC_TIMEOUT(0,1,128,11,0,0)	
				/* rcv timeout: 33 ms, snd timeout 0 */
#define FIRSTWAIT_TIMEOUT	L4_IPC_NEVER
#define REPLY_TIMEOUT		L4_IPC_NEVER
#define RECEIVE_TIMEOUT		L4_IPC_NEVER
#define REPRCV_TIMEOUT		L4_IPC_NEVER

/******************************************************************************
 * check if thread is existent                                                *
 ******************************************************************************/
/* we really need this */
/* it's a useful util function - maybe for pkg/l4util? */
static inline int
thread_exists(l4_threadid_t thread)
{
  l4_umword_t dw0=0, dw1=0;
  l4_msgdope_t result;
  int error;

  error = l4_i386_ipc_receive(thread, 
			      L4_IPC_SHORT_MSG, 
		  	      &dw0, &dw1, 
	  		      L4_IPC_TIMEOUT(0, 0, 0, 1, 0, 0), 
  			      &result);

  return (error != L4_IPC_ENOT_EXISTENT);
}

#endif /* !_IPC_H */


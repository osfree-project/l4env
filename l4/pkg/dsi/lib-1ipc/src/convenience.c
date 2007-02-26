/* $Id$ */
/*****************************************************************************/
/**
 * \file	dsi/lib/src/convenience.c
 *
 * \brief	Convinience functions for our poor users.
 *
 * \date	05/15/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * 
 */
/*****************************************************************************/

#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/thread/thread.h>

#include <l4/dsi/dsi.h>
#include "__debug.h"
#include "__config.h"

/****************************************************************************
 *
 * Threading issues
 *
 ****************************************************************************/

/*!
 * \brief   Send start message to work thread and wait for ready-notification.
 * \ingroup thread
 *
 * We start the work-thread and wait until it gives an ok..
 *
 * \param  socket	socket descriptor
 * \param  ret_code	(optional) pointer to the return-code provided
 *			by the worker-thread with dsi_thread_worker_started()
 * \retval 0		on success
 * \retval -DROPS_EIPC	error sending start IPC
 *
 * The intention of this function is to be used by the creator of a
 * socket in conjunction with dsi_thread_worker_wait() used by the
 * worker thread. If the creator creates a worker-thread that handles
 * the socket, it cannot create the socket prior to the worker-thread,
 * because the worker-id is needed for socket-creation. Thus, the
 * worker-thread must be created and wait to be notified about the
 * socket then.
 */
int dsi_thread_start_worker(dsi_socket_t * socket,
			    l4_umword_t * ret_code){
    int ret;
    l4_msgdope_t result;
    l4_umword_t d1, *dp;

    dp=ret_code?ret_code:&d1;

    /* send start IPC */
    ret = l4_ipc_send(socket->work_th,L4_IPC_SHORT_MSG,(l4_umword_t)socket,0,
			   L4_IPC_NEVER,&result);
    if (ret) return -L4_EIPC;

    ret = l4_ipc_receive(socket->work_th, L4_IPC_SHORT_MSG,
			      dp, &d1, L4_IPC_NEVER, &result);
    if(ret) return -L4_EIPC;

    return 0;
}


/*!
 * \brief   Wait until our parent sends the socket.
 * \ingroup thread
 *
 * \param  socket	will be filled with the socket descriptor
 * \retval 0		on success, IPC-error otherwise
 *
 * This function should be called by a worker-thread after it was created
 * by its parent. It informs the caller about the socket to use.
 *
 * \note This function is only useful in conjunction with
 *	 dsi_thread_start_worker().
 */
int dsi_thread_worker_wait(dsi_socket_t ** socket){
    int			ret;
    l4_msgdope_t	result;
    l4_umword_t		dw1;
    l4_threadid_t	parentid;

    parentid = l4thread_l4_id (l4thread_get_parent ());

    /* wait for start notification */
    ret = l4_ipc_receive(parentid,L4_IPC_SHORT_MSG,
			      (l4_umword_t *)socket,&dw1,
			      L4_IPC_NEVER,&result);
    return ret;
}

/*!
 * \brief   Send ready-notification to parent thread.
 * \ingroup thread
 *
 * \param  ret_code	code to be send to the parent
 * \retval 0		on success
 * \retval -DROPS_EIPC	error sending start IPC
 *
 * This function should be used by a worker thread to indicate that it
 * started. An error-code can be passed to the parent.
 */
int dsi_thread_worker_started(int ret_code){
    int ret;
    l4_msgdope_t result;
    l4_threadid_t	parentid;

    parentid = l4thread_l4_id (l4thread_get_parent ());

    /* send start IPC */
    ret = l4_ipc_send(parentid,L4_IPC_SHORT_MSG,ret_code,0,
			   L4_IPC_NEVER,&result);
    if (ret) return -L4_EIPC;
    return 0;
}



/****************************************************************************
 *
 * Default Handlers
 *
 ****************************************************************************/

/*!\brief Connect-function for use with local socket refs
 * \ingroup socket
 *
 * This callback can be used for the connect-callback in an DSI
 * component descriptor (#dsi_component_t) if the socket is a socket in
 * the local address space, and no other actions must be performed on
 * connecting a socket than to call dsi_socket_connect().
 *
 * \see dsi_socket_connect().
 */
int dsi_socket_connect_local(dsi_component_t *comp,
			     dsi_socket_ref_t *remote){
    dsi_socket_t *socket;
    int ret;

    if((ret=dsi_socket_get_descriptor(comp->socketref.socket,&socket))!=0)
	return ret;
    return dsi_socket_connect(socket, remote);
}

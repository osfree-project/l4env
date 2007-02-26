/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/lib/src/libsend.c
 * \brief  Send component client lib
 *
 * \date   01/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>
#include <l4/dsi/dsi.h>
 
/* local includes */
#include <l4/dsi_example/send.h>
#include <l4/dsi_example/send-client.h>

/*****************************************************************************
 * global stuff
 *****************************************************************************/

/* send component id */
l4_threadid_t send_id = L4_INVALID_ID;

/*****************************************************************************
 * helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Get send component id
 * 
 * \return 0 on success, -1 if failed.
 */
/*****************************************************************************/ 
static inline int
__get_component_id(void)
{
  if (l4_is_invalid_id(send_id))
    {
      /* request send component id */
      if (!names_waitfor_name(DSI_EXAMPLE_SEND_NAMES,&send_id,10000))
        {
          Panic("send component (\"%s\") not found!",
		DSI_EXAMPLE_SEND_NAMES);
          send_id = L4_INVALID_ID;
          return -1;
        }
    }
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Connect components.
 * 
 * \param  local  Reference to local (send) socket
 * \param  remote Reference to remote (receive) socket
 * \return 0 on success, -1 if connect failed
 */
/*****************************************************************************/ 
static int 
__send_connect(dsi_component_t * comp, dsi_socket_ref_t * remote)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get send component id */
  if (__get_component_id() < 0)
    return -1;

  /* call send component to connect socket */
  ret = dsi_example_send_connect_call(&send_id,
  				 (dsi_example_send_socket_t *)&comp->socketref,
				 (dsi_example_send_socket_t *)remote,&env); 
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    {
      Panic("connect failed (ret %d, exc %d)",ret,env.major);
      return -1;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Start transfer.
 * 
 * \param  local Socket reference.
 * \return 0 on success, -1 if start failed.
 */
/*****************************************************************************/ 
static int
__send_start(dsi_component_t * comp)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get send component id */
  if (__get_component_id() < 0)
    return -1;

  /* call send component to start transfer */
  ret = dsi_example_send_start_call(&send_id,
  			       (dsi_example_send_socket_t *)&comp->socketref,
			       &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    {
      Panic("start failed (ret %d, exc %d)",ret,env.major);
      return -1;
    }
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Stop transfer.
 * 
 * \param  local         Socket reference 
 *	
 * \return 0 on success, -1 if failed.
 */
/*****************************************************************************/ 
static int
__send_stop(dsi_component_t * comp)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get send component id */
  if (__get_component_id() < 0)
    return -1;

  /* call send component to stop transfer */
  ret = dsi_example_send_stop_call(&send_id,
  			      (dsi_example_send_socket_t *)&comp->socketref,
			       &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    {
      Panic("stop failed (ret %d, exc %d)",ret,env.major);
      return -1;
    }
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Close send socket
 * 
 * \param  local         Socket reference
 *	
 * \return 0 on success, -1 if failed.
 */
/*****************************************************************************/ 
static int
__send_close(dsi_component_t * comp)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get send component id */
  if (__get_component_id() < 0)
    return -1;

  /* call send component to stop transfer */
  ret = dsi_example_send_close_call(&send_id,
  			       (dsi_example_send_socket_t *)&comp->socketref,
			       &env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    {
      Panic("close failed (ret %d, exc %d)",ret,env.major);
      return -1;
    }
  
  /* done */
  return 0;
}

/*****************************************************************************
 * API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open send socket
 * 
 * \retval sender  Reference to send component
 * \retval ctrl_ds Control dataspace
 * \retval data_ds Data dataspace
 * \return 0 on success, -1 if open failed
 */
/*****************************************************************************/ 
int 
send_open(dsi_component_t * sender, l4dm_dataspace_t * ctrl_ds, 
	  l4dm_dataspace_t * data_ds)
{
  int ret;
  dsi_example_send_socket_t socket_ref;
  CORBA_Environment env = dice_default_environment;
  
  /* get send component id */
  if (__get_component_id() < 0)
    return -1;

  /* call send component to create socket */
  ret = dsi_example_send_open_call(&send_id,&socket_ref,
			      (dsi_example_send_dataspace_t *)ctrl_ds,
			      (dsi_example_send_dataspace_t *)data_ds,&env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    {
      Panic("open socket failed (ret %d, exc %d)",ret,env.major);
      return -1;
    }

  /* setup component descriptor */
  memcpy(&sender->socketref,&socket_ref,sizeof(dsi_socket_ref_t));
  sender->connect = __send_connect;
  sender->start = __send_start;
  sender->stop = __send_stop;
  sender->close = __send_close;

  /* done */
  return 0;
}


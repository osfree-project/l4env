/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/lib/receive/libreceive.c
 * \brief  Receive component client lib.
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
#include <l4/dm_generic/dm_generic.h>
#include <l4/dsi/dsi.h>

/* local includes */
#include <l4/dsi_example/receive.h>
#include <l4/dsi_example/receive-client.h>

/*****************************************************************************
 * global stuff
 *****************************************************************************/

/* receive component id */
l4_threadid_t receive_id = L4_INVALID_ID;

/*****************************************************************************
 * helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Get receive component id
 *
 * \return 0 on success, -1 if failed.
 */
/*****************************************************************************/ 
static inline int
__get_component_id(void)
{
  if (l4_is_invalid_id(receive_id))
    {
      /* request send component id */
      if (!names_waitfor_name(DSI_EXAMPLE_RECEIVE_NAMES,&receive_id,10000))
        {
          Panic("receive component (\"%s\") not found!",
		DSI_EXAMPLE_RECEIVE_NAMES);
          receive_id = L4_INVALID_ID;
          return -1;
        }
    }
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Connect components
 * 
 * \param  local  Reference to local (receive) socket
 * \param  remote Reference to remote (send) socket
 * \return 0 on success, -1 if connect failed
 */
/*****************************************************************************/ 
static int
__receive_connect(dsi_component_t * comp, dsi_socket_ref_t * remote)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  /* get receive component id */
  if (__get_component_id() < 0)
    return -1;

  /* call receive component to connect socket */
  ret = dsi_example_receive_connect_call(&receive_id,
			    (dsi_example_receive_socket_t *)&comp->socketref,
			    (dsi_example_receive_socket_t *)remote,
			    &env); 
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
 * \brief Close receive socket.
 * 
 * \param  local         Socket reference
 *	
 * \return 0 on success, -1 if failed.
 */
/*****************************************************************************/ 
static int
__receive_close(dsi_component_t * comp)
{
  /* do nothing */
  LOGL("receiver closed.");

  /* done */
  return 0;
}

/*****************************************************************************
 * API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open receive socket
 * 
 * \param  ctrl_ds  Control dataspace
 * \param  data_ds  Data dataspace
 * \retval receiver Reference to receive component
 * \return 0 on success, -1 if open failed.
 */
/*****************************************************************************/ 
int 
receive_open(l4dm_dataspace_t ctrl_ds,l4dm_dataspace_t data_ds,
	     dsi_component_t * receiver)
{
  int ret;
  dsi_example_receive_socket_t socket_ref;
  CORBA_Environment env = dice_default_environment;
   
  /* get receive component id */
  if (__get_component_id() < 0)
    return -1;
  
  /* allow dataspace acces for receiver */
  ret = l4dm_share(&ctrl_ds,receive_id,L4DM_RW);
  if (ret < 0)
    {
      Panic("share ctrl dataspace failed: %s (%d)!",l4env_errstr(ret),ret);
      return -1;
    }

  ret = l4dm_share(&data_ds,receive_id,L4DM_RW);
  if (ret < 0)
    {
      Panic("share data dataspace failed: %s (%d)!",l4env_errstr(ret),ret);
      return -1;
    }
  
  /* call receive component to create socket */
  ret = dsi_example_receive_open_call(&receive_id,
				 (dsi_example_receive_dataspace_t *)&ctrl_ds,
				 (dsi_example_receive_dataspace_t *)&data_ds,
				 &socket_ref,&env);
  if (ret || (env.major != CORBA_NO_EXCEPTION))
    {
      Panic("open socket failed (ret %d, exc %d)",ret,env.major);
      return -1;
    }

  /* setup component descriptor */
  memcpy(&receiver->socketref,&socket_ref,sizeof(dsi_socket_ref_t));
  receiver->connect = __receive_connect;
  receiver->start = NULL;
  receiver->stop = NULL;
  receiver->close = __receive_close;

  /* done */
  return 0;
}

/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/lib/filter/libfilter.c
 * \brief  Filter component client lib.
 *
 * \date   01/15/2001
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
#include <l4/dsi_example/filter.h>
#include <l4/dsi_example/filter-client.h>

/*****************************************************************************
 * global stuff
 *****************************************************************************/

/* send component id */
l4_threadid_t filter_id = L4_INVALID_ID;

/*****************************************************************************
 * helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Get filter component id.
 *	
 * \return 0 on success, -1 if failed.
 */
/*****************************************************************************/ 
static inline int
__get_component_id(void)
{
  if (l4_is_invalid_id(filter_id))
    {
      /* request send component id */
      if (!names_waitfor_name(DSI_EXAMPLE_FILTER_NAMES,&filter_id,10000))
        {
          Panic("filter component (\"%s\") not found!\n",
		DSI_EXAMPLE_FILTER_NAMES);
          filter_id = L4_INVALID_ID;
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
 * \param  local         Reference to local socket
 * \param  remote        Reference to remote socket
 * 
 * \return 0 on success, -1 if connect failed
 */
/*****************************************************************************/ 
static int 
__filter_connect(dsi_component_t * comp, dsi_socket_ref_t * remote)
{
  int ret;
  sm_exc_t _exc;

  /* get send component id */
  if (__get_component_id() < 0)
    return -1;

  /* call send component to connect socket */
  ret = dsi_example_filter_connect(filter_id,
			   (dsi_example_filter_socket_t *)&comp->socketref,
			   (dsi_example_filter_socket_t *)remote,
			   &_exc); 
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      Panic("connect failed (ret %d, exc %d)\n",ret,_exc._type);
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
 * \brief Open filter.
 * 
 * \param  rcv_ctrl_ds   Control dataspace input socket
 * \param  rcv_data_ds   Data dataspace input socket
 * \retval rcv_filter    Reference to input socket
 * \retval snd_filter    Reference to output socket
 * \retval snd_ctrl_ds   Control dataspace output socket
 * \retval snd_data_ds   Data dataspace output socket
 *      
 * \return 0 on success, -1 if open failed.
 */
/*****************************************************************************/ 
int
filter_open(l4dm_dataspace_t rcv_ctrl_ds, l4dm_dataspace_t rcv_data_ds,
            dsi_component_t * rcv_filter, dsi_component_t * snd_filter,
            l4dm_dataspace_t * snd_ctrl_ds, l4dm_dataspace_t  * snd_data_ds)
{
  int ret;
  dsi_example_filter_socket_t rcv_socket_ref,snd_socket_ref;
  sm_exc_t _exc;
  
  /* get send component id */
  if (__get_component_id() < 0)
    return -1;

#if 0
  INFO("receive socket:\n");
  INFO("ctrl_ds %d at %x.%x\n",rcv_ctrl_ds.id,
       rcv_ctrl_ds.manager.id.task,rcv_ctrl_ds.manager.id.lthread);
  INFO("data_ds %d at %x.%x\n",rcv_data_ds.id,
       rcv_data_ds.manager.id.task,rcv_data_ds.manager.id.lthread);
#endif

  /* allow dataspace access for filter */
  ret = l4dm_share(&rcv_ctrl_ds,filter_id,L4DM_RW);
  if (ret < 0)
    {
      Panic("share ctrl dataspace failed: %s (%d)!",l4env_errstr(ret),ret);
      return -1;
    }

  ret = l4dm_share(&rcv_data_ds,filter_id,L4DM_RW);
  if (ret < 0)
    {
      Panic("share data dataspace failed: %s (%d)!",l4env_errstr(ret),ret);
      return -1;
    }

  /* call filter to create sockets */
  ret = dsi_example_filter_open(filter_id,0,
			  (dsi_example_filter_dataspace_t *)&rcv_ctrl_ds,
			  (dsi_example_filter_dataspace_t *)&rcv_data_ds,
			  &rcv_socket_ref,&snd_socket_ref,
			  (dsi_example_filter_dataspace_t *)snd_ctrl_ds,
			  (dsi_example_filter_dataspace_t *)snd_data_ds,
			  &_exc);
  if (ret || (_exc._type != exc_l4_no_exception))
    {
      Panic("open socket failed (ret %d, exc %d)\n",ret,_exc._type);
      return -1;
    }
  
  /* setup component descriptors */
  memcpy(&rcv_filter->socketref,&rcv_socket_ref,sizeof(dsi_socket_ref_t));
  rcv_filter->connect = __filter_connect;
  rcv_filter->start = NULL;
  rcv_filter->stop = NULL;

  memcpy(&snd_filter->socketref,&snd_socket_ref,sizeof(dsi_socket_ref_t));
  snd_filter->connect = __filter_connect;
  snd_filter->start = NULL;
  snd_filter->stop = NULL;

  /* done */
  return 0;
}

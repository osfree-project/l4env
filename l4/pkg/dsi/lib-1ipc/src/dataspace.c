/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi/lib/src/dataspace.c
 * \brief  Control / Data dataspace handling.
 *
 * \date   07/01/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * \todo Do not manage attach area for control / data datspaces 
 *       (next_map_cds, next_map_dds), use l4rm instead (requires that l4rm
 *       provides alligned attachs to be able to map larger areas by just 
 *       one flexpage).
 * 
 * \todo Rethink whole dataspace handling. Who creates/attaches/frees/detaches
 *       dataspaces.
 */
/*****************************************************************************/

/* standard/OSKit includes */
#include <stdlib.h>
#include <string.h>

 /* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

/* private includes */
#include <l4/dsi/dsi.h>
#include "__sync.h"
#include "__socket.h"
#include "__config.h"
#include "__debug.h"
#include "__dataspace.h"

/*****************************************************************************
 * global structures
 *****************************************************************************/

//! dataspace manager id
static l4_threadid_t dsi_dm_id = L4_INVALID_ID;

/**
 * map area control dataspaces
 */
static l4_uint32_t cds_map_area = -1;

/**
 * map area data dataspaces
 */
static l4_uint32_t dds_map_area = -1;

/*****************************************************************************
 * helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Request dataspace manager from L4 environment.
 * \ingroup internal
 * 
 * \return 0 on success (\a dsi_dm_id contains ID of dataspace manager), 
 *         -1 if querry failed.
 */
/*****************************************************************************/ 
static inline int
__get_dm_id(void)
{
  dsi_dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dsi_dm_id))
    {
      Panic("DSI: no dataspace manager found");
      dsi_dm_id = L4_INVALID_ID;
      return -1;
    }
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Setup control area.
 * \ingroup internal
 * 
 * \param  socket        Socket descriptor   
 * \param  cfg           Low level stream configuration
 * 
 * \return 0 on success, error code otherwise:
 *         - \c -L4_ENOMEM  out of memory attaching dataspace
 *         - \c -L4_ENOMAP  no map area available
 *         - \c -L4_EINVAL  invalid argument attaching dataspace
 *
 * Attach dataspace and setup area header.
 */
/*****************************************************************************/ 
static int
__setup_ctrl_ds(dsi_socket_t * socket, dsi_stream_cfg_t cfg)
{
  int ret;
  l4_size_t size;
  l4_addr_t map_addr;

  /* check map area */
  ret = l4dm_mem_size(&socket->ctrl_ds,&size);
  if (ret < 0)
    {
      Error("DSI: get dataspace size failed: %s (%d)",l4env_errstr(ret),ret);
      return -L4_EINVAL;
    }

  LOGdL(DEBUG_CTRL_DS,"attaching ctrl dataspace, size %d", size);
  LOGdL(DEBUG_CTRL_DS,"ds %d at %t",socket->ctrl_ds.id, socket->ctrl_ds.manager);
  
  /* attach dataspace */
  if (cds_map_area != -1)
    ret = l4rm_area_attach(&socket->ctrl_ds,cds_map_area,size,0,
			   L4DM_RW | L4RM_MAP,(void **)&map_addr);
  else
    ret = l4rm_attach(&socket->ctrl_ds,size,0,L4DM_RW | L4RM_MAP,
		      (void **)&map_addr);
  if (ret < 0)
    {
      Error("DSI: attach dataspace failed: %s (%d)",
	    l4env_errstr(ret),ret);
      return ret;
    }

  /* setup socket pointers */
  socket->header = (dsi_ctrl_header_t *)map_addr;
  socket->packets = (dsi_packet_t *)(map_addr + sizeof(dsi_ctrl_header_t));
  socket->sg_lists = (dsi_sg_elem_t *)(map_addr + sizeof(dsi_ctrl_header_t) +
				       cfg.num_packets * sizeof(dsi_packet_t));

  LOGdL(DEBUG_CTRL_DS,"attached crtl ds to 0x%08x",map_addr);
  LOGdL(DEBUG_CTRL_DS,"header at 0x%08x",(l4_addr_t)socket->header);
  LOGdL(DEBUG_CTRL_DS,"packets at 0x%08x",(l4_addr_t)socket->packets);
  LOGdL(DEBUG_CTRL_DS,"sg_lists at 0x%08x",(l4_addr_t)socket->sg_lists);

  /* done */
  return 0;
}

/*****************************************************************************
 * public library stuff
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Allocate and setup control area.
 * \ingroup internal
 * 
 * \param socket         Socket descriptor
 * \param jcp_stream     Stream description
 * \param cfg            Low level stream configuration
 *	
 * \return 0 on success, error code otherwise:
 *         - -DSI_ENODSM    dataspace manager not found
 *         - -L4_EIPC    IPC error calling dataspace manager
 *         - -L4_ENOMEM  not enough memory available allocating ctrl area
 *                          or attaching dataspace
 *         - -L4_ENOMAP  no map area avaliable mapping ctrl area
 *         - -L4_EINVAL  invalid argument (can happen if the map area is
 *                          already used by someone else)
 */
/*****************************************************************************/ 
int
dsi_create_ctrl_area(dsi_socket_t * socket, dsi_jcp_stream_t jcp_stream, 
		     dsi_stream_cfg_t cfg)
{
  int ret,i;
  l4_size_t size;

  /* sanity checks */
  Assert(socket != NULL);

  /* check dataspace manager */
  if (l4_thread_equal(dsi_dm_id,L4_INVALID_ID))
    {
      if (__get_dm_id())
	return -DSI_ENODSM;
    }
  ASSERT(!l4_thread_equal(dsi_dm_id,L4_INVALID_ID));

  /* calculate dataspace size */
  size = sizeof(dsi_ctrl_header_t) + 
    cfg.num_packets * sizeof(dsi_packet_t) +
    cfg.num_packets * cfg.max_sg *  sizeof(dsi_sg_elem_t);

  LOGdL(DEBUG_CTRL_DS,"header size %u",sizeof(dsi_ctrl_header_t));
  LOGdL(DEBUG_CTRL_DS,"%u packets of size %u",
        cfg.num_packets,sizeof(dsi_packet_t));
  LOGdL(DEBUG_CTRL_DS,"%u sg elems of size %u",cfg.num_packets * cfg.max_sg, 
       sizeof(dsi_sg_elem_t));
  LOGdL(DEBUG_CTRL_DS,"total size %u",size);

  /* align size to page size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* allocate dataspace */
  ret = l4dm_mem_open(dsi_dm_id,size,0,0,"DSI ctrl area",&socket->ctrl_ds);
  if (ret < 0)
    {
      Error("DSI: dataspace allocation failed: %s (%d)",l4env_errstr(ret),ret);
      return ret;
    }

  LOGdL(DEBUG_CTRL_DS,"ds %d at %x.%x",socket->ctrl_ds.id,
        socket->ctrl_ds.manager.id.task,socket->ctrl_ds.manager.id.lthread);

  /* setup dataspace */
  ret = __setup_ctrl_ds(socket,cfg);
  if (ret)
    {
      /* failed to setup control area, release dataspace */
      l4dm_close(&socket->ctrl_ds);
      return ret;
    }

  /* better to clean it */
  memset(socket->header,0,size);

  /* finish setup, do things only neccessary for newly allocated area */ 
  socket->num_packets = socket->header->num_packets 
  		      = cfg.num_packets;
  socket->num_sg_elems = socket->header->num_sg_elems
  		       = cfg.num_packets * cfg.max_sg;
  socket->header->max_sg_len = cfg.max_sg;

  /* setup packet descriptors */
  for (i = 0; i < cfg.num_packets; i++)
    {
      socket->packets[i].tx_sem = DSI_SEMAPHORE_UNLOCKED;
      socket->packets[i].rx_sem = DSI_SEMAPHORE_LOCKED;
    }

  /* setup scatter gather elements */
  for (i = 0; i < cfg.num_packets * cfg.max_sg; i++)
    socket->sg_lists[i].flags = DSI_SG_ELEM_UNUSED;

  /* done */

  return 0;
}

/*****************************************************************************/
/**
 * \brief Attach and setup control area.
 * \ingroup internal
 * 
 * \param socket	 Socket descriptor
 * \param ctrl_ds	 Control dataspace
 * \param jcp_stream	 Stream description
 * \param cfg		 Low level stream configuration
 *	
 * \return 0 on success, error code otherwise:
 *         - -L4_EINVAL  invalid dataspace or configuration
 *         - -L4_EPERM   no permissions to access dataspace
 *         - -L4_EIPC    IPC error calling dataspace manager
 *         - -L4_ENOMAP  no map area available to attach dataspace
 *         - -L4_ENOMEM  out of memory attaching dataspace
 */
/*****************************************************************************/ 
int
dsi_set_ctrl_area(dsi_socket_t * socket, l4dm_dataspace_t ctrl_ds,
		  dsi_jcp_stream_t jcp_stream, dsi_stream_cfg_t cfg)
{
  int ret;
  l4_size_t size,s;
  l4_addr_t map;

  /* sanity checks */
  Assert(socket != NULL);

  /* check dataspace */
  ret = l4dm_check_rights(&ctrl_ds,L4DM_RW);
  if (ret < 0)
    {
      Error("DSI: invalid dataspace: %s (%d)",l4env_errstr(ret),ret);
      return ret;
    }

  /* set control dataspace */
  socket->ctrl_ds = ctrl_ds;

  /* setup control area */
  ret = __setup_ctrl_ds(socket,cfg);
  if (ret)
    {
      /* failed to setup control area */
      return ret;
    }

  /* The following is a bit confusing: We have the same information twice.
     1. We store it in the write-shared header of the control-area to
        communicate it to our peer.
     2. We store it in the socket-structure to be resistant against a
        malicious peer overwriting the control-area with invalid data.
  */
  /* Obtain the values from the header of the control area. */
  socket->num_packets = socket->header->num_packets;
  socket->num_sg_elems = socket->header->num_sg_elems;

  /* sanity check, compare configuration in cfg and control area header */
  if ((cfg.num_packets != socket->num_packets) ||
      (socket->num_sg_elems != cfg.max_sg * socket->num_packets))
    {
      Error("DSI: configuration mismatch in control area");
      
      /* check if attached area is big enough */
      size = sizeof(dsi_ctrl_header_t) + 
	socket->num_packets * sizeof(dsi_packet_t) +
	socket->num_sg_elems *  sizeof(dsi_sg_elem_t);

      l4dm_mem_size(&socket->ctrl_ds,&s);

      if (size > s)
	{
	  Panic("DSI: size mismatch");
	  return -L4_EINVAL;
	}
      
      /* adapt packet / scatter gather list pointer */
      printf("DSI: adjusting packet / scatter gather list pointers\n");
      map = (l4_addr_t)socket->header;
      socket->packets = (dsi_packet_t *)(map + sizeof(dsi_ctrl_header_t));
      socket->sg_lists = (dsi_sg_elem_t *)
	(map + sizeof(dsi_ctrl_header_t) + 
	 socket->num_packets * sizeof(dsi_packet_t));
    }

  return 0;
}

/*****************************************************************************/
/**
 * \brief Release control dataspace.
 * \ingroup internal
 * 
 * \param socket         Socket descriptor
 *	
 * \return 0 on success, error code otherwise.
 *
 * Detach control dataspace and close dataspace if we created it.
 */
/*****************************************************************************/ 
int
dsi_release_ctrl_area(dsi_socket_t * socket)
{
  int ret;
  int error = 0;
  
  LOGdL(DEBUG_CTRL_DS,"detaching control area (ds %d at %x.%x)",
        socket->ctrl_ds.id,
        socket->ctrl_ds.manager.id.task,socket->ctrl_ds.manager.id.lthread);

  /* detach control dataspace */
  ret = l4rm_detach(socket->header);
  if (ret < 0)
    {
      Error("DSI: detach control dataspace failed: %s (%d)",
	    l4env_errstr(ret),ret);
      error = ret;
    }

  if (socket->flags & DSI_SOCKET_FREE_CTRL)
    {
      /* we created the control dataspace, close it */
      ret = l4dm_close(&socket->ctrl_ds);
      if (ret < 0)
	{
	  Error("DSI: close control dataspace failed: %s (%d)",
		l4env_errstr(ret),ret);
	  error = ret;
	}
    }

  /* done */
  return error;
}

/*****************************************************************************/
/**
 * \brief Attach data dataspace
 * \ingroup internal
 * 
 * \param socket         Socket descriptor
 * \param data_ds        Data dataspace
 *	
 * \return 0 on success, error code otherwise:
 *         - -L4_EINVAL  invalid dataspace
 *         - -L4_EPERM   no permissions to access dataspace
 *         - -L4_EIPC    IPC error calling dataspace manager
 *         - -L4_ENOMEM  out of memory attaching dataspace
 *         - -L4_ENOMAP  no map area available to attach dataspace
 */
/*****************************************************************************/ 
int 
dsi_set_data_area(dsi_socket_t * socket, l4dm_dataspace_t data_ds)
{
  int ret,i;
  l4_size_t size;
  void * map;
  l4dm_dataspace_t ds;
  l4_uint32_t flags;

  /* sanity checks */
  Assert(socket != NULL);

  if (IS_RECEIVE_SOCKET(socket) && (socket->flags & DSI_SOCKET_COPY))
    {
      /* we copy the data, allocate new dataspace */
      ret = l4dm_mem_size(&data_ds,&size);
      if (ret < 0)
	{
	  Error("DSI: get dataspace size failed: %s (%d)",
		l4env_errstr(ret),ret);
	  return -L4_EINVAL;
	}

      LOGdL(DEBUG_DATA_DS,"allocate data_ds, size %u",size);

      /* check dataspace manager */
      if (l4_thread_equal(dsi_dm_id,L4_INVALID_ID))
	{
	  if (__get_dm_id())
	    return -DSI_ENODSM;
	}
      ASSERT(!l4_thread_equal(dsi_dm_id,L4_INVALID_ID));

      /* allocate dataspace */
      if (l4dm_mem_ds_is_contiguous(&data_ds))
	ret = l4dm_mem_open(dsi_dm_id,size,0,L4DM_CONTIGUOUS,"DSI data",&ds);
      else
	ret = l4dm_mem_open(dsi_dm_id,size,0,0,"DSI data",&ds);
      if (ret < 0)
	{
	  Error("DSI: dataspace allocation failed: %s (%d)!",
		l4env_errstr(ret),ret);
	  return ret;
	}

      /* we must release this dataspace on close */
      socket->flags |= DSI_SOCKET_FREE_DATA;
    }
  else
    {
      /* check dataspace */
      ret = l4dm_check_rights(&data_ds,L4DM_RW);
      if (ret < 0)
	{
	  Error("DSI: invalid data dataspace: %s (%d)!",
		l4env_errstr(ret),ret);
	  return ret;
	}
      ds = data_ds;
    }

  LOGdL(DEBUG_DATA_DS,"attaching data area");
  LOGdL(DEBUG_DATA_DS,"ds %d at %#t",data_ds.id,data_ds.manager);
  
  /* get dataspace size */
  ret = l4dm_mem_size(&ds,&size);
  if (ret < 0)
    {
      Error("DSI: get dataspace size failed: %s (%d)",l4env_errstr(ret),ret);
      return -L4_EINVAL;
    }

  /* Calculate the alignment of the map area. We attach the map area to 
   * an address which is aligned to the next larger 2^n size of the dataspace
   * size. We need this to be able to specify the map area as a receive
   * fpage if we must map the data area from the remote component.
   * (Actually, we alignment is done by the region mapper, but we need 
   * the aligment to specify the receive fpage). 
   */
  i = 0;
  while (size > (1 << i)) 
    i++;
  
  flags = L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC;
  if (!(socket->flags & DSI_SOCKET_MAP))
    {
      flags |= L4RM_MAP;
#if 0 /* normal op, receiver only reads data */
      if (IS_RECEIVE_SOCKET(socket) && !(socket->flags & DSI_SOCKET_COPY))
	flags |= L4DM_RO;
      else
	flags |= L4DM_RW;
#else
	flags |= L4DM_RW;
#endif
    }
  
  /* attach dataspace */
  if (dds_map_area != -1)
    ret = l4rm_area_attach(&ds,dds_map_area,size,0,flags,&map);
  else
    ret = l4rm_attach(&ds,size,0,flags,&map);
  if (ret)
    {
      Error("DSI: attach data dataspace failed: %s (%d)",
	    l4env_errstr(ret),ret);
      return ret;
    }
  
  /* setup socket descriptor */
  socket->data_ds = ds;
  socket->data_area = map;
  socket->data_size = size;
  socket->data_map_size = i;
  socket->next_buf = map;

  LOGdL(DEBUG_DATA_DS,"attached data ds to addr 0x%08x",(l4_addr_t)map);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Detach data dataspace.
 * 
 * \param  socket        Socket descriptor
 *	
 * \return 0 on success, error code otherwise.
 */
/*****************************************************************************/ 
int 
dsi_release_data_area(dsi_socket_t * socket)
{
  int ret;
  int error = 0;

  LOGdL(DEBUG_DATA_DS,"detaching data area (ds %d at %x.%x)",
        socket->data_ds.id,
        socket->data_ds.manager.id.task,socket->data_ds.manager.id.lthread);

  /* detach data dataspace */
  ret = l4rm_detach(socket->data_area);
  if (ret < 0)
    {
      Error("DSI: detach data dataspace failed: %s (%d)",
	    l4env_errstr(ret),ret);
      error = ret;
    }

  if (socket->flags & DSI_SOCKET_FREE_DATA)
    {
      /* close dataspace */
      ret = l4dm_close(&socket->data_ds);
      if (ret < 0)
	{
	  Error("DSI: close data dataspace failed: %s (%d)",
		l4env_errstr(ret),ret);
	  error = ret;
	}
    }

  /* done */
  return error;
}

/*****************************************************************************/
/**
 * \brief Setup dataspace handling.
 */
/*****************************************************************************/ 
void
dsi_init_dataspaces(void)
{
  int ret;

#if DSI_USE_CDS_AREA
  // reserve map area for control dataspaces
  ret = l4rm_area_reserve_region(DSI_CDS_MAP_START,DSI_CDS_MAP_SIZE,
				 0,&cds_map_area);
  if (ret)
    {
      Error("DSI: failed to reserve map area for control dataspaces: "
	    "%s (%d)",l4env_errstr(ret),ret);
      cds_map_area = -1;
    }
#endif

#if DSI_USE_DDS_AREA
  // reserve map area for data dataspaces
  ret = l4rm_area_reserve_region(DSI_DDS_MAP_START,DSI_DDS_MAP_SIZE,
				 0,&dds_map_area);
  if (ret)
    {
      Error("DSI: failed to reserve map area for data dataspaces: "
	    "%s (%d)",l4env_errstr(ret),ret);
      dds_map_area = -1;
    }
#endif
}

/*****************************************************************************
 * API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Set new dataspace manager.
 * \ingroup general
 * 
 * \param  id            dataspace manager id
 */
/*****************************************************************************/ 
void 
dsi_set_dataspace_manager(l4_threadid_t id)
{
  /* set dataspace manager id */
  dsi_dm_id = id;
}

/*****************************************************************************/
/**
 * \brief  Share socket dataspaces
 * \ingroup socket
 * 
 * \param  socket        Socket descriptor
 * \param  client        Client thread id
 *	
 * \return 0 on success (allowed read-write access for client), 
 *         error code otherwise:
 *         - \c -L4_EINVAL invalid socket descriptor
 *         - \c -L4_EPERM  operation not allowed
 *         - \c -L4_EIPC   error calling dataspace manager
 */
/*****************************************************************************/ 
int
dsi_socket_share_ds(dsi_socket_t * socket, l4_threadid_t client)
{
  int ret;
  int error = 0;

  /* check socket descriptor */
  if (!dsi_is_valid_socket(socket))
    return -L4_EINVAL;

  if (!l4dm_is_invalid_ds(socket->data_ds))
    {
      ret = l4dm_share(&socket->data_ds,client,L4DM_RW);
      if (ret < 0)
	error = ret;
    }

  if (!l4dm_is_invalid_ds(socket->ctrl_ds))
    {
      ret = l4dm_share(&socket->ctrl_ds,client,L4DM_RW);
      if (ret < 0)
	error = ret;
    }

  /* done */
  return error;
}

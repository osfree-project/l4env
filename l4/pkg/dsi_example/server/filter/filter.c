/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/server/filter/filter.c
 * \brief  Filter component.
 *
 * \date   01/15/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/dsi/dsi.h>

/* local includes */
#include <l4/dsi_example/filter.h>
#include "filter-server.h"
#include "__config.h"

/*****************************************************************************
 * Global stuff
 *****************************************************************************/

/* Semaphore to start work thread */
l4semaphore_t sem = L4SEMAPHORE_LOCKED;

/* sockets */
dsi_socket_t * rcv_soc;
dsi_socket_t * snd_soc;

/* receive packet table, index -> snd_packet number % NUM_PACKETS */
dsi_packet_t * rcv_packets[NUM_PACKETS];

/*****************************************************************************/
/**
 * \brief Work thread.
 * 
 * \param data           Thread data (unused)
 */
/*****************************************************************************/ 
static void
filter_thread(void * data)
{
  int ret;
  dsi_packet_t * rcv_p;
  dsi_packet_t * snd_p;
  void *rcv_area,*snd_area;  
  void *addr;
  l4_addr_t offs;
  l4_size_t size;
  unsigned long count;

  /* wait for start */
  l4semaphore_down(&sem);

  /* get send/receive area */
  ret = dsi_socket_get_data_area(rcv_soc,&rcv_area,&size);
  ret = dsi_socket_get_data_area(snd_soc,&snd_area,&size);

  LOG("started, rcv at %p, snd at %p",rcv_area,snd_area);

  count = 0;
  while (1)
    {
      /* get receive packet */
      ret = dsi_packet_get(rcv_soc,&rcv_p);
      if (ret)
	{
	  Panic("get receive packet failed (%d)",ret);
	  return;
	}

      /* get packet data */
      ret = dsi_packet_get_data(rcv_soc,rcv_p,&addr,&size);
      if (ret)
	{
	  Panic("get receive data failed (%d)",ret);
	  return;
	}
      offs = addr - rcv_area;
      rcv_packets[count % NUM_PACKETS] = rcv_p;

#if 0
      KDEBUG("got rcv packet %u",rcv_p->no);
#endif

      /* get send packet */
      ret = dsi_packet_get(snd_soc,&snd_p);
      if (ret)
	{
	  Panic("get send packet failed (%d)",ret);
	  return;
	}
      
      /* add data */
      ret = dsi_packet_add_data(snd_soc,snd_p,snd_area + offs,size,0);
      if (ret)
	{
	  Panic("add data failed (%d)",ret);
	  return;
	}
	  
      /* set packet number */
      ret = dsi_packet_set_no(snd_soc,snd_p,count++);
      if (ret)
	{
	  Panic("set packet number failed (%d)",ret);
	  return;
	}
      
      /* commit send packet */
      ret = ret = dsi_packet_commit(snd_soc,snd_p);
      if (ret)
	{
	  Panic("commit send packet failed (%d)",ret);
	  return;
	}
    }

  /* this should not happen */
  Panic("left filter work loop!");
}

/*****************************************************************************/
/**
 * \brief Packet release callback.
 * 
 * \param socket         Socket descriptor
 * \param packet         Packet descriptor
 */
/*****************************************************************************/ 
void 
__release_callback(dsi_socket_t * socket, dsi_packet_t * packet);

void 
__release_callback(dsi_socket_t * socket, dsi_packet_t * packet)
{
  int ret;

#if 0
  LOG("packet %u",packet->no);
#endif  

  /* commit packet */
  ret = dsi_packet_commit(rcv_soc,rcv_packets[packet->no % NUM_PACKETS]);
  if (ret)
    Panic("commit receive packet failed (%d)",ret);
}

/*****************************************************************************
 * Interface functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open filter.
 * 
 * \param  request       Flick request structure
 * \param  num           Filter number (used in filter chains)
 * \param  rcv_ctrl_ds   Control dataspace receive socket
 * \param  rcv_data_ds   Data dataspace receive socket
 * \retval rcv_s         Receive socket reference
 * \retval snd_s         Send socket reference
 * \retval snd_ctrl_ds   Control dataspace send socket
 * \retval snd_data_ds   Data dataspace receive socket
 * \retval _ev           Flick exception structure (unused)
 *	
 * \return 0 on success, -1 if creation failed.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_filter_open_component(CORBA_Object _dice_corba_obj,
    l4_int32_t num,
    const dsi_example_filter_dataspace_t *rcv_ctrl_ds,
    const dsi_example_filter_dataspace_t *rcv_data_ds,
    dsi_example_filter_socket_t *rcv_s,
    dsi_example_filter_socket_t *snd_s,
    dsi_example_filter_dataspace_t *snd_ctrl_ds,
    dsi_example_filter_dataspace_t *snd_data_ds,
    CORBA_Environment *_dice_corba_env)
{
  int ret;
  l4_threadid_t work_id, rcv_sync_id, snd_sync_id;
  dsi_jcp_stream_t jcp;
  dsi_stream_cfg_t cfg;
  l4_uint32_t flags;
  void *addr;
  l4_addr_t a;
  l4_size_t size;

  /* start work thread */
  ret = l4thread_create_long(L4THREAD_INVALID_ID,filter_thread,
			     L4THREAD_INVALID_SP,L4THREAD_DEFAULT_SIZE,
			     255,NULL,L4THREAD_CREATE_ASYNC);
  if (ret < 0)
    {
      Panic("start work thread failed!");
      return -1;
    }
  work_id = l4thread_l4_id(ret);

  /***************************************************************************
   * create receive socket 
   ***************************************************************************/

#if 0
  LOGL("receive socket:");
  LOGL("ctrl_ds %d at %x.%x",((l4dm_dataspace_t *)rcv_ctrl_ds)->id,
       ((l4dm_dataspace_t *)rcv_ctrl_ds)->manager.id.task,
       ((l4dm_dataspace_t *)rcv_ctrl_ds)->manager.id.lthread);
  LOGL("data_ds %d at %x.%x",((l4dm_dataspace_t *)rcv_data_ds)->id,
       ((l4dm_dataspace_t *)rcv_data_ds)->manager.id.task,
       ((l4dm_dataspace_t *)rcv_data_ds)->manager.id.lthread);
#endif
       
  /* what type of socket? */
  flags = DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK;
#if DSI_MAP
  flags |= DSI_SOCKET_MAP;
#elif DSI_COPY
  flags |= DSI_SOCKET_COPY;
#endif

  /* create socket */
  cfg.num_packets = NUM_PACKETS;
  cfg.max_sg = 2;
  rcv_sync_id = L4_INVALID_ID;
  ret = dsi_socket_create(jcp,cfg,(l4dm_dataspace_t *)rcv_ctrl_ds,
			  (l4dm_dataspace_t *)rcv_data_ds,work_id, 
			  &rcv_sync_id,flags, &rcv_soc);
  if (ret)
    {
      Panic("create DSI socket failed");
      return -1;
    }

  /* get socket reference */
  ret = dsi_socket_get_ref(rcv_soc,(dsi_socket_ref_t *)rcv_s);
  if (ret)
    {
      Panic("get socket ref failed\n");
      return -1;
    }

#if DSI_COPY
  /* make sure the data area is mapped */
  ret = dsi_socket_get_data_area(rcv_soc,&addr,&size);
  if (ret)
    {
      Panic("get data area failed");
      return -1;
    }

  a = 0;
  while (a < size)
    {
      /* touch data area */
      *((int *)(addr + a)) = 1;
      a += L4_PAGESIZE;
    }
#endif

  /***************************************************************************
   * create send socket 
   ***************************************************************************/

  /* UGLY: get data dataspace from receive socket descriptor */
  *((l4dm_dataspace_t *)snd_data_ds) = rcv_soc->data_ds;

  /* what type of socket? */
  flags = DSI_SOCKET_SEND | DSI_SOCKET_BLOCK;
#if DSI_MAP
  flags |= DSI_SOCKET_MAP;
#elif DSI_COPY
  flags |= DSI_SOCKET_COPY;
#endif

  /* create socket */
  cfg.num_packets = NUM_PACKETS;
  cfg.max_sg = 2; 
  *((l4dm_dataspace_t *)snd_ctrl_ds) = L4DM_INVALID_DATASPACE;
  snd_sync_id = L4_INVALID_ID;
  ret = dsi_socket_create(jcp,cfg,(l4dm_dataspace_t *)snd_ctrl_ds,
			  (l4dm_dataspace_t *)snd_data_ds,
                          work_id,&snd_sync_id,flags,&snd_soc);
  if (ret)
    {
      Panic("create DSI socket failed");
      return -1;
    }

  /* set release callback */
  ret = dsi_socket_set_release_callback(snd_soc,__release_callback);
  if (ret)
    {
      Panic("set release callback failed");
      return -1;
    }

  /* get socket reference */
  ret = dsi_socket_get_ref(snd_soc,(dsi_socket_ref_t *)snd_s);
  if (ret)
    {
      Panic("get socket ref failed");
      return -1;
    }

  /* make sure the data area is mapped */
  ret = dsi_socket_get_data_area(snd_soc,&addr,&size);
  if (ret)
    {
      Panic("get data area failed");
      return -1;
    }

  a = 0;
  while (a < size)
    {
      /* touch data area */
      *((int *)(addr + a)) = 1;
      a += L4_PAGESIZE;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Connect to send/receive component.
 * 
 * \param  request       Flick request structure
 * \param  local         Reference to local socket
 * \param  remote        Reference to remote socket
 * \retval _ev           Flick ecxeption structure (unused)
 *	
 * \return 0 on success, -1 if connect failed.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_filter_connect_component(CORBA_Object _dice_corba_obj,
    const dsi_example_filter_socket_t *local,
    const dsi_example_filter_socket_t *remote,
    CORBA_Environment *_dice_corba_env)
{
  dsi_socket_t * s;
  int ret;

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor(local->socket,&s);
  if (ret)
    {
      Panic("invalid socket");
      return -1;
    }

  /* connect socket */
  ret = dsi_socket_connect(s,(dsi_socket_ref_t *)remote);
  if (ret)
    {
      Panic("connect failed");
      return -1;
    }

  if (s == rcv_soc)
    /* receive socket connected, start work thread */
    l4semaphore_up(&sem);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Main.
 */
/*****************************************************************************/ 
int main(void)
{
  /* init log lib */
  LOG_init("filter");

  /* init DSI lib */
  dsi_init();

  /* register at nameserver */
  if (!names_register(DSI_EXAMPLE_FILTER_NAMES))
    {
      Panic("failed to register filter!");
      return -1;
    }

  /* Dice server loop */
  dsi_example_filter_server_loop(NULL);

  /* this should never happen */
  Panic("left filter server loop...");
  return 0;
}
  

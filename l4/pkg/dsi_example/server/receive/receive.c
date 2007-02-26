/* $Id$ */
/*****************************************************************************/
/**
 * \file  dsi_example/server/receive/receive.c
 * \brief Receive component.
 *
 * \date   01/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/util/macros.h>
#include <l4/util/rdtsc.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/dsi/dsi.h>

/* local includes */
#include <l4/dsi_example/receive.h>
#include "receive-server.h"
#include "__config.h"

/*****************************************************************************
 * Global stuff
 *****************************************************************************/

/* Semaphore to start work thread */
l4semaphore_t sem = L4SEMAPHORE_LOCKED;

/* receive socket */
dsi_socket_t * soc;

/*****************************************************************************/
/**
 * \brief Work thread.
 * 
 * \param data Thread data (unused).
 */
/*****************************************************************************/ 
static void
receive_thread(void * data)
{
  int ret,count;
  l4_uint32_t num;
  dsi_packet_t * p;
  void *addr;
  l4_size_t size;
  l4_cpu_time_t t_start,t_end;
  unsigned long ms;
  unsigned long long c_start,c_end,cycles;

  /* wait for start */
  l4semaphore_down(&sem);

#if TEST_OVERHEAD
  l4thread_sleep(5000);
#endif

  Msg("started\n");

  /* start measurement */
  t_start = l4_rdtsc();

  count = 0;
  while(1)
    {
      /* get packet */
      ret = dsi_packet_get(soc,&p);
#if DO_SANITY
      if (ret)
	{
	  if (ret == -DSI_ENOPACKET)
	    break;
	  else
	    {
	      Panic("get packet failed (%d)\n",ret);
	      return;
	    }
	}
#else
      if (ret == -DSI_ENOPACKET)
	break;
#endif

      count++;

      /* get packet number */
      ret = dsi_packet_get_no(soc,p,&num);
#if DO_SANITY
      if (ret)
	{
	  Panic("get packet number failed (%d)\n",ret);
	  return;
	}
#endif

      /* get packet data */
      ret = dsi_packet_get_data(soc,p,&addr,&size);
#if DO_SANITY
      if (ret)
	{
	  Panic("get data failed (%d)\n",ret);
	  return;
	}
#endif

#if 0
      KDEBUG("got packet %u\n",num);
#endif

      /* immediately acknowledge packet */
      ret = dsi_packet_commit(soc,p);
#if DO_SANITY
      if (ret)
	{
	  Panic("commit packet failed (%d)\n",ret);
	  return;
	}
#endif

#if 0
      KDEBUG("commited packet\n");
#endif
    }

  /* end measurement */
  t_end = l4_rdtsc();

  ms = (l4_uint32_t)
	  ((l4_tsc_to_ns(t_end) - l4_tsc_to_ns(t_start)) / 1000000ULL);
  c_start = t_start;
  c_end   = t_end;
  cycles  = c_end - c_start;

  Msg("receive done (%d packets):\n",count);
  Msg("t = %lums\n",ms);
  Msg("cycles = %u:%u (%u per packet)\n",
      (l4_uint32_t)(cycles / 0x100000000ULL),
      (l4_uint32_t) cycles,
      (l4_uint32_t)(cycles / count));

#if 1
  KDEBUG("done.");
#endif

}

/*****************************************************************************
 * Interface functions 
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open receive socket.
 * 
 * \param  request       Flick request structure
 * \param  ctrl_ds       Control dataspace
 * \param  data_ds       Data dataspace
 * \retval s             Socket reference
 * \retval _ev           Flick exception structure (unused)
 * 
 * \return 0 on success, -1 if creation failed.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_receive_server_open(sm_request_t * request, 
				const dsi_example_receive_dataspace_t * ctrl_ds, 
				const dsi_example_receive_dataspace_t * data_ds, 
				dsi_example_receive_socket_t * s, 
				sm_exc_t * _ev)
{
  int ret;
  l4_threadid_t work_id, sync_id;
  dsi_jcp_stream_t jcp;
  dsi_stream_cfg_t cfg;
  l4_uint32_t flags;
#if DSI_COPY
  l4_addr_t addr,a;
  l4_size_t size;
#endif

  /* start work thread */
  ret = l4thread_create_long(L4THREAD_INVALID_ID,receive_thread,
			     L4THREAD_INVALID_SP,L4THREAD_DEFAULT_SIZE,
			     L4THREAD_DEFAULT_PRIO,NULL,L4THREAD_CREATE_ASYNC);
  if (ret < 0)
    {
      Panic("start work thread failed!\n");
      return -1;
    }
  work_id = l4thread_l4_id(ret);

  /* what type of socket? */
#if TEST_ROUNDTRIP
  flags = DSI_SOCKET_RECEIVE | DSI_SOCKET_BLOCK;
#else
  flags = DSI_SOCKET_RECEIVE;
#endif

#if DSI_MAP
  flags |= DSI_SOCKET_MAP;
#elif DSI_COPY
  flags |= DSI_SOCKET_COPY;
#endif
  
  /* create socket */
  cfg.num_packets = NUM_PACKETS;
  cfg.max_sg = 2;
  sync_id = L4_INVALID_ID;
  ret = dsi_socket_create(jcp,cfg,(l4dm_dataspace_t *)ctrl_ds,
			  (l4dm_dataspace_t *)data_ds,work_id, &sync_id,
			  flags, &soc);
  if (ret)
    {
      Panic("create DSI socket failed\n");
      return -1;
    }

  /* get socket reference */
  ret = dsi_socket_get_ref(soc,(dsi_socket_ref_t *)s);
  if (ret)
    {
      Panic("get socket ref failed\n");
      return -1;
    }

#if DSI_COPY
  /* make sure the data area is mapped */
  ret = dsi_socket_get_data_area(soc,&addr,&size);
  if (ret)
    {
      Panic("get data area failed\n");
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

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Connect to send component.
 * 
 * \param  request       Flick request structure
 * \param  local         Reference to local socket
 * \param  remote        Reference to send component's socket
 * \retval _ev           Flick exception structure (unused)
 * 
 * \return 0 on success, -1 if connect failed.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_receive_server_connect(sm_request_t * request, 
				   const dsi_example_receive_socket_t * local, 
				   const dsi_example_receive_socket_t * remote, 
				   sm_exc_t *_ev)
{
  dsi_socket_t * s;
  int ret;

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor(local->socket,&s);
  if (ret)
    {
      Panic("invalid socket\n");
      return -1;
    }

  /* connect socket */
  ret = dsi_socket_connect(s,(dsi_socket_ref_t *)remote);
  if (ret)
    {
      Panic("connect failed\n");
      return -1;
    }

#if 1
  /* immediately start work thread */
  l4semaphore_up(&sem);
#endif

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
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;
  int ret;

  /* init log lib */
  LOG_init("receive");

  /* init DSI lib */
  dsi_init();

  /* clibrate time stamp counter */
  l4_calibrate_tsc();

  /* register at nameserver */
  if (!names_register(DSI_EXAMPLE_RECEIVE_NAMES))
    {
      Panic("failed to register receiver!\n");
      return -1;
    }
  
#if 0
  INFO("receiver up.\n");
#endif

  /* Flick server loop */
  flick_init_request(&request, &ipc_buf);
  while (1)
    {
      result = flick_server_wait(&request);

      while (!L4_IPC_IS_ERROR(result))
	{
          /* dispatch request */
          ret = dsi_example_receive_server(&request);
          switch(ret)
            {
            case DISPATCH_ACK_SEND:
              /* reply and wait for next request */
              result = flick_server_reply_and_wait(&request);
              break;
              
            default:
              INFO("Flick dispatch error (%d)!\n",ret);
              
              /* wait for next request */
              result = flick_server_wait(&request);
              break;
            }
        }
      Msg("Flick IPC error (0x%08x)!\n",result.msgdope);
    }

  /* this should never happen */
  Panic("left receive server loop...\n");
  return 0;
}
  

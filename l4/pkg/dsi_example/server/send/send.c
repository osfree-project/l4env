/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/server/send/send.c
 * \brief  Send component.
 *
 * \date   01/10/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/rdtsc.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dsi/dsi.h>

/* local includes */
#include <l4/dsi_example/send.h>
#include "send-server.h"
#include "__config.h"

char LOG_tag[9]="send";

/*****************************************************************************
 * Global stuff
 *****************************************************************************/

/* Semaphore to start work thread */
l4semaphore_t sem = L4SEMAPHORE_LOCKED;

/* send socket */
dsi_socket_t * soc;

/*****************************************************************************
 * Helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Allocate dataspace
 * 
 * \param  size dataspace size
 * \retval ds   dataspace id 
 * \return 0 on success, -1 if allocation failed.
 */
/*****************************************************************************/ 
static int
__allocate_ds(l4_size_t size, l4dm_dataspace_t * ds)
{
  int ret;

  /* allocate dataspace */
  ret = l4dm_mem_open(L4DM_DEFAULT_DSM,size,0,0,"DSI data",ds);
  if (ret < 0)
    {
      Panic("dataspace allocation failed: %d",ret);
      return -1;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Work thread.
 * 
 * \param data Thread data (unused)
 */
/*****************************************************************************/ 
static void 
send_thread(void * data)
{
  int ret,i;
  unsigned long count;
  dsi_packet_t * p;
  void *start_addr, *addr;
  l4_size_t size;
  l4_cpu_time_t t_start,t_end;
  unsigned long ms;
  unsigned long long cycles,ns;
  double ns_per_cycle,rate;

  /* wait for start */
  l4semaphore_down(&sem);

  /* get data arae */
  ret = dsi_socket_get_data_area(soc,&start_addr,&size);
  if (ret)
    {
      Panic("get data area failed (%d)",ret);
      return;
    }

  LOGL("started");

  count = 0;

  /* start measurement */
  t_start = l4_rdtsc();

  for (i = 0; i < NUM_ROUNDS; i++)
    {
      addr = start_addr;
      while (addr < (start_addr + size))
	{
	  /* get packet descriptor */
	  ret = dsi_packet_get(soc,&p);
#if DO_SANITY
	  if (ret)
	    {
	      Panic("get packet failed (%d)",ret);
	      return;
	    }
#endif

          LOGdL(DO_DEBUG, "got send packet, round %d, count %lu", i, count);
#if 0
	  KDEBUG("got send packet");
#endif

	  /* add data */
	  ret = dsi_packet_add_data(soc,p,addr,PACKET_SIZE,0);
#if DO_SANITY
	  if (ret)
	    {
	      Panic("add data failed (%d)",ret);
	      return;
	    }
#endif

	  /* set packet number */
	  ret = dsi_packet_set_no(soc,p,count++);
#if DO_SANITY
	  if (ret)  
	    {
	      Panic("set packet number failed (%d)",ret);
	      return;
	    }
#endif
	    
	  /* commit packet */
	  ret = dsi_packet_commit(soc,p);
#if DO_SANITY
	  if (ret)
	    {
	      Panic("commit packet failed (%d)",ret);
	      return;
	    }
#endif
          LOGdL(DO_DEBUG, "commited send packet...");

	  addr += PACKET_SIZE;
	}
    }

#if TEST_ROUNDTRIP
  /* wait until the receiver processed all packets */
  for (i = 0; i < NUM_PACKETS; i++)
    {
      /* get packet descriptor */
      ret = dsi_packet_get(soc,&p);
      if (ret)
	{
	  Panic("get packet failed (%d)",ret);
	  return;
	}
    }
#endif

  /* end measurement */
  t_end = l4_rdtsc();

  LOGL("test done:");
  ns_per_cycle = l4_tsc_to_ns(1000000ULL) / 1000000.0;
  printf(" ns_per_cycle %u.%03u\n",(unsigned)ns_per_cycle,
         (unsigned)((ns_per_cycle - (unsigned)ns_per_cycle) * 1000));

  cycles = t_end - t_start;
  ns = cycles * ns_per_cycle;
  ms = (unsigned long)(ns / 1000000ULL);
  rate = ((double)count / (double)ms) * 1000;

#if DSI_MAP
  printf(" DSI type: map, %u byte packets\n",PACKET_SIZE);
#elif DSI_COPY
  printf(" DSI type: copy, %u byte packets\n",PACKET_SIZE);
#else
  printf(" DSI type: standard, %u byte packets\n",PACKET_SIZE);
#endif
  printf(" send done (%lu packets, %lums total)\n", count, ms);
  printf(" cycles = %lu:%lu (%lu per packet)\n",
         (unsigned long)(cycles / 0x100000000ULL),(unsigned long)cycles,
         (unsigned long)(cycles / count));
  printf(" rate %u.%03u packets/s\n",(unsigned)rate,
         (unsigned)((rate - (unsigned)rate) * 1000));

  /* signal end of stream */
  ret = dsi_socket_set_event(soc,DSI_EVENT_EOS);
  if (ret)
    {
      Panic("signal end of stream failed (%d)",ret);
      return;
    }
  l4thread_sleep(300000);

#if TEST_ROUNDTRIP
  KDEBUG("done.");
#endif
}

/*****************************************************************************
 * Interface functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Open new send socket
 * 
 * \param  request       Flick request struture
 * \retval s             Reference to new send socket
 * \retval ctrl_ds       Id of control dataspace
 * \retval data_ds       Id of data dataspace
 * \retval _ev           Flick exception structure (unused)
 * 
 * \return 0 on success, -1 if creation failed.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_send_open_component(CORBA_Object _dice_corba_obj,
                                dsi_example_send_socket_t *s,
                                dsi_example_send_dataspace_t *ctrl_ds,
                                dsi_example_send_dataspace_t *data_ds,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  l4_threadid_t work_id,sync_id;
  l4dm_dataspace_t * cds = (l4dm_dataspace_t *)ctrl_ds;
  dsi_jcp_stream_t jcp;
  dsi_stream_cfg_t cfg;
  l4_uint32_t flags;
  void *addr;
  l4_size_t size,a;

  /* allocate data area */
  ret = __allocate_ds(PACKET_SIZE * NUM_PACKETS,(l4dm_dataspace_t *)data_ds);
  if (ret < 0)
    {
      Panic("allocation of data area failed!");
      return -1;
    }

  /* start work thread */
  ret = l4thread_create_long(L4THREAD_INVALID_ID,send_thread, 0,
			     L4THREAD_INVALID_SP,L4THREAD_DEFAULT_SIZE,
			     L4THREAD_DEFAULT_PRIO,NULL,L4THREAD_CREATE_ASYNC);
  if (ret < 0)
    {
      Panic("start work thread failed");
      return -1;
    }
  work_id = l4thread_l4_id(ret);

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
  *cds = L4DM_INVALID_DATASPACE;
  sync_id = L4_INVALID_ID;
  ret = dsi_socket_create(jcp,cfg,cds,(l4dm_dataspace_t *)data_ds,
                          work_id,&sync_id,flags,&soc);
  if (ret)
    {
      Panic("create DSI socket failed");
      return -1;
    }

  /* get socket reference */
  ret = dsi_socket_get_ref(soc,(dsi_socket_ref_t *)s);
  if (ret)
    {
      Panic("get socket ref failed");
      return -1;
    }

  /* make sure the data area is mapped */
  ret = dsi_socket_get_data_area(soc,&addr,&size);
  if (ret)
    {
      Panic("get data area failed");
      return -1;
    }

  /* share dataspaces with caller */
  ret = dsi_socket_share_ds(soc,*_dice_corba_obj);
  if (ret < 0)
    {
      Panic("share dataspaces failed: %s (%d)!",l4env_errstr(ret),ret);
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
 * \brief Connect to receive component.
 * 
 * \param  request Flick request structure
 * \param  local   Local socket
 * \param  remote  Remote socket
 * \retval _ev     Flick exception structure (unused)
 * \return 0 on success, -1 if connect failed.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_send_connect_component(CORBA_Object _dice_corba_obj,
                                   const dsi_example_send_socket_t *local,
                                   const dsi_example_send_socket_t *remote,
                                   CORBA_Server_Environment *_dice_corba_env)
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

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Start transfer.
 * 
 * \param  request Flick request structure
 * \param  local   Socket
 * \retval _ev     Flick exception structure (unused)
 * \return 0 on success, -1 if failed.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_send_start_component(CORBA_Object _dice_corba_obj,
                                 const dsi_example_send_socket_t *local,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  /* start semaphore thread */
  l4semaphore_up(&sem);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Stop transfer.
 * 
 * \param  request       Flick request structure
 * \param  local         Socket reference
 * \retval _ev           Flick exception structure (unused)
 *	
 * \return 0
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_send_stop_component(CORBA_Object _dice_corba_obj,
                                const dsi_example_send_socket_t *local,
                                CORBA_Server_Environment *_dice_corba_env)
{
  LOGL("stopped");
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Close send socket.
 * 
 * \param  request       Flick request structure
 * \param  local         Socket reference
 * \retval _ev           Flick exception structure (unused)
 *	
 * \return 0 on success, error code otherwise.
 */
/*****************************************************************************/ 
l4_int32_t 
dsi_example_send_close_component(CORBA_Object _dice_corba_obj,
                                 const dsi_example_send_socket_t *local,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  dsi_socket_t * s;

  /* get socket descriptor */
  ret = dsi_socket_get_descriptor(local->socket,&s);
  if (ret)
    Panic("get socket descriptor failed (%x)",-ret);

  /* close socket */
  ret = dsi_socket_close(s);
  if (ret)
    Panic("close socket failed (%x)",-ret);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * Main
 */
/*****************************************************************************/ 
int main(void)
{
  /* init log lib */
  /* init DSI lib */
  dsi_init();

  /* clibrate time stamp counter */
  l4_calibrate_tsc();

  /* register at nameserver */
  if (!names_register(DSI_EXAMPLE_SEND_NAMES))
    {
      Panic("failed to register sender!");
      return -1;
    }

#if 0
  LOGL("sender up.");
#endif

  /* Dice server loop */
  dsi_example_send_server_loop(NULL);

  /* this should never happen */
  Panic("left send server loop...");
  return 0;
}
  



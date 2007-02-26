/* $Id$ */
/*****************************************************************************/
/**
 * \file   dsi_example/server/app/main.c
 * \brief  Control application.
 *
 * \date   01/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/dsi/dsi.h>

/* local includes */
#include <l4/dsi_example/send.h>
#include <l4/dsi_example/receive.h>
#include <l4/dsi_example/filter.h>
#include "__config.h"

/*****************************************************************************/
/**
 * \brief Main.
 */
/*****************************************************************************/ 
int main(void)
{
  dsi_component_t sender,receiver;
  l4dm_dataspace_t ctrl1;
  l4dm_dataspace_t data1;
  dsi_stream_t * s1;
  int ret,i;
  dsi_select_socket_t s_select[2];
  dsi_select_socket_t s_events[2];
  int num_events;
#if USE_FILTER
  dsi_component_t rcv_filter,snd_filter;
  l4dm_dataspace_t ctrl2;
  l4dm_dataspace_t data2;
  dsi_stream_t * s2;
#endif

  /* init log library */
  LOG_init("app");

  /* init DSI library */
  dsi_init();

#if 0
  printf("app up.\n");
#endif

  /* open send socket */
  ret = send_open(&sender,&ctrl1,&data1);
  if (ret)
    {
      Panic("open send socket failed");
      return -1;
    }

#if USE_FILTER
  /* open filter */
  ret = filter_open(ctrl1,data1,&rcv_filter,&snd_filter,&ctrl2,&data2);
  if (ret)
    {
      Panic("open filter failed");
      return -1;
    }

  /* open receive socket */
  ret = receive_open(ctrl2,data2,&receiver);
  if (ret)
    {
      Panic("open receive socket failed");
      return -1;
    }

  /* create streams */
  ret = dsi_stream_create(&sender,&rcv_filter,ctrl1,data1,&s1);
  if (ret)
    {
      Panic("creat stream1 failed");
      return -1;
    }

  ret = dsi_stream_create(&snd_filter,&receiver,ctrl2,data2,&s2);
  if (ret)
    {
      Panic("creat stream2 failed");
      return -1;
    }

#else /* USE_FILTER */

  /* open receive socket */
  ret = receive_open(ctrl1,data1,&receiver);
  if (ret)
    {
      Panic("open receive socket failed");
      return -1;
    }

  /* create streams */
  ret = dsi_stream_create(&sender,&receiver,ctrl1,data1,&s1);
  if (ret)
    {
      Panic("create stream1 failed");
      return -1;
    }
#endif

  l4thread_sleep(2000);
  
  /* start transfer */
  LOGL("start stream...");
  ret = dsi_stream_start(s1);
  if (ret)
    {
      Panic("start failed");
      return -1;
    }

  /* wait for end of stream */
  s_select[0].stream = s1;
  s_select[0].component = DSI_SEND_COMPONENT;
  s_select[0].events = DSI_EVENT_EOS;
  s_select[1].stream = s1;
  s_select[1].component = DSI_RECEIVE_COMPONENT;
  s_select[1].events = DSI_EVENT_EOS;
  ret = dsi_stream_select(s_select,2,s_events,&num_events);
  if (ret)
    {
      Panic("select failed (%d)",ret);
      return -1;
    }

  LOGL("number of events: %d",num_events);
  for (i = 0; i < num_events; i++)
    {
      if (s_events[i].events & DSI_EVENT_EOS)
	{
	  LOGL("EOS, close stream...");
	  ret = dsi_stream_close(s_events[i].stream);
	  if (ret)
	    Panic("stream close failed (%d)",ret);

	}
    }

  KDEBUG("done.");

  /* done */
  return 0;
}

 

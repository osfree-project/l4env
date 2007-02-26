/* $Id$ */
/*****************************************************************************/
/**
 * \file   pingpong/server/src/ipc_buffer.c
 */
/*****************************************************************************/

#include <l4/sys/types.h>

#include "global.h"
#include "ipc_buffer.h"

long_send_msg_t long_send_msg[NR_MSG];
long_recv_msg_t long_recv_msg[NR_MSG];
indirect_send_msg_t indirect_send_msg[NR_MSG];
indirect_recv_msg_t indirect_recv_msg[NR_MSG];
indirect_send_str_t indirect_send_str[NR_STRINGS*NR_MSG*4096*4]
  __attribute__((aligned(4096)));
indirect_recv_str_t indirect_recv_str[NR_STRINGS*NR_MSG*4096*4]
  __attribute__((aligned(4096)));

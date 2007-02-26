#ifndef IPC_BUFFER_H
#define IPC_BUFFER_H

typedef struct __attribute__ ((aligned(4096)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[NR_DWORDS];
} long_send_msg_t;

typedef struct __attribute__ ((aligned(4096)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[NR_DWORDS];
} long_recv_msg_t;

typedef struct __attribute__ ((aligned(128)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[4];
  l4_strdope_t  str[NR_STRINGS];
} indirect_send_msg_t;

typedef struct __attribute__ ((aligned(128)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[4];
  l4_strdope_t  str[NR_STRINGS];
} indirect_recv_msg_t;

typedef char indirect_send_str_t;
typedef char indirect_recv_str_t;

extern long_send_msg_t long_send_msg[NR_MSG];
extern long_recv_msg_t long_recv_msg[NR_MSG];
extern indirect_send_msg_t indirect_send_msg[NR_MSG];
extern indirect_recv_msg_t indirect_recv_msg[NR_MSG];
extern indirect_send_str_t indirect_send_str[NR_STRINGS*NR_MSG*4096*4];
extern indirect_recv_str_t indirect_recv_str[NR_STRINGS*NR_MSG*4096*4];

#endif

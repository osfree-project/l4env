/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/examples/test/client.c
 * \brief  Testcase; client.
 *
 * \date   30/08/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* *** GENERAL INCLUDES *** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/util/util.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>

/* *** LOCAL INCLUDES *** */
#include "util.h"

/* ******************************************************************* */

/* l4vfs client-lib is not thread-safe -> NUM_CLIENT=1 */
#define NUM_CLIENTS 1

char LOG_tag[9]="client";

/* ******************************************************************* */

static void client(void *p);
static void client(void *p) {

  int                fd, ret, addr_len, i;
  struct sockaddr_un addr;
  char               msg_big[4096];
  char               buf[32];
  struct timeval     tv0, tv1;

#if 0
  l4thread_started(NULL);
#endif

  fd = socket(PF_LOCAL, SOCK_STREAM, 0);
  LOG("socket(): %d; errno=%d: %s", fd, errno, strerror(errno));  

  fill_addr(&addr, &addr_len, "/tmp/sock");
  ret = connect(fd, (struct sockaddr *) &addr, addr_len);
  LOG("connect(): %d; errno=%d: %s", ret, errno, strerror(errno));  

#if 1
  /* send a lot of data to server process */
  LOG("Sending some data to server to benchmark throughput ...");
  gettimeofday(&tv0, NULL);
  for (i = 0; i < 500; i++) {
    ret = write(fd, msg_big, 4096);
    //LOG("write(): %d; errno=%d: %s", ret, errno, strerror(errno));  
    if (ret <= 0)
      break;
  }
  gettimeofday(&tv1, NULL);
  LOG("elapsed time: %ld ms", elapsed_time(&tv0, &tv1));

  ret = shutdown(fd, SHUT_WR);
  LOG("shutdown(): %d; errno=%d: %s", ret, errno, strerror(errno));  
#endif

#if 1
  ret = fcntl(fd, F_SETFL, O_NDELAY);
  LOG("fcntl(): %d; errno=%d: %s", ret, errno, strerror(errno));  
  ret = fcntl(fd, F_GETFL);
  LOG("fcntl(): %d; errno=%d: %s", ret, errno, strerror(errno));  

  while (simple_select(fd, 0) == 0)
    ; /* do nothing */
#endif

  ret = read(fd, buf, 8);
  LOG("read(): %d; errno=%d: %s", ret, errno, strerror(errno));  
  LOG("received message via read(): '%s'", buf);

  l4_sleep(3000);
  ret = close(fd);
  LOG("close(): %d; errno=%d: %s", ret, errno, strerror(errno));

#if 1
  l4thread_started(NULL);
#endif
}

/* ******************************************************************* */

int main(int argc, char **argv) {

  int i;

  // wait for local_socks-test_server to bind() to socket
  l4_sleep(500);

  // ignore errors which might occur while opening STDIN, ...!
  errno = 0;

  for (i = 0; i < NUM_CLIENTS; i++)
    l4thread_create(client, NULL, L4THREAD_CREATE_SYNC);

  l4_sleep_forever();

  return 0;
}


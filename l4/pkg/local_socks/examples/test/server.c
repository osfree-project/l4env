/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/examples/test/server.c
 * \brief  Testcase; server.
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

char LOG_tag[9]="server";

/* ******************************************************************* */

static void server(void *p);
static void server(void *p) {

  int fd0, fd1, ret, addr0_len, addr1_len;
  struct sockaddr_un addr0, addr1;
  char msg[] = "0123456";
  char *buf;

  // ignore errors which might occur while opening STDIN, ...!
  errno = 0;

  fd0 = socket(PF_LOCAL, SOCK_STREAM, 0);
  LOG("socket(): %d; errno=%d: %s", fd0, errno, strerror(errno));  

  fill_addr(&addr0, &addr0_len, "/tmp/sock");
  ret = bind(fd0, (struct sockaddr *) &addr0, addr0_len);
  LOG("bind(): %d; errno=%d: %s", ret, errno, strerror(errno));  

  ret = listen(fd0, 4);
  LOG("listen(): %d; errno=%d: %s", ret, errno, strerror(errno));  

  while (1) {
    
    //simple_select(fd0, 0);

    fill_addr(&addr1, &addr1_len, "");
    fd1 = accept(fd0, (struct sockaddr *) &addr1, &addr1_len);
    LOG("accept(): %d; errno=%d: %s", fd1, errno, strerror(errno));  
    
    buf = (char *) malloc(4096);

#if 1
    /* receive a lot of data */
    //ret = fcntl(fd1, F_SETFL, O_NDELAY);
    //LOG("fcntl(): %d; errno=%d: %s", ret, errno, strerror(errno));
    do {
      ret = read(fd1, buf, 4096);
      //LOG("+read(): %d; errno=%d: %s", ret, errno, strerror(errno));
      //l4_sleep(400);
    } while (ret > 0);
#endif

#if 1
    /* let select() in client process wait 1000 ms */
    l4_sleep(1000);
#endif
    ret = send(fd1, msg, 3, 0);
    LOG("send(): %d; errno=%d: %s", ret, errno, strerror(errno));

    ret = send(fd1, msg + 3, 5, 0);
    LOG("send(): %d; errno=%d: %s", ret, errno, strerror(errno));  

    ret = close(fd1);
    LOG("close(): %d; errno=%d: %s", ret, errno, strerror(errno));  
  }
}

/* ******************************************************************* */

int main(int argc, char **argv) {

  server(NULL);

  return 0;
}


/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/examples/test/util.c
 * \brief  Testcase; utility functions.
 *
 * \date   17/09/2004
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
#include <errno.h>

/* *** L4-SPECIFIC INCLUDES *** */
#include <l4/util/util.h>
#include <l4/log/l4log.h>

/* *** LOCAL INCLUDES *** */
#include "util.h"

void fill_addr(struct sockaddr_un *addr, int *len, char *path) {

  strncpy(addr->sun_path, path, 107);
  addr->sun_path[107] = 0;
  *len                = sizeof(addr->sun_family) + strlen(addr->sun_path) + 1;
  addr->sun_family    = AF_LOCAL;
}


int simple_select(int fd, int mode) {

  int            ret;
  fd_set         fds, *rfds, *wfds;
  struct timeval timeout;

  timeout.tv_sec  = 3;
  timeout.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  if (mode == 0) {
    rfds = &fds;
    wfds = NULL;
  } else {
    rfds = NULL;
    wfds = &fds;
  }

  ret = select(FD_SETSIZE, rfds, wfds, NULL, &timeout);
  LOG("select(): %d; errno=%d: %s", ret, errno, strerror(errno));

  return ret;
}


long elapsed_time(struct timeval *tv0, struct timeval *tv1) {

  long t;

  t = (tv1->tv_sec - tv0->tv_sec) * 1000;
  if (t) {
    t -= tv0->tv_usec / 1000;
    t += tv1->tv_usec / 1000;
  } else
    t = (tv1->tv_usec - tv0->tv_usec) / 1000;

  return t;
}



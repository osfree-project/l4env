/* $Id$ */
/*****************************************************************************/
/**
 * \file   local_socks/examples/test/util.h
 * \brief  Testcase; Header file for utility functions.
 *
 * \date   17/09/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __UTIL_H_uniq_sdasdf
#define __UTIL_H_uniq_sdasdf

extern void fill_addr(struct sockaddr_un *addr, int *len, char *path);
extern int simple_select(int fd, int mode);
extern long elapsed_time(struct timeval *tv0, struct timeval *tv1);

#endif /* __UTIL_H_uniq_sdasdf */

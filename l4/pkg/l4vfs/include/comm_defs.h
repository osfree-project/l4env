/**
 * \file   l4vfs/include/comm_defs.h
 * \brief  Communication defines for l4vfs
 *
 * \date   2007-08-23
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_COMM_DEFS_H_
#define __L4VFS_INCLUDE_COMM_DEFS_H_

#define L4VFS_WRITE_RCVBUF_SIZE 65536

//#include <sys/un.h>  // for 'struct sockaddr_un'
//#define L4VFS_SOCKET_MAX_ADDRLEN (sizeof(struct sockaddr_un))
// todo: DICE currently can not parse the above, so we hardcode the
//       number for now, bug is filed (#143)
//       The current number 120 leaves some room for compiler padding
//       in the struct
#define L4VFS_SOCKET_MAX_ADDRLEN 120

#endif

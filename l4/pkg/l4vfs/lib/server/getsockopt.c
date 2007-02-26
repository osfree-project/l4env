/**
 * \file   l4vfs/lib/server/getsockopt.c
 * \brief  
 *
 * \date   2006-10-18
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <net_io-server.h>
#include <l4/log/l4log.h>

l4_int32_t
l4vfs_net_io_getsockopt_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t s,
                                  int level,
                                  int optname,
                                  char *optval,
                                  int *optlen,
                                  int *actual_optlen,
                                  CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

l4_int32_t
l4vfs_net_io_getsockopt_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t s,
                                  int level,
                                  int optname,
                                  char *optval,
                                  int *optlen,
                                  int *actual_optlen,
                                  CORBA_Server_Environment *_dice_corba_env)
{
    LOG("l4vfs_net_io_getsockopt_component is not implemented!");
    return -ENOTSOCK;
}

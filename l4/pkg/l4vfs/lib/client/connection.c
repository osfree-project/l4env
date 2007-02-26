/**
 * \file   l4vfs/lib/client/connection.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/connection.h>
#include <l4/l4vfs/connection-client.h>
#include <l4/sys/types.h>

#include <stdlib.h>

l4_threadid_t l4vfs_init_connection(l4_threadid_t server)
{
    l4_threadid_t server_thread;
    CORBA_Environment env = dice_default_environment;
    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;

    server_thread = l4vfs_connection_init_connection_call(&server, &env);

    /* check if server implements connection interface */
    if (env.major    == CORBA_SYSTEM_EXCEPTION &&
        env.repos_id == CORBA_DICE_EXCEPTION_WRONG_OPCODE)
    {
        return server;   // no, server does not, so use the service thread
    }

    return server_thread;
}

void l4vfs_close_connection(l4_threadid_t server, l4_threadid_t connection)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    l4vfs_connection_close_connection_call(&server,
                                           &connection,
                                           &_dice_corba_env);
}

/**
 * \file   l4vfs/lib/client/basic_name_server.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "basic_name_server-client.h"

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/basic_name_server.h>

#include <l4/log/l4log.h>

#include <stdlib.h>

object_id_t l4vfs_resolve(l4_threadid_t server,
                          object_id_t base,
                          const char * pathname)
{
    object_id_t ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    ret = l4vfs_basic_name_server_resolve_call(&server, &base, pathname,
                                               &_dice_corba_env);

    return ret;
}

char *  l4vfs_rev_resolve(l4_threadid_t server, object_id_t dest,
                          object_id_t *parent)
{
    char * ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    ret = l4vfs_basic_name_server_rev_resolve_call(&server, &dest, parent,
                                                   &_dice_corba_env);

    if ( _dice_corba_env.major != CORBA_NO_EXCEPTION )
    {
        LOG("Resolving failed due to an exception!");
        if (_dice_corba_env.major == CORBA_SYSTEM_EXCEPTION)
        {
            switch( _dice_corba_env.repos_id )
            {
            case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
                LOG("Server did not recognize the opcode");
                return NULL;
            case CORBA_DICE_EXCEPTION_IPC_ERROR:
                LOG("IPC error occured.");
                return NULL;
            default:
                LOG("some other error found: %d", _dice_corba_env.repos_id );
                return NULL;
            }
        }
    }
    return ret;
}

l4_threadid_t l4vfs_thread_for_volume(l4_threadid_t server,
                                      volume_id_t volume_id)
{
    l4_threadid_t ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    ret = l4vfs_basic_name_server_thread_for_volume_call(&server, volume_id,
                                                         &_dice_corba_env);

    if ( _dice_corba_env.major != CORBA_NO_EXCEPTION )
    {
        LOG("Resolving failed due to an exception!");
        if (_dice_corba_env.major == CORBA_SYSTEM_EXCEPTION)
        {
            switch( _dice_corba_env.repos_id )
            {
            case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
                LOG("Server did not recognize the opcode");
                return L4_INVALID_ID;
            case CORBA_DICE_EXCEPTION_IPC_ERROR:
                LOG("IPC error occured.");
                return L4_INVALID_ID;
            default:
                LOG("some other error found: %d", _dice_corba_env.repos_id );
                return L4_INVALID_ID;
            }
        }
    }
    return ret;
}

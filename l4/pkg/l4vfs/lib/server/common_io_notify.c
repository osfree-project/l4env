/**
 * \file   l4vfs/lib/server/common_io_notify.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/common_io_notify-server.h>
#include <l4/log/l4log.h>

// this should normally not be useful as anyone using the
// common_io_notify interface would want to implement his
// own version

void l4vfs_common_io_read_notify_component( CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            l4_int32_t retval,
                            const char *buf,
                            l4vfs_size_t *count,
                            const l4_threadid_t *source,
                            CORBA_Server_Environment *_dice_corba_env )
__attribute__((weak));

void l4vfs_common_io_read_notify_component( CORBA_Object _dice_corba_obj,
                            object_handle_t fd,
                            l4_int32_t retval,
                            const char *buf,
                            l4vfs_size_t *count,
                            const l4_threadid_t *source,
                            CORBA_Server_Environment *_dice_corba_env )
{
    LOG("weak read notify");
}

void
l4vfs_common_io_notify_write_notify_component(CORBA_Object _dice_corba_obj,
                                              object_handle_t fd,
                                              l4_int32_t retval,
                                              l4vfs_size_t *count,
                                              const l4_threadid_t *source,
                                              CORBA_Server_Environment *_dice_corba_env)
__attribute__((weak));

void
l4vfs_common_io_notify_write_notify_component(CORBA_Object _dice_corba_obj,
                                              object_handle_t fd,
                                              l4_int32_t retval,
                                              l4vfs_size_t *count,
                                              const l4_threadid_t *source,
                                              CORBA_Server_Environment *_dice_corba_env)
{
    LOG("weak write notify");
}


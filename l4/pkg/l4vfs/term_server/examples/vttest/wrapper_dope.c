/**
 * \file   l4vfs/term_server/examples/vttest/wrapper_dope.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Björn Döbel  <doebel@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/volume_ids.h>
#include <l4/sys/types.h>
#include <l4/util/util.h>

#include <dice/dice.h> // for CORBA_*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "wrapper.h"

#ifdef DEBUG
int _DEBUG = 1;
#else
int _DEBUG = 0;
#endif

char LOG_tag[9] = "vttest";

int main( int argc, char **argv )
{ 
    l4_threadid_t ns;
    int ret,fd1,fd2;

    ns = l4vfs_get_name_server_threadid();
    l4_sleep(2000);

    LOG("vttest");

    while ((ret = l4vfs_attach_namespace(ns, 1000,
                                         "/", "/test3")) == 3)
    {
        LOG("ret = %d - server not found", ret);
        l4_sleep(1000);
    }

    LOG("attached namespace");

    ret = open("/test3/vc1", O_RDONLY);
    fd1 = open("/test3/vc1", O_WRONLY);
    fd2 = open("/test3/vc1", O_WRONLY);

    LOG("opened vc1: %d, %d, %d", ret, fd1, fd2);

    return do_main( argc, argv );
    
}

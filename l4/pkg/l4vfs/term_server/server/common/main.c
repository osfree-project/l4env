/**
 * \file   l4vfs/term_server/server/common/main.c
 * \brief  global term_server startup
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// server headers
#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/name_space_provider.h>
#include <l4/l4vfs/types.h>
#include <l4/l4vfs/object_server-server.h>
#include <l4/term_server/main-server.h>
// l4 headers
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>

#include "term.h"

#ifdef DEBUG
    int _DEBUG = 1;
#else
    int _DEBUG = 0;
#endif

extern int myvolume;
  
int main(int argc, char **argv)
{
    l4_threadid_t ns;
    int ret, i;
    CORBA_Server_Environment env = dice_default_server_environment;
    object_id_t root_id;

    // call private init (this at least sets up myvolume)
    ret = term_server_init();

    if (ret)
    {
        LOG("term initialization failed.");
        exit(1);
    }
    
    // set up root object
    root_id.volume_id = myvolume;
    root_id.object_id = 0;

    LOG("starting.");
    // wait some time for the name_server to set up
    ns = l4vfs_get_name_server_threadid();
    LOG("name server is: "l4util_idfmt, l4util_idstr(ns));

    // try to register at name_server 3 times before giving up
    // Martin: I think the loop is not needed anymore
    for (i=0; i<3; i++)
    {
        ret = l4vfs_register_volume(ns, l4_myself(), root_id);
        // registered ok --> done
        if (ret == 0) break;
        else LOG("error registering: %d", ret);
    }

    // if i>2 we tried 3 times and did not succeed
    if (i>2)
    {
        LOG("tried 3 times: registering failed");
        exit(1);
    }
    LOG("Registered at name_server");
    LOG("ready to run.");

    // server loop
    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;
    l4vfs_term_server_main_server_loop(&env);

    return 0;
}

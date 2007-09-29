/*
 * \brief   Test application for BMSI test cases.
 * \date    2006-07-14
 * \author  Christelle Braun <cbraun@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007 Christelle Braun <cbraun@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the BMSI package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/names/libnames.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/log/l4log.h> 
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

int main(int argc, char **argv)
{
    names_register("hello_test1");

    l4_threadid_t me = l4_myself();
        
    LOG("(HELLO1[%u.%u]) START!", me.id.task, me.id.lthread);

    int i=0;
    int nb=0;
    for(i=0; i<5; i++)
    {
        LOG("(HELLO1[%u.%u]) Hello_test -- loop(%d)!", me.id.task, me.id.lthread, nb);
        l4_sleep(1000);
        nb++;
    }


    /* Wait for IPC from hello2. */

    l4_umword_t d0 = 0;
    l4_umword_t d1 = 0;

    l4_umword_t ds0 = 112;
    l4_umword_t ds1 = 122;

    l4_msgdope_t result;

    l4_threadid_t sender = L4_INVALID_ID;


    int res_ipc_w = l4_ipc_wait(&sender,
                                L4_IPC_SHORT_MSG,
                                &d0,
                                &d1,
                                L4_IPC_NEVER,
                                &result);

	LOG("(HELLO1[" l4util_idfmt "]): received ipc(%lu - %lu) from " l4util_idfmt, l4util_idstr(me), d0, d1, l4util_idstr(sender));

    /* Reply to hello2. */
    int p=0;
    int res_send_ipc = -1;
    while(res_send_ipc)
    {

        res_send_ipc = l4_ipc_send(sender,
                                L4_IPC_SHORT_MSG,
                                ds0,
                                ds1,
                                L4_IPC_NEVER,
                                &result);

        LOG("(HELLO1[%u.%u]): res_send_ipc(%d) loop(%d)",
            me.id.task,
            me.id.lthread,
            res_send_ipc,
            p);

        l4_sleep(2000);
    }

    int qq=0;
    for(qq=0; qq<5; qq++)
    {
        LOG("(HELLO1[%u.%u]): Still writing -- loop(%d)",
            me.id.task,
            me.id.lthread,
            qq);

    l4_sleep(1000);
    }

    return 0;
}

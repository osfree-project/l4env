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
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/sys/ipc.h>

int main(int argc, char **argv)
{
    names_register("hello_test2");

    l4_threadid_t me = l4_myself();
    l4_threadid_t id_hello1 = L4_INVALID_ID;

    int j=0;
    int test=0;
    /* Look for the l4_threadid of hello_test1. */
    while (!names_waitfor_name("hello_test1", &id_hello1, 3000) && test<10)
    {
        LOG("(HELLO2[%u.%u]): hello_test1 not found -- loop(%d)!", 
            me.id.task,
            me.id.lthread,
            j++);
        test++;
        l4_sleep(2000);
    } 


    int p=3;
    for(p=3; p>=0; p--)
    {
        LOG("(HELLO2[%u.%u]) hello_test1(%u.%u) -- loop(%d)", 
            me.id.task,
            me.id.lthread,
            id_hello1.id.task, id_hello1.id.lthread, p);
        l4_sleep(2000);
    }

    l4_umword_t dw0=77;
    l4_umword_t dw1=88;

    //l4_timeout_t timeout;
    l4_msgdope_t result;

    //l4_umword_t dwr0, dwr1;
  
    //int q=0;
    
    /* Send IPC to hello1. */
    int res_send_ipc = -1;
    while(res_send_ipc)
    {

       res_send_ipc = l4_ipc_send(id_hello1,
                                    L4_IPC_SHORT_MSG,
                                    dw0,
                                    dw1,
                                    L4_IPC_NEVER,
                                    &result);
		 LOG("(HELLO2[" l4util_idfmt "]): res_send_ipc(%d)", l4util_idstr(me), res_send_ipc);        

        l4_sleep(2000);
    }

    /* Wait for reply from hello1. */
    l4_umword_t d0 = 0;
    l4_umword_t d1 = 0;

    l4_msgdope_t result2;
    l4_threadid_t sender = L4_INVALID_ID;

    int res_wait_ipc = l4_ipc_wait(&sender,
                                L4_IPC_SHORT_MSG,
                                &d0,
                                &d1,
                                L4_IPC_NEVER,
                                &result2);

	LOG("(HELLO2[" l4util_idfmt "]): received ipc(%lu - %lu) from " l4util_idfmt, l4util_idstr(me), d0, d1, l4util_idstr(sender));

    int qq=0;
    while (qq<5)
    {
        LOG("(HELLO2[%u.%u]): Still writing -- loop(%d)...", 
            me.id.task,
            me.id.lthread,
            qq);
        l4_sleep(1000);
        qq++;
    }
 
    return 0;
}

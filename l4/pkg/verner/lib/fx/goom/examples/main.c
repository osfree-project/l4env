/*
 * \brief   Startup's and main loop for VERNER's sync component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


/* STD/L4 Includes */
#include <stdio.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/dsi/dsi.h>

/* local includes */
#include "goom.h"

/* cfg */
l4_ssize_t l4libc_heapsize = (5*1024*1024);
char LOG_tag[9] = "goom";

int
main (int argc, char **argv)
{

    jeko_init();
    
    l4thread_sleep(5000);
    jeko_set_text("BOOM GOOM test");

    l4thread_sleep(5000);
    jeko_set_text("Does it work?");


    l4thread_sleep(5000);
    jeko_set_text("Yes. Have fun.");
    
    while(1) l4thread_sleep(1);
    
    jeko_cleanup();
    
}

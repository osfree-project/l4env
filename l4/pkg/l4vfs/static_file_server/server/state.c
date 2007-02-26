/**
 * \file   l4vfs/static_file_server/server/state.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "state.h"
#include <l4/log/l4log.h>

extern int _DEBUG;

static_file_t files[MAX_STATIC_FILES];

extern int _binary_hosts_start;
extern int _binary_hosts_size;

void state_init(void)
{
    int i;
    for (i = 0; i < MAX_STATIC_FILES; i++)
    {
        files[i].name = NULL;
        files[i].data = NULL;
        files[i].length = 0;
    }

    files[0].name = "hosts";
     /* get adr. of linked file */
    files[0].data = (char *)&_binary_hosts_start;
    files[0].length = (int)&_binary_hosts_size; /* size of linked file */
}

/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include <l4/log/l4log.h>
#include "uip.h"

// no logging per default - the uIP lib is too talkative
void uip_log(char *m)
{
//          LOG("%s", m);
}


/**
 * \file   rtc/lib/libc_backends/time/l4libc_init_time.c
 * \brief  init function
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <sys/time.h>
#include <errno.h>

#include <l4/rtc/rtc.h>
#include <l4/crtx/ctor.h>
#include <l4/log/l4log.h>

#include "gettime.h"

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

unsigned int offset;

void l4libc_init_time(void);
void l4libc_init_time(void)
{
    int ret;
    LOGd(_DEBUG, "Init. time backend ...");

    libc_backend_rtc_init();

    ret = l4rtc_get_offset_to_realtime(&offset);

    // error, assume offset 0
    if (ret != 0)
    {
        LOG("RTC server not found, assuming 1.1.1970, 0:00 ...");
        offset = 0;
    }
    LOGd(_DEBUG, "Init. time backend ... done!");
}
L4C_CTOR(l4libc_init_time, 1200);

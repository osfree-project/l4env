/**
 * \file   rt_mon/examples/dope_mon/main.c
 * \brief  Example monitoring server for DOpE.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/dm_phys/dm_phys.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/rand.h>
#include <l4/util/util.h>
#include <l4/semaphore/semaphore.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/thread/thread.h>

#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    int ret, count;
    long app_id;
    void * vscr4_id;

    // low count protect slow computers
    if (argc == 2)
    {
        count = atoi(argv[1]);
        printf("count = %d\n", count);
    }
    else
    {
        printf("no count specified, using 4.\n");
        count = 4;
    }

    while ((ret = dope_init()))
    {
        printf("Waiting for DOpE ...\n");
        l4_sleep(100);
    }

    // fixme: check ret
    app_id = dope_init_app("DOpE RtMon");

    dope_cmd(app_id, "window = new Window()");
    dope_cmd(app_id, "grid = new Grid()");
    dope_cmd(app_id, "vscr1 = new VScreen()");
    dope_cmd(app_id, "vscr2 = new VScreen()");
    dope_cmd(app_id, "vscr3 = new VScreen()");
    dope_cmd(app_id, "vscr4 = new VScreen()");
    if (count >= 4)
        dope_cmd(app_id, "vscr1.set(-framerate 25)");
    if (count >= 3)
        dope_cmd(app_id, "vscr2.set(-framerate 25)");
    if (count >= 2)
        dope_cmd(app_id, "vscr3.set(-framerate 25)");
    if (count >= 1)
        dope_cmd(app_id, "vscr4.set(-framerate 25)");
    dope_cmd(app_id, "vscr1.setmode(50, 50, \"RGB16\")");
    dope_cmd(app_id, "vscr2.setmode(50, 50, \"RGB16\")");
    dope_cmd(app_id, "vscr3.setmode(50, 50, \"RGB16\")");
    dope_cmd(app_id, "vscr4.setmode(50, 50, \"RGB16\")");
    dope_cmd(app_id, "grid.place(vscr1, -column 0 -row 0)");
    dope_cmd(app_id, "grid.place(vscr2, -column 1 -row 0)");
    dope_cmd(app_id, "grid.place(vscr3, -column 0 -row 1)");
    dope_cmd(app_id, "grid.place(vscr4, -column 1 -row 1)");
    dope_cmd(app_id, "window.set(-content grid)");
    dope_cmd(app_id, "window.open()");
    dope_cmd(app_id, "window.set(-workw 410 -workh 410)");

    vscr4_id = vscr_get_server_id(app_id, "vscr4");

    for (;;)
    {
        int w, h, oldw = 0, oldh = 0;
        w = l4util_rand() % 400;
        h = l4util_rand() % 400;
	(void)oldw; (void)oldh;
        vscr_server_waitsync(vscr4_id);
        dope_cmdf(app_id, "grid.rowconfig(0, -size %d)",       h);
        dope_cmdf(app_id, "grid.rowconfig(1, -size %d)", 401 - h);
        dope_cmdf(app_id, "grid.columnconfig(0, -size %d)",       w);
        dope_cmdf(app_id, "grid.columnconfig(1, -size %d)", 401 - w);

        // the idea here is to order the resize requests such, that
        // the resulting window will never get bigger, that is, always
        // do requests that make the window smaller first.
/*
        // for some reason it does not work well on slow systems, so
        // deactivate the complicated stuff for now
        if (h < oldh)
        {
            dope_cmdf(app_id, "grid.rowconfig(0, -size %d)",       h);
            dope_cmdf(app_id, "grid.rowconfig(1, -size %d)", 401 - h);
        }
        else
        {
            dope_cmdf(app_id, "grid.rowconfig(1, -size %d)", 401 - h);
            dope_cmdf(app_id, "grid.rowconfig(0, -size %d)",       h);
        }
        if (w < oldw)
        {
            dope_cmdf(app_id, "grid.columnconfig(0, -size %d)",       w);
            dope_cmdf(app_id, "grid.columnconfig(1, -size %d)", 401 - w);
        }
        else
        {
            dope_cmdf(app_id, "grid.columnconfig(1, -size %d)", 401 - w);
            dope_cmdf(app_id, "grid.columnconfig(0, -size %d)",       w);
        }

        oldw = w;
        oldh = h;
 */
    }

    return 0;
}
